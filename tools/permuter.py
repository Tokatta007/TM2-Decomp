#!/usr/bin/env python3
"""Source permuter for close functions (docs/PERMUTER.md).

Randomly rewrites the body of one function of a maintained C unit with
plain-C, behaviour-preserving transformations (statement order, temporaries,
declarations, loop shape, do/while(0) blocks, cross-jump duplicates, ...),
compiles every candidate with the unit's real recipe (PsyQ CC1PSX in DOSBox,
via tools/match_unit.compile_candidate) and keeps the variants whose bytes come
closer to retail. Score = differing words of the target function against the
retail slot (as tools/match_unit.py counts them); a size mismatch adds
--size-penalty, a compile error scores 10**6.

    py -3.14 tools/permuter.py --source src/semantic/font_init.c --va 80113F30 \
        --out <dir> --jobs 3 --minutes 60 [--seed file.c ...] [--resume]
    py -3.14 tools/permuter.py --source ... --va ... --out <dir> --eval variant.c

Nothing in src/ is ever written. Improvements are logged to <out>/<func>.log
(with a diff against the starting function), the best function text is kept in
<out>/<func>.best.c, every 0-word candidate in <out>/<func>.match_N.c, and the
search state in <out>/<func>.state.json (--resume continues from it). A match
is only a candidate: it must be reviewed for behaviour equivalence by hand.
"""
import argparse
import csv
import difflib
import hashlib
import json
import math
import random
import re
import shutil
import subprocess
import sys
import threading
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import match_unit as mu  # noqa: E402
from semantic_compile import CC1, MIPS, ROOT, preprocess  # noqa: E402
from semantic_elf import global_symbol, read_elf  # noqa: E402
from build_maximal_hybrid import SECTION_HEX, address_for, symbol_map  # noqa: E402

FAIL = 10 ** 6

# --------------------------------------------------------------------------- tokens

KEYWORDS = {'auto', 'break', 'case', 'char', 'const', 'continue', 'default', 'do', 'double', 'else', 'enum',
            'extern', 'float', 'for', 'goto', 'if', 'int', 'long', 'register', 'return', 'short', 'signed',
            'sizeof', 'static', 'struct', 'switch', 'typedef', 'union', 'unsigned', 'void', 'volatile', 'while',
            '__typeof__', 'typeof', '__inline__', 'inline', '__attribute__', '__extension__', '__const__',
            '__volatile__', '__signed__'}
TYPE_WORDS = {'char', 'const', 'double', 'enum', 'extern', 'float', 'int', 'long', 'register', 'short', 'signed',
              'static', 'struct', 'union', 'unsigned', 'void', 'volatile', '__typeof__', 'typeof', '__signed__',
              '__const__', 'auto'}
OPS = ['...', '<<=', '>>=', '->', '++', '--', '<<', '>>', '<=', '>=', '==', '!=', '&&', '||', '+=', '-=', '*=',
       '/=', '%=', '&=', '|=', '^=']
ASSIGN_OPS = {'=', '+=', '-=', '*=', '/=', '%=', '&=', '|=', '^=', '<<=', '>>='}
NUM_RE = re.compile(r'(?:0[xX][0-9a-fA-F]+|\d+\.?\d*(?:[eE][+-]?\d+)?|\.\d+)[uUlLfF]*')
ID_RE = re.compile(r'[A-Za-z_$][\w$]*')


class Tok:
    __slots__ = ('kind', 'text', 'start', 'end')

    def __init__(self, kind, text, start, end):
        self.kind, self.text, self.start, self.end = kind, text, start, end

    def __repr__(self):
        return f'{self.kind}:{self.text}'


def tokenize(text, start=0, end=None):
    """Tokens of text[start:end]; comments and whitespace are skipped, a
    preprocessor line is one 'pp' token."""
    end = len(text) if end is None else end
    out, i = [], start
    while i < end:
        c = text[i]
        if c in ' \t\r\n\f\v':
            i += 1
            continue
        if c == '\\' and text[i + 1:i + 2] in ('\n', '\r'):
            i += 1
            continue
        if text.startswith('/*', i):
            j = text.find('*/', i + 2)
            i = end if j < 0 else j + 2
            continue
        if text.startswith('//', i):
            j = text.find('\n', i)
            i = end if j < 0 else j
            continue
        if c == '#':
            ls = text.rfind('\n', 0, i) + 1
            if not text[ls:i].strip():
                j = i
                while True:
                    k = text.find('\n', j)
                    if k < 0:
                        k = end
                        break
                    if text[k - 1] == '\\' or (text[k - 1] == '\r' and text[k - 2] == '\\'):
                        j = k + 1
                        continue
                    break
                k = min(k, end)
                out.append(Tok('pp', text[i:k], i, k))
                i = k
                continue
        if c in '"\'':
            j = i + 1
            while j < end and text[j] != c:
                j += 2 if text[j] == '\\' else 1
            out.append(Tok('str' if c == '"' else 'chr', text[i:j + 1], i, j + 1))
            i = j + 1
            continue
        m = NUM_RE.match(text, i) if (c.isdigit() or (c == '.' and text[i + 1:i + 2].isdigit())) else None
        if m:
            out.append(Tok('num', m.group(), i, m.end()))
            i = m.end()
            continue
        m = ID_RE.match(text, i)
        if m:
            out.append(Tok('id', m.group(), i, m.end()))
            i = m.end()
            continue
        for op in OPS:
            if text.startswith(op, i):
                out.append(Tok('op', op, i, i + len(op)))
                i += len(op)
                break
        else:
            out.append(Tok('op', c, i, i + 1))
            i += 1
    return out


def match_close(toks, i):
    """Index of the bracket closing toks[i] ('(', '[' or '{')."""
    pairs = {'(': ')', '[': ']', '{': '}'}
    o = toks[i].text
    c = pairs[o]
    depth = 0
    for j in range(i, len(toks)):
        t = toks[j].text
        if toks[j].kind != 'op':
            continue
        if t == o:
            depth += 1
        elif t == c:
            depth -= 1
            if depth == 0:
                return j
    raise ValueError('unbalanced ' + o)


# --------------------------------------------------------------------------- statements


class Node:
    """A statement. ti..tj is its token range (inclusive), start..end its text span."""

    def __init__(self, kind, ti, tj, toks, **kw):
        self.kind, self.ti, self.tj = kind, ti, tj
        self.start, self.end = toks[ti].start, toks[tj].end
        self.parent = None
        self.__dict__.update(kw)

    def children(self):
        for k in ('stmts',):
            for s in getattr(self, k, ()) or ():
                yield s
        for k in ('then', 'els', 'body'):
            s = getattr(self, k, None)
            if s is not None:
                yield s

    def walk(self):
        yield self
        for c in self.children():
            yield from c.walk()

    def __repr__(self):
        return f'<{self.kind} {self.ti}-{self.tj}>'


class Parser:
    def __init__(self, toks, typedefs):
        self.toks, self.typedefs = toks, typedefs

    def is_decl(self, i):
        t = self.toks
        if t[i].kind != 'id':
            return False
        if t[i].text in TYPE_WORDS or t[i].text == 'typedef':
            return True
        if t[i].text in KEYWORDS:
            return False
        if i + 1 < len(t) and t[i + 1].kind == 'id' and t[i + 1].text not in KEYWORDS:
            return True
        if t[i].text in self.typedefs:
            j = i + 1
            while j < len(t) and t[j].text == '*':
                j += 1
            if j > i + 1 and j < len(t) and t[j].kind == 'id':
                return True
        return False

    def stmt_end(self, i):
        """Index of the ';' ending a simple statement starting at i (skipping brackets)."""
        t = self.toks
        while i < len(t):
            x = t[i]
            if x.kind == 'op' and x.text in '([{':
                i = match_close(t, i) + 1
                continue
            if x.kind == 'op' and x.text == ';':
                return i
            i += 1
        raise ValueError('missing ;')

    def block(self, i, j):
        """Statements of toks[i:j]."""
        out = []
        while i < j:
            s = self.stmt(i)
            out.append(s)
            i = s.tj + 1
        return out

    def stmt(self, i):
        t = self.toks
        x = t[i]
        if x.kind == 'pp':
            return Node('pp', i, i, t)
        if x.kind == 'op' and x.text == '{':
            k = match_close(t, i)
            n = Node('block', i, k, t, stmts=self.block(i + 1, k))
            return n
        if x.kind == 'op' and x.text == ';':
            return Node('empty', i, i, t)
        if x.kind == 'id':
            w = x.text
            if w == 'if':
                c = match_close(t, i + 1)
                then = self.stmt(c + 1)
                els = None
                k = then.tj
                if k + 1 < len(t) and t[k + 1].text == 'else':
                    els = self.stmt(k + 2)
                    k = els.tj
                return Node('if', i, k, t, cond=(i + 2, c - 1), then=then, els=els)
            if w in ('while', 'switch'):
                c = match_close(t, i + 1)
                body = self.stmt(c + 1)
                return Node(w, i, body.tj, t, cond=(i + 2, c - 1), body=body)
            if w == 'for':
                c = match_close(t, i + 1)
                semis = [k for k in range(i + 2, c) if t[k].text == ';' and self.depth_at(i + 2, k) == 0]
                body = self.stmt(c + 1)
                return Node('for', i, body.tj, t, init=(i + 2, semis[0] - 1), cond=(semis[0] + 1, semis[1] - 1),
                            inc=(semis[1] + 1, c - 1), body=body)
            if w == 'do':
                body = self.stmt(i + 1)
                k = body.tj + 1  # 'while'
                c = match_close(t, k + 1)
                return Node('do', i, c + 1, t, body=body, cond=(k + 2, c - 1))
            if w in ('return', 'break', 'continue', 'goto'):
                return Node('jump', i, self.stmt_end(i), t, word=w)
            if w in ('case', 'default'):
                k = i
                while t[k].text != ':':
                    k += 1
                return Node('label', i, k, t)
            if i + 1 < len(t) and t[i + 1].text == ':' and w not in KEYWORDS:
                return Node('label', i, i + 1, t)
            if self.is_decl(i):
                return Node('decl', i, self.stmt_end(i), t)
        return Node('expr', i, self.stmt_end(i), t)

    def depth_at(self, a, k):
        d = 0
        for q in range(a, k):
            if self.toks[q].text in ('(', '[', '{'):
                d += 1
            elif self.toks[q].text in (')', ']', '}'):
                d -= 1
        return d


# --------------------------------------------------------------------------- expressions


class Ex:
    """Expression node over token range ti..tj (inclusive)."""

    def __init__(self, kind, ti, tj, op=None, kids=()):
        self.kind, self.ti, self.tj, self.op, self.kids = kind, ti, tj, op, list(kids)

    def walk(self):
        yield self
        for k in self.kids:
            yield from k.walk()

    def __repr__(self):
        return f'Ex({self.kind},{self.op},{self.ti}-{self.tj})'


BIN_PREC = {'*': 10, '/': 10, '%': 10, '+': 9, '-': 9, '<<': 8, '>>': 8, '<': 7, '>': 7, '<=': 7, '>=': 7,
            '==': 6, '!=': 6, '&': 5, '^': 4, '|': 3, '&&': 2, '||': 1}


class ExprParser:
    def __init__(self, toks, typedefs):
        self.t, self.typedefs = toks, typedefs

    def parse(self, i, j):
        self.j = j
        self.i = i
        e = self.comma()
        if self.i != j + 1:
            raise ValueError('trailing tokens')
        return e

    def peek(self):
        return self.t[self.i].text if self.i <= self.j else None

    def comma(self):
        s = self.i
        e = self.assign()
        while self.peek() == ',':
            self.i += 1
            r = self.assign()
            e = Ex('comma', s, r.tj, ',', [e, r])
        return e

    def assign(self):
        s = self.i
        e = self.cond()
        if self.peek() in ASSIGN_OPS:
            op = self.peek()
            self.i += 1
            r = self.assign()
            return Ex('assign', s, r.tj, op, [e, r])
        return e

    def cond(self):
        s = self.i
        e = self.binary(1)
        if self.peek() == '?':
            self.i += 1
            a = self.comma()
            if self.peek() != ':':
                raise ValueError(':')
            self.i += 1
            b = self.cond()
            return Ex('cond', s, b.tj, '?', [e, a, b])
        return e

    def binary(self, prec):
        s = self.i
        e = self.unary()
        while True:
            op = self.peek()
            p = BIN_PREC.get(op)
            if p is None or p < prec:
                return e
            self.i += 1
            r = self.binary(p + 1)
            e = Ex('bin', s, r.tj, op, [e, r])

    def is_type_start(self, k):
        x = self.t[k]
        return x.kind == 'id' and (x.text in TYPE_WORDS or x.text in self.typedefs)

    def unary(self):
        s = self.i
        op = self.peek()
        if op in ('-', '+', '!', '~', '*', '&', '++', '--'):
            self.i += 1
            e = self.unary()
            return Ex('unary', s, e.tj, op, [e])
        if op == 'sizeof':
            self.i += 1
            if self.peek() == '(' and self.is_type_start(self.i + 1):
                k = match_close(self.t, self.i)
                self.i = k + 1
                return Ex('sizeof', s, k, 'sizeof')
            e = self.unary()
            return Ex('sizeof', s, e.tj, 'sizeof', [e])
        if op == '(' and self.is_type_start(self.i + 1):
            k = match_close(self.t, self.i)
            self.i = k + 1
            e = self.unary()
            return Ex('cast', s, e.tj, None, [e])
        return self.postfix()

    def postfix(self):
        s = self.i
        e = self.primary()
        while True:
            op = self.peek()
            if op == '[':
                k = match_close(self.t, self.i)
                sub = ExprParser(self.t, self.typedefs).parse(self.i + 1, k - 1)
                self.i = k + 1
                e = Ex('index', s, k, '[', [e, sub])
            elif op == '(':
                k = match_close(self.t, self.i)
                args = []
                if k > self.i + 1:
                    a = ExprParser(self.t, self.typedefs).parse(self.i + 1, k - 1)
                    args = self.flatten_comma(a)
                self.i = k + 1
                e = Ex('call', s, k, '(', [e] + args)
            elif op in ('.', '->'):
                self.i += 2
                e = Ex('member', s, self.i - 1, op, [e])
            elif op in ('++', '--'):
                self.i += 1
                e = Ex('post', s, self.i - 1, op, [e])
            else:
                return e

    @staticmethod
    def flatten_comma(a):
        if a.kind == 'comma':
            return ExprParser.flatten_comma(a.kids[0]) + [a.kids[1]]
        return [a]

    def primary(self):
        x = self.t[self.i] if self.i <= self.j else None
        if x is None:
            raise ValueError('eof')
        if x.text == '(':
            k = match_close(self.t, self.i)
            if self.t[self.i + 1].text == '{':
                s = self.i
                self.i = k + 1
                return Ex('stmtexpr', s, k)
            inner = ExprParser(self.t, self.typedefs).parse(self.i + 1, k - 1)
            s = self.i
            self.i = k + 1
            return Ex('paren', s, k, '(', [inner])
        if x.kind in ('id', 'num', 'str', 'chr'):
            self.i += 1
            if x.kind == 'str':
                while self.i <= self.j and self.t[self.i].kind == 'str':
                    self.i += 1
            return Ex(x.kind, self.i - 1 if x.kind != 'str' else self.i - 1, self.i - 1)
        raise ValueError('unexpected ' + x.text)


# --------------------------------------------------------------------------- unit context


class Unit:
    """The unit text, the target function's body span and the names needed for analysis."""

    def __init__(self, text, va, include_dirs):
        self.text = text
        self.va = va
        self.nl = '\r\n' if '\r\n' in text else '\n'
        m = re.search(r'section\s*\(\s*"\.text_%s"\s*\)' % va, text, re.I)
        if not m:
            raise SystemExit(f'no function with section .text_{va} in the unit')
        self.attr = m.start()
        nm = re.search(r'\)\s*\)\s*\**\s*([A-Za-z_]\w*)\s*\(', text[m.start():])
        self.name = nm.group(1)
        self.macros = {}
        self.typedefs = set()
        self.global_arrays = {}
        self.member_arrays = set()
        self.union_members = set()
        for t in [text] + [p.read_text(errors='replace') for p in self.headers(text, include_dirs)]:
            self.scan_defs(t)

    @staticmethod
    def headers(text, dirs, seen=None):
        seen = set() if seen is None else seen
        out = []
        for inc in re.findall(r'(?m)^\s*#\s*include\s+"([^"]+)"', text):
            for d in dirs:
                p = (d / inc)
                if p.is_file() and p not in seen:
                    seen.add(p)
                    out.append(p)
                    out += Unit.headers(p.read_text(errors='replace'), dirs, seen)
                    break
        return out

    def scan_defs(self, text):
        for m in re.finditer(r'(?m)^[ \t]*#[ \t]*define[ \t]+([A-Za-z_]\w*)(\([^)]*\))?((?:[^\n\\]|\\\r?\n|\\.)*)', text):
            params = None
            if m.group(2) is not None:
                params = [p.strip() for p in m.group(2)[1:-1].split(',') if p.strip()]
            self.macros[m.group(1)] = (params, m.group(3).replace('\\\r\n', ' ').replace('\\\n', ' ').strip())
        for m in re.finditer(r'\btypedef\b([^;{]*(?:\{(?:[^{}]|\{[^{}]*\})*\})?[^;]*);', text):
            body = m.group(1)
            fp = re.search(r'\(\s*\*\s*([A-Za-z_]\w*)\s*\)', body)
            if fp:
                self.typedefs.add(fp.group(1))
                continue
            names = re.findall(r'([A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*(?:,|$)', body.strip())
            self.typedefs.update(names)
        clean = re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)
        for m in re.finditer(r'(?m)^[A-Za-z_][^;(){}=\n]*?\b([A-Za-z_]\w*)\s*\[[^\]]*\](?:\s*\[[^\]]*\])*\s*(?:=|;|,|__attribute__)', clean):
            self.global_arrays[m.group(1)] = m.group(0).count('[')
        for m in re.finditer(r'[A-Za-z_]\w*[\s*]+([A-Za-z_]\w*)\s*\[[^\]]*\](?:\s*\[[^\]]*\])*\s*[;,]', clean):
            self.member_arrays.add(m.group(1))
        for m in re.finditer(r'\bunion\b[^{;]*\{((?:[^{}]|\{[^{}]*\})*)\}', clean):
            self.union_members.update(re.findall(r'([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)*(?:;|,|:)', m.group(1)))
        for w in ('s8', 'u8', 's16', 'u16', 's32', 'u32', 'f32'):
            self.typedefs.add(w)

    def body_span(self, text):
        """(open, close) offsets of the target function's braces in text."""
        m = re.search(r'section\s*\(\s*"\.text_%s"\s*\)\s*\)\s*\)' % self.va, text, re.I)
        toks = tokenize(text, m.end())
        # first '{' at depth 0 after the parameter list
        depth = 0
        for k, x in enumerate(toks):
            if x.text == '(':
                depth += 1
            elif x.text == ')':
                depth -= 1
            elif x.text == '{' and depth == 0:
                c = match_close(toks, k)
                return x.start, toks[c].start
        raise SystemExit('function body not found')

    def func_span(self, text):
        """Offsets of the whole definition (from the start of its line to the closing brace)."""
        m = re.search(r'section\s*\(\s*"\.text_%s"\s*\)' % self.va, text, re.I)
        ls = text.rfind('\n', 0, m.start()) + 1
        _, close = self.body_span(text)
        return ls, close + 1

    def expand(self, toks, depth=0):
        """Macro-expanded copy of toks (analysis only); unknown macros stay as calls."""
        if depth > 6:
            return toks
        out, i, changed = [], 0, False
        while i < len(toks):
            x = toks[i]
            mac = self.macros.get(x.text) if x.kind == 'id' else None
            if mac is None or not mac[1]:
                out.append(x)
                i += 1
                continue
            params, body = mac
            if params is None:
                out += tokenize(' ( ' + body + ' ) ')
                i += 1
                changed = True
                continue
            if i + 1 >= len(toks) or toks[i + 1].text != '(':
                out.append(x)
                i += 1
                continue
            k = match_close(toks, i + 1)
            args, cur, d = [], [], 0
            for y in toks[i + 2:k]:
                if y.text in '([{' and y.kind == 'op':
                    d += 1
                elif y.text in ')]}' and y.kind == 'op':
                    d -= 1
                if y.text == ',' and d == 0:
                    args.append(cur)
                    cur = []
                else:
                    cur.append(y)
            args.append(cur)
            bt = tokenize(body)
            sub = {p: a for p, a in zip(params, args)}
            out.append(Tok('op', '(', 0, 0))
            for y in bt:
                if y.kind == 'id' and y.text in sub:
                    out.append(Tok('op', '(', 0, 0))
                    out += sub[y.text]
                    out.append(Tok('op', ')', 0, 0))
                else:
                    out.append(y)
            out.append(Tok('op', ')', 0, 0))
            i = k + 1
            changed = True
        return self.expand(out, depth + 1) if changed else out


class Effects:
    """Reads/writes of a token range (after macro expansion), conservatively."""

    def __init__(self):
        self.sreads, self.swrites, self.mreads, self.mwrites = set(), set(), set(), set()
        self.call = self.jump = self.label = self.decl = self.pp = False
        self.addr = set()

    def merge(self, o):
        for k in ('sreads', 'swrites', 'mreads', 'mwrites', 'addr'):
            getattr(self, k).update(getattr(o, k))
        for k in ('call', 'jump', 'label', 'decl', 'pp'):
            setattr(self, k, getattr(self, k) or getattr(o, k))


ANY = '*any*'


class Analyzer:
    """Conservative read/write sets. Memory classes: a global or local array/struct name
    (with `#k` for a constant first index), an --alias target for pointer locals, or ANY
    (any other dereference). Calls are barriers (handled by the caller)."""

    def __init__(self, unit, locals_, arrays, aliases):
        self.unit, self.locals, self.arrays, self.aliases = unit, locals_, arrays, aliases

    def is_name(self, toks, k):
        x = toks[k]
        if x.kind != 'id' or x.text in KEYWORDS or x.text in self.unit.typedefs:
            return False
        if k > 0 and toks[k - 1].text in ('.', '->', 'struct', 'union', 'enum'):
            return False
        return True

    def ref_class(self, toks, k):
        """Memory class of the reference rooted at name toks[k] (with its postfix chain and
        &/* prefix); None when it touches no memory (a local scalar, or &object)."""
        x = toks[k].text
        n = len(toks)
        # prefix: unary * and & (through parentheses and casts)
        stars = amp = 0
        casted = False
        j = k - 1
        parens = 0
        while j >= 0:
            t = toks[j].text
            if t == '(':
                parens += 1
            elif t == '&' and self.unary_at(toks, j):
                amp += 1
            elif t == '*' and self.unary_at(toks, j):
                stars += 1
            elif t == ')':
                # a cast in front: skip back to its '('
                casted = True
                d = 0
                while j >= 0:
                    if toks[j].text == ')':
                        d += 1
                    elif toks[j].text == '(':
                        d -= 1
                        if d == 0:
                            break
                    j -= 1
            else:
                break
            j -= 1
        # postfix chain
        subs = arrows = msubs = 0
        idx = ''
        member = None
        first = None
        arith = None
        q = k + 1
        while q < n:
            t = toks[q].text
            if t == '[':
                c = match_close(toks, q)
                if member is None:
                    if subs == 0 and c == q + 2 and toks[q + 1].kind == 'num':
                        idx = '#' + toks[q + 1].text
                    subs += 1
                elif member == '?' or member not in self.unit.member_arrays:
                    msubs += 1   # subscript of a pointer member: a dereference
                q = c + 1
            elif t == '.':
                member = toks[q + 1].text if q + 1 < n else ''
                if first is None:
                    first = member
                q += 2
            elif t == '->':
                arrows += 1
                member = toks[q + 1].text if q + 1 < n else ''
                if first is None:
                    first = member
                q += 2
            elif t == ')' and parens > 0:
                parens -= 1
                q += 1
            elif parens > 0 and not arith:
                # the name is an operand of arithmetic inside parentheses: skip to their end
                arith = (stars, arrows, msubs, subs)
                d = 0
                while q < n and parens > 0:
                    if toks[q].text in ('(', '['):
                        d += 1
                    elif toks[q].text in (')', ']'):
                        if d == 0:
                            parens -= 1
                        else:
                            d -= 1
                    q += 1
                member = '?'
            else:
                break
        dims = self.arrays.get(x) if x in self.locals else self.unit.global_arrays.get(x)
        if arith:
            if stars + arrows + msubs + subs - sum(arith) + arith[0] > 0 or stars > 0:
                # pointer arithmetic on the name, then a dereference
                if x in self.locals and x in self.aliases:
                    return self.aliases[x]
                if dims:
                    return x
                return ANY
            return None if x in self.locals else x
        derefs = stars + arrows + msubs + max(0, subs - (dims or 0)) - amp
        if derefs < 0:
            return None          # an address computation only
        if derefs == 0:
            if x in self.locals and not dims:
                return None      # a local scalar or struct
            if first and not casted and first not in self.unit.union_members:
                return x + idx + '@' + first
            return x + idx
        if x in self.locals and x in self.aliases and derefs == 1:
            return self.aliases[x] + idx
        return ANY

    def is_cast_close(self, toks, k):
        d = 0
        j = k
        while j >= 0:
            if toks[j].text == ')':
                d += 1
            elif toks[j].text == '(':
                d -= 1
                if d == 0:
                    break
            j -= 1
        return j + 1 < len(toks) and toks[j + 1].kind == 'id' and (toks[j + 1].text in TYPE_WORDS or toks[j + 1].text in self.unit.typedefs)

    @staticmethod
    def unary_at(toks, k):
        return k == 0 or (toks[k - 1].kind == 'op' and toks[k - 1].text not in (')', ']')) or toks[k - 1].text == 'return'

    def root_index(self, lv):
        for k in range(len(lv)):
            if self.is_name(lv, k):
                return k
        return None

    def effects(self, toks):
        toks = self.unit.expand(toks)
        e = Effects()
        n = len(toks)
        for k, x in enumerate(toks):
            if self.is_name(toks, k):
                nxt = toks[k + 1].text if k + 1 < n else None
                if nxt == '(' and x.text not in self.locals:
                    e.call = True
                    continue
                if x.text in self.locals:
                    e.sreads.add(x.text)
                if k > 0 and toks[k - 1].text == '&':
                    e.addr.add(x.text)
                try:
                    c = self.ref_class(toks, k)
                except ValueError:
                    c = ANY
                if c:
                    e.mreads.add(c)
            elif x.text == ')' and k + 1 < n and toks[k + 1].text == '(' and not self.is_cast_close(toks, k):
                e.call = True
        for k, x in enumerate(toks):
            if x.kind == 'op' and (x.text in ASSIGN_OPS or x.text in ('++', '--')):
                if x.text in ('++', '--'):
                    pre = k + 1 < n and (toks[k + 1].kind == 'id' or toks[k + 1].text in ('(', '*')) and \
                        not (k > 0 and (toks[k - 1].kind == 'id' or toks[k - 1].text in (')', ']')))
                    lv = self.operand_after(toks, k) if pre else self.operand_before(toks, k)
                else:
                    lv = self.operand_before(toks, k)
                self.add_write(e, lv)
        return e

    def add_write(self, e, lv):
        lv = list(lv)
        r = self.root_index(lv)
        if r is None:
            e.mwrites.add(ANY)
            return
        name = lv[r].text
        try:
            c = self.ref_class(lv, r)
        except ValueError:
            c = ANY
        if c is None:
            e.swrites.add(name)   # a local scalar (or a member of a local struct)
            return
        e.mwrites.add(c)

    @staticmethod
    def operand_before(toks, k):
        """Tokens of the postfix/unary expression ending right before k (an lvalue)."""
        j = k - 1
        d = 0
        while j >= 0:
            t = toks[j].text
            if t in (')', ']'):
                d += 1
            elif t in ('(', '['):
                if d == 0:
                    break
                d -= 1
            elif d == 0 and toks[j].kind == 'op' and t not in ('.', '->', '*', '&'):
                break
            elif d == 0 and t in ('*', '&') and j > 0 and (toks[j - 1].kind in ('id', 'num') or toks[j - 1].text in (')', ']')):
                break
            j -= 1
        return toks[j + 1:k]

    @staticmethod
    def operand_after(toks, k):
        j = k + 1
        d = 0
        while j < len(toks):
            t = toks[j].text
            if t in ('(', '['):
                d += 1
            elif t in (')', ']'):
                if d == 0:
                    break
                d -= 1
            elif d == 0 and toks[j].kind == 'op' and t not in ('.', '->', '*'):
                break
            j += 1
        return toks[k + 1:j]


def cls_conflict(a, b):
    if a == ANY or b == ANY:
        return True
    a, _, ma = a.partition('@')
    b, _, mb = b.partition('@')
    ba, _, ia = a.partition('#')
    bb, _, ib = b.partition('#')
    return ba == bb and (not ia or not ib or ia == ib) and (not ma or not mb or ma == mb)


def conflicts(a, b):
    """True when statements with effects a and b may not be swapped."""
    if a.call or b.call or a.jump or b.jump or a.label or b.label or a.pp or b.pp:
        return True
    if a.swrites & (b.sreads | b.swrites) or b.swrites & a.sreads:
        return True
    for x, y in ((a, b), (b, a)):
        for w in x.mwrites:
            for o in y.mreads | y.mwrites:
                if cls_conflict(w, o):
                    return True
    return False


# --------------------------------------------------------------------------- mutations


class Ctx:
    """Parsed view of one candidate text."""

    def __init__(self, unit, text, aliases):
        self.unit, self.text = unit, text
        self.open, self.close = unit.body_span(text)
        self.toks = tokenize(text, self.open, self.close + 1)
        p = Parser(self.toks, unit.typedefs)
        self.parser = p
        self.root = p.stmt(0)  # the body block
        self.locals, self.arrays = self.collect_locals()
        self.an = Analyzer(unit, self.locals, self.arrays, aliases)
        self.eff_cache = {}
        for n in self.root.walk():
            for c in n.children():
                c.parent = n

    def collect_locals(self):
        names, arrays = set(), {}
        # parameters
        hs = re.search(r'section\s*\(\s*"\.text_%s"\s*\)' % self.unit.va, self.text, re.I).end()
        head = tokenize(self.text, hs, self.open)
        for k, t in enumerate(head):
            if t.kind == 'id' and t.text not in KEYWORDS and t.text not in self.unit.typedefs and k + 1 < len(head) \
                    and head[k + 1].text in (',', ')', ';', '['):
                names.add(t.text)
        for n in self.root.walk():
            if n.kind == 'decl':
                for name, is_arr in self.declared(n):
                    names.add(name)
                    if is_arr:
                        arrays[name] = is_arr
        names.discard(self.unit.name)
        return names, arrays

    def declared(self, n):
        """[(name, is_array)] declared by decl node n."""
        toks = self.toks[n.ti:n.tj]
        out, d, k = [], 0, 0
        chunks, cur = [], []
        for t in toks:
            if t.text in '([{' and t.kind == 'op':
                d += 1
            elif t.text in ')]}' and t.kind == 'op':
                d -= 1
            if t.text == ',' and d == 0:
                chunks.append(cur)
                cur = []
            else:
                cur.append(t)
        chunks.append(cur)
        for c in chunks:
            # name = last identifier at depth 0 before '=' / '[' / end
            d, name, arr = 0, None, 0
            for k, t in enumerate(c):
                if t.text == '=' and d == 0:
                    break
                if t.text in '([{' and t.kind == 'op':
                    if t.text == '[' and d == 0:
                        arr += 1
                    if t.text == '(' and d == 0 and k + 1 < len(c) and c[k + 1].text == '*':
                        # function pointer declarator (*name)
                        name = c[k + 2].text if k + 2 < len(c) else name
                    d += 1
                    continue
                if t.text in ')]}' and t.kind == 'op':
                    d -= 1
                    continue
                if d == 0 and t.kind == 'id' and t.text not in KEYWORDS and t.text not in TYPE_WORDS and not arr:
                    name = t.text
            if name:
                out.append((name, arr))
        return out

    def eff(self, n):
        key = (n.ti, n.tj)
        if key in self.eff_cache:
            return self.eff_cache[key]
        e = Effects()
        if n.kind == 'pp':
            e.pp = True
        elif n.kind == 'label':
            e.label = True
        elif n.kind == 'jump':
            e.jump = True
            e.merge(self.an.effects(self.toks[n.ti + 1:n.tj]))
        elif n.kind == 'decl':
            e.decl = True
            for name, _ in self.declared(n):
                e.swrites.add(name)
            # initialisers
            toks = self.toks[n.ti:n.tj]
            for k, t in enumerate(toks):
                if t.text == '=':
                    e.merge(self.an.effects(toks[k + 1:]))
                    break
        elif n.kind == 'expr':
            e.merge(self.an.effects(self.toks[n.ti:n.tj]))
        else:
            for name in ('cond', 'init', 'inc'):
                r = getattr(n, name, None)
                if r and r[1] >= r[0]:
                    e.merge(self.an.effects(self.toks[r[0]:r[1] + 1]))
            for c in n.children():
                e.merge(self.eff(c))
            if n.kind in ('for', 'while', 'do', 'switch'):
                pass
        self.eff_cache[key] = e
        return e

    def blocks(self):
        return [n for n in self.root.walk() if n.kind == 'block']

    def src(self, a, b):
        return self.text[a:b]

    def ntext(self, n):
        return self.text[n.start:n.end]

    def rtext(self, r):
        return self.text[self.toks[r[0]].start:self.toks[r[1]].end] if r[1] >= r[0] else ''

    def replace(self, edits):
        """Apply [(start, end, new)] (non-overlapping) to the text."""
        out = self.text
        for s, e, new in sorted(edits, key=lambda x: -x[0]):
            out = out[:s] + new + out[e:]
        return out

    def indent(self, n):
        ls = self.text.rfind('\n', 0, n.start) + 1
        return re.match(r'[ \t]*', self.text[ls:]).group()

    def first_stmt_index(self, blk):
        for k, s in enumerate(blk.stmts):
            if s.kind not in ('decl', 'pp'):
                return k
        return len(blk.stmts)


def free_jumps(n):
    """True if n contains break/continue that would bind to an enclosing loop."""
    def rec(x, in_loop, in_switch):
        if x.kind == 'jump' and ((x.word == 'continue' and not in_loop) or (x.word == 'break' and not in_loop and not in_switch)):
            return True
        if x.kind == 'jump' and x.word in ('goto', 'return'):
            return False
        il = in_loop or x.kind in ('for', 'while', 'do')
        isw = in_switch or x.kind == 'switch'
        return any(rec(c, il if x is not n or x.kind in ('for', 'while', 'do') else in_loop, isw) for c in x.children())
    return rec(n, False, False) if n.kind not in ('for', 'while', 'do', 'switch') else any(
        rec(c, n.kind != 'switch', n.kind == 'switch') for c in n.children())


def has_kind(n, kinds):
    return any(x.kind in kinds for x in n.walk())


def ends_with_jump(n):
    if n.kind == 'jump':
        return True
    if n.kind == 'block' and n.stmts:
        return ends_with_jump(n.stmts[-1])
    return False


_tmp_counter = [0]


def fresh(ctx, base='t'):
    used = set(re.findall(r'[A-Za-z_]\w*', ctx.text))
    k = 0
    while True:
        name = f'p{base}{k}'
        if name not in used:
            return name
        k += 1


def m_swap(ctx, rng):
    """Swap two adjacent independent statements (or move one over several)."""
    sites = []
    for b in ctx.blocks():
        st = b.stmts
        for k in range(len(st) - 1):
            a, c = st[k], st[k + 1]
            if a.kind in ('pp', 'label', 'empty') or c.kind in ('pp', 'label', 'empty'):
                continue
            if (a.kind == 'decl') != (c.kind == 'decl'):
                continue
            sites.append((b, k))
    rng.shuffle(sites)
    for b, k in sites:
        a, c = b.stmts[k], b.stmts[k + 1]
        if conflicts(ctx.eff(a), ctx.eff(c)):
            continue
        return ctx.replace([(a.start, a.end, ctx.ntext(c)), (c.start, c.end, ctx.ntext(a))]), f'swap {short(ctx, a)} <> {short(ctx, c)}'
    return None


def m_move(ctx, rng):
    """Move a statement up or down past several independent statements."""
    sites = []
    for b in ctx.blocks():
        st = b.stmts
        for k, a in enumerate(st):
            if a.kind in ('pp', 'label', 'empty'):
                continue
            sites.append((b, k))
    rng.shuffle(sites)
    for b, k in sites[:20]:
        st = b.stmts
        a = st[k]
        ea = ctx.eff(a)
        d = rng.choice((-1, 1))
        j = k
        reach = []
        while 0 <= j + d < len(st):
            o = st[j + d]
            if o.kind in ('pp', 'label') or (o.kind == 'decl') != (a.kind == 'decl') or conflicts(ea, ctx.eff(o)):
                break
            j += d
            reach.append(j)
        if len(reach) < 2:
            continue
        j = rng.choice(reach[1:])
        tgt = st[j]
        ind = ctx.indent(a)
        body = ctx.ntext(a)
        # remove a with its leading whitespace line, insert next to tgt
        ls = ctx.text.rfind('\n', 0, a.start) + 1
        le = a.end
        nl = '\r\n' if '\r\n' in ctx.text else '\n'
        if d > 0:
            ins = (tgt.end, tgt.end, nl + ind + body)
        else:
            ins = insert_before(ctx, tgt, body)
        if ctx.text[ls:a.start].strip() == '':
            # delete the whole line of a (from its line start to end incl. newline)
            le2 = a.end
            if ctx.text.startswith('\r\n', le2):
                le2 += 2
            elif ctx.text.startswith('\n', le2):
                le2 += 1
            rem = (ls, le2, '')
        else:
            rem = (a.start, a.end, '')
        return ctx.replace([rem, ins]), f'move {short(ctx, a)} {"down" if d > 0 else "up"} {abs(j - k)}'
    return None


def insert_before(ctx, n, txt):
    """Edit that inserts statement text txt right before statement n: on its own line when
    n starts its line, else directly in front of n (never outside n's block)."""
    ls = ctx.text.rfind('\n', 0, n.start) + 1
    if ctx.text[ls:n.start].strip() == '':
        nl = '\r\n' if '\r\n' in ctx.text else '\n'
        return (ls, ls, ctx.indent(n) + txt + nl)
    return (n.start, n.start, txt + ' ')


def short(ctx, n, w=40):
    s = re.sub(r'\s+', ' ', ctx.ntext(n))
    return s if len(s) <= w else s[:w - 3] + '...'


def m_barrier(ctx, rng):
    """Insert an empty `do ; while (0);` before a statement."""
    sites = []
    for b in ctx.blocks():
        f = ctx.first_stmt_index(b)
        for k in range(f, len(b.stmts) + 1):
            sites.append((b, k))
    if not sites:
        return None
    b, k = rng.choice(sites)
    nl = '\r\n' if '\r\n' in ctx.text else '\n'
    if k < len(b.stmts):
        s = b.stmts[k]
        return ctx.replace([insert_before(ctx, s, 'do ; while (0);')]), f'barrier before {short(ctx, s)}'
    if not b.stmts:
        return None
    s = b.stmts[-1]
    return ctx.replace([(s.end, s.end, nl + ctx.indent(s) + 'do ; while (0);')]), f'barrier after {short(ctx, s)}'


def m_unwrap(ctx, rng):
    """Remove a `do ; while (0);` barrier or unwrap `do { S } while (0);`."""
    sites = []
    for n in ctx.root.walk():
        if n.kind == 'do' and ctx.rtext(n.cond).strip() == '0':
            if n.body.kind == 'empty' or (n.body.kind == 'block' and not n.body.stmts):
                sites.append((n, None))
            elif not free_jumps(n.body) and not (n.body.kind == 'block' and any(s.kind == 'decl' for s in n.body.stmts)):
                sites.append((n, n.body))
    if not sites:
        return None
    n, body = rng.choice(sites)
    if body is None:
        ls = ctx.text.rfind('\n', 0, n.start) + 1
        le = n.end
        if ctx.text[ls:n.start].strip() == '' and (ctx.text[le:le + 2] == '\r\n' or ctx.text[le:le + 1] == '\n'):
            le += 2 if ctx.text[le:le + 2] == '\r\n' else 1
            return ctx.replace([(ls, le, '')]), 'remove barrier'
        return ctx.replace([(n.start, n.end, '')]), 'remove barrier'
    inner = ctx.ntext(body)
    if body.kind == 'block':
        inner = ctx.text[body.start + 1:body.end - 1].strip()
    return ctx.replace([(n.start, n.end, inner)]), f'unwrap do-while(0) {short(ctx, body)}'


def m_wrap(ctx, rng):
    """Wrap a statement (or two adjacent ones) in `do { ... } while (0);`."""
    sites = []
    for b in ctx.blocks():
        for k in range(ctx.first_stmt_index(b), len(b.stmts)):
            s = b.stmts[k]
            if s.kind in ('pp', 'label', 'empty') or free_jumps(s) or has_kind(s, ('label',)):
                continue
            sites.append((b, k))
    if not sites:
        return None
    b, k = rng.choice(sites)
    span = 1
    if k + 1 < len(b.stmts) and rng.random() < 0.3:
        s2 = b.stmts[k + 1]
        if s2.kind not in ('pp', 'label', 'empty', 'decl') and not free_jumps(s2) and not has_kind(s2, ('label',)):
            span = 2
    a, z = b.stmts[k], b.stmts[k + span - 1]
    txt = ctx.text[a.start:z.end]
    return ctx.replace([(a.start, z.end, 'do { ' + txt + ' } while (0);')]), f'wrap do-while(0) {short(ctx, a)}' + (' +1' if span == 2 else '')


def side_effect_free(ctx, ex):
    for x in ex.walk():
        if x.kind in ('assign', 'call', 'post', 'stmtexpr'):
            return False
        if x.kind == 'unary' and x.op in ('++', '--'):
            return False
    return True


def subexprs(ctx, ex, parent=None, ctx_flags=()):
    """Candidate subexpressions for a temporary: (Ex, conditional?)"""
    out = []

    def rec(x, cond, lval):
        if x.kind == 'assign':
            rec(x.kids[0], cond, True)
            rec(x.kids[1], cond, False)
            return
        if x.kind == 'bin' and x.op in ('&&', '||'):
            rec(x.kids[0], cond, False)
            rec(x.kids[1], True, False)
            return
        if x.kind == 'cond':
            rec(x.kids[0], cond, False)
            rec(x.kids[1], True, False)
            rec(x.kids[2], True, False)
            return
        if x.kind == 'unary' and x.op == '&':
            # &expr: the operand is an lvalue; only its index subexpressions are candidates
            rec(x.kids[0], cond, True)
            if not lval:
                out.append((x, cond))
            return
        if x.kind == 'call':
            for a in x.kids[1:]:
                rec(a, cond, False)
            return
        if x.kind in ('post',) or (x.kind == 'unary' and x.op in ('++', '--')):
            return
        if lval:
            if x.kind == 'index':
                rec(x.kids[0], cond, True)
                if x.kids[1].kind != 'num':
                    rec(x.kids[1], cond, False)
            elif x.kind == 'member':
                rec(x.kids[0], cond, x.op == '.')
            elif x.kind == 'unary' and x.op == '*':
                rec(x.kids[0], cond, False)
            elif x.kind == 'paren':
                rec(x.kids[0], cond, True)
            elif x.kind == 'cast':
                rec(x.kids[0], cond, True)
            return
        if x.kind not in ('str',):
            out.append((x, cond))
        if x.kind == 'index':
            # the base may be an array (no temporary of array type); constant subscripts stay
            rec(x.kids[0], cond, True)
            if x.kids[1].kind != 'num':
                rec(x.kids[1], cond, False)
            return
        for k in x.kids:
            if x.kind == 'member' and x.op == '.':
                rec(k, cond, True)
            else:
                rec(k, cond, False)
    rec(ex, False, False)
    return out


def m_temp(ctx, rng):
    """Introduce a temporary for a side-effect-free subexpression: { __typeof__(e) t = e; S }."""
    sites = []
    for n in ctx.root.walk():
        if n.kind == 'expr' or (n.kind == 'if') or (n.kind == 'jump' and n.word == 'return' and n.tj > n.ti + 1):
            sites.append(n)
    rng.shuffle(sites)
    ep = ExprParser(ctx.toks, ctx.unit.typedefs)
    for n in sites[:10]:
        if n.parent is None or n.parent.kind != 'block':
            continue
        try:
            if n.kind == 'expr':
                ex = ep.parse(n.ti, n.tj - 1)
            elif n.kind == 'if':
                ex = ep.parse(*n.cond)
            else:
                ex = ep.parse(n.ti + 1, n.tj - 1)
        except (ValueError, IndexError):
            continue
        # statement-level safety: at most one write (top level assignment), at most one call
        writes = [x for x in ex.walk() if x.kind == 'assign' or x.kind == 'post' or (x.kind == 'unary' and x.op in ('++', '--'))]
        calls = [x for x in ex.walk() if x.kind == 'call']
        if len(writes) > 1 or len(calls) > 1 or (n.kind == 'if' and writes):
            continue
        if writes and not (ex.kind == 'assign' and writes[0] is ex):
            continue
        if any(x.kind == 'comma' for x in ex.walk()):
            continue
        cands = [(x, c) for x, c in subexprs(ctx, ex) if not c and side_effect_free(ctx, x) and x is not ex
                 and x.kind not in ('str',) and not (x.kind == 'paren' and x.kids[0].kind in ('id', 'num'))]
        if writes:
            # sub must not read what the assignment writes (it is evaluated first either way, but be strict)
            lv = ctx.toks[ex.kids[0].ti:ex.kids[0].tj + 1]
            w = Effects()
            ctx.an.add_write(w, lv)
            ok = []
            for x, c in cands:
                if x.ti >= ex.kids[0].ti and x.tj <= ex.kids[0].tj and ex.kids[0].kind == 'id':
                    continue
                ok.append((x, c))
            cands = ok
        if calls:
            call = calls[0]
            # the sub must lie inside the call's arguments (evaluated before the call) and not contain it
            cands = [(x, c) for x, c in cands if x.ti > call.kids[0].tj and x.tj < call.tj]
        if not cands:
            continue
        x, _ = rng.choice(cands)
        if x.kind == 'cast' and rng.random() < 0.5:
            x = x.kids[0]
        etxt = ctx.text[ctx.toks[x.ti].start:ctx.toks[x.tj].end]
        if not etxt.strip() or '__typeof__' in etxt and False:
            continue
        name = fresh(ctx)
        if x.kind == 'num':
            typ = rng.choice(['int', 'short', 'unsigned short', 'unsigned char', 'unsigned int'])
            v = int(etxt.rstrip('uUlL'), 0) if re.fullmatch(r'(0[xX][0-9a-fA-F]+|\d+)[uUlL]*', etxt) else None
            if v is None:
                continue
            lim = {'int': (-2**31, 2**31 - 1), 'short': (-32768, 32767), 'unsigned short': (0, 65535),
                   'unsigned char': (0, 255), 'unsigned int': (0, 2**32 - 1)}[typ]
            if not lim[0] <= v <= lim[1]:
                typ = 'int'
            # a narrower typed copy of a constant changes the expression type only where the
            # value fits (checked); usual arithmetic conversions then give the same value
            if typ != 'int':
                typ = 'int'
        else:
            typ = '__typeof__(%s)' % etxt
        s0 = ctx.toks[n.ti].start
        new_stmt = ctx.text[s0:ctx.toks[x.ti].start] + name + ctx.text[ctx.toks[x.tj].end:n.end]
        decl = f'{typ} {name} = {etxt};'
        return ctx.replace([(n.start, n.end, '{ ' + decl + ' ' + new_stmt + ' }')]), f'temp {name} = {etxt[:30]}'
    return None


def m_untemp(ctx, rng):
    """Inline a block temporary used exactly once: { T t = e; S } -> S[t := (e)]."""
    sites = []
    for b in ctx.blocks():
        if b is ctx.root:
            continue
        st = b.stmts
        if len(st) == 2 and st[0].kind == 'decl' and st[1].kind in ('expr', 'if', 'jump'):
            names = ctx.declared(st[0])
            if len(names) != 1 or names[0][1]:
                continue
            name = names[0][0]
            toks = ctx.toks[st[0].ti:st[0].tj]
            eq = [k for k, t in enumerate(toks) if t.text == '=']
            if not eq:
                continue
            uses = [k for k in range(st[1].ti, st[1].tj + 1) if ctx.toks[k].text == name]
            if len(uses) != 1:
                continue
            e = ctx.an.effects(toks[eq[0] + 1:])
            if e.call or e.swrites or e.mwrites:
                continue
            e2 = ctx.eff(st[1])
            if e2.call and (e.mreads) :
                continue
            sites.append((b, name, toks[eq[0] + 1:], uses[0]))
    if not sites:
        return None
    b, name, etoks, use = rng.choice(sites)
    etxt = ctx.text[etoks[0].start:etoks[-1].end]
    st = b.stmts[1]
    u = ctx.toks[use]
    new = ctx.text[st.start:u.start] + '(' + etxt + ')' + ctx.text[u.end:st.end]
    return ctx.replace([(b.start, b.end, new)]), f'untemp {name}'


def m_decl_split(ctx, rng):
    """`T a = e;` at the end of a declaration list -> `T a;` + `a = e;` (or the reverse); split `T a, b;`."""
    sites = []
    for b in ctx.blocks():
        st = b.stmts
        f = ctx.first_stmt_index(b)
        for k in range(f):
            s = st[k]
            if s.kind != 'decl':
                continue
            names = ctx.declared(s)
            toks = ctx.toks[s.ti:s.tj]
            if any(t.text in ('static', 'extern', 'typedef') for t in toks):
                continue
            if len(names) > 1:
                sites.append(('split', b, k))
            elif any(t.text == '=' for t in toks) and k == f - 1 and not names[0][1] and not any(t.text == '{' for t in toks):
                sites.append(('init', b, k))
        if f > 0 and f < len(st) and st[f - 1].kind == 'decl' and st[f].kind == 'expr':
            s, a = st[f - 1], st[f]
            names = ctx.declared(s)
            toks = ctx.toks[a.ti:a.tj]
            if len(names) == 1 and not any(t.text == '=' for t in ctx.toks[s.ti:s.tj]) and len(toks) > 2 \
                    and toks[0].text == names[0][0] and toks[1].text == '=' and not names[0][1]:
                sites.append(('merge', b, f))
    if not sites:
        return None
    kind, b, k = rng.choice(sites)
    st = b.stmts
    nl = '\r\n' if '\r\n' in ctx.text else '\n'
    if kind == 'split':
        s = st[k]
        toks = ctx.toks[s.ti:s.tj]
        chunks, cur, d = [], [], 0
        for t in toks:
            if t.text in '([{' and t.kind == 'op':
                d += 1
            elif t.text in ')]}' and t.kind == 'op':
                d -= 1
            if t.text == ',' and d == 0:
                chunks.append(cur)
                cur = []
            else:
                cur.append(t)
        chunks.append(cur)
        first = chunks[0]
        name0 = ctx.declared(s)[0][0]
        idx = next(i for i, t in enumerate(first) if t.text == name0)
        while idx > 0 and first[idx - 1].text == '*':
            idx -= 1
        if idx == 0 or any(t.text == '(' for t in first[:idx]):
            return None
        base = ctx.text[first[0].start:first[idx - 1].end]
        parts = [ctx.text[first[idx].start:first[-1].end]] + [ctx.text[c[0].start:c[-1].end] for c in chunks[1:]]
        ind = ctx.indent(s)
        return ctx.replace([(s.start, s.end, (nl + ind).join(f'{base} {p};' for p in parts))]), f'split decl {short(ctx, s)}'
    if kind == 'init':
        s = st[k]
        toks = ctx.toks[s.ti:s.tj]
        eq = next(i for i, t in enumerate(toks) if t.text == '=')
        name = ctx.declared(s)[0][0]
        init = ctx.text[toks[eq + 1].start:toks[-1].end]
        f = ctx.first_stmt_index(b)
        decl = ctx.text[s.start:toks[eq - 1].end] + ';'
        edits = [(s.start, s.end, decl)]
        ind = ctx.indent(s)
        if f < len(st):
            t = st[f]
            edits.append(insert_before(ctx, t, f'{name} = {init};'))
        else:
            edits.append((s.end, s.end, nl + ind + f'{name} = {init};'))
        return ctx.replace(edits), f'split init {name}'
    # merge
    s, a = st[k - 1], st[k]
    toks = ctx.toks[a.ti:a.tj]
    init = ctx.text[toks[2].start:toks[-1].end]
    if any(t.text == '{' for t in toks):
        return None
    decl_toks = ctx.toks[s.ti:s.tj]
    if any(t.text == '[' for t in decl_toks):
        return None
    ls = ctx.text.rfind('\n', 0, a.start) + 1
    le = a.end + (2 if ctx.text[a.end:a.end + 2] == '\r\n' else 1 if ctx.text[a.end:a.end + 1] == '\n' else 0)
    rem = (ls, le, '') if not ctx.text[ls:a.start].strip() else (a.start, a.end, '')
    return ctx.replace([(s.end - 1, s.end - 1, ' = ' + init), rem]), f'merge init {toks[0].text}'


TYPE_GROUPS = [['int', 's32', 'long', 'signed int'], ['unsigned int', 'u32', 'unsigned long', 'unsigned'],
               ['short', 's16', 'short int', 'signed short'], ['unsigned short', 'u16'],
               ['unsigned char', 'u8'], ['signed char', 's8']]
TYPE_RISKY = {'int': ['unsigned int', 'short'], 's32': ['u32', 's16'], 'short': ['int', 'unsigned short'],
              's16': ['s32', 'u16'], 'u16': ['s32', 'u32', 's16'], 'unsigned short': ['int', 'unsigned int'],
              'u8': ['s32', 'u16', 'u32'], 'unsigned char': ['int', 'unsigned short'], 'u32': ['s32'],
              'unsigned int': ['int']}


def m_type(ctx, rng):
    """Change a local's base type to an equivalent spelling (or, flagged risky, another width/sign)."""
    sites = []
    for n in ctx.root.walk():
        if n.kind != 'decl':
            continue
        toks = ctx.toks[n.ti:n.tj + 1]
        if any(t.text in ('static', 'extern', 'struct', 'union', 'typedef', '[') for t in toks):
            continue
        # base type tokens = leading type words
        k = 0
        while k < len(toks) and toks[k].kind == 'id' and (toks[k].text in TYPE_WORDS or (k == 0 and toks[k].text in ctx.unit.typedefs)):
            k += 1
        if k == 0 or k >= len(toks):
            continue
        base = ' '.join(t.text for t in toks[:k])
        sites.append((n, toks[0].start, toks[k - 1].end, base, any(t.text == '*' for t in toks[k:k + 1])))
    rng.shuffle(sites)
    for n, s, e, base, ptr in sites:
        grp = next((g for g in TYPE_GROUPS if base in g), None)
        opts = [x for x in (grp or []) if x != base]
        risky = []
        if not ptr:
            risky = TYPE_RISKY.get(base, [])
        if not opts and not risky:
            continue
        if risky and (not opts or rng.random() < 0.5):
            new = rng.choice(risky)
            return ctx.replace([(s, e, new)]), f'RISKY type {base} -> {new} in {short(ctx, n)}'
        new = rng.choice(opts)
        return ctx.replace([(s, e, new)]), f'type {base} -> {new}'
    return None


def m_dup_if(ctx, rng):
    """Move the statement after an if/else into the end of both arms (cross-jump merge), or hoist it back."""
    sites = []
    for b in ctx.blocks():
        st = b.stmts
        for k in range(len(st) - 1):
            n, s = st[k], st[k + 1]
            if n.kind == 'if' and s.kind in ('expr', 'jump') and not (s.kind == 'jump' and s.word in ('break', 'continue')):
                if ends_with_jump(n.then) or (n.els is not None and ends_with_jump(n.els)):
                    continue
                sites.append(('down', b, k))
        for k in range(len(st)):
            n = st[k]
            if n.kind == 'if' and n.els is not None and n.then.kind == 'block' and n.els.kind == 'block' \
                    and n.then.stmts and n.els.stmts:
                a, c = n.then.stmts[-1], n.els.stmts[-1]
                if a.kind == 'expr' and re.sub(r'\s+', '', ctx.ntext(a)) == re.sub(r'\s+', '', ctx.ntext(c)):
                    sites.append(('up', b, k))
        for k in range(1, len(st)):
            n, s = st[k], st[k - 1]
            if n.kind == 'if' and s.kind == 'expr' and k - 1 >= ctx.first_stmt_index(b):
                ce = ctx.an.effects(ctx.toks[n.cond[0]:n.cond[1] + 1])
                if not conflicts(ctx.eff(s), ce):
                    sites.append(('before', b, k))
    if not sites:
        return None
    kind, b, k = rng.choice(sites)
    st = b.stmts
    nl = '\r\n' if '\r\n' in ctx.text else '\n'

    def append_to(arm, txt):
        if arm.kind == 'block':
            if arm.stmts:
                last = arm.stmts[-1]
                return (last.end, last.end, nl + ctx.indent(last) + txt)
            return (arm.end - 1, arm.end - 1, ' ' + txt + ' ')
        return (arm.start, arm.end, '{ ' + ctx.ntext(arm) + ' ' + txt + ' }')

    def prepend_to(arm, txt):
        if arm.kind == 'block':
            f = ctx.first_stmt_index(arm)
            if f < len(arm.stmts):
                t = arm.stmts[f]
                return (t.start, t.start, txt + nl + ctx.indent(t))
            return (arm.end - 1, arm.end - 1, ' ' + txt + ' ')
        return (arm.start, arm.end, '{ ' + txt + ' ' + ctx.ntext(arm) + ' }')

    if kind == 'down':
        n, s = st[k], st[k + 1]
        txt = ctx.ntext(s)
        edits = [append_to(n.then, txt)]
        if n.els is not None:
            edits.append(append_to(n.els, txt))
        else:
            edits.append((n.end, n.end, ' else { ' + txt + ' }'))
        ls = ctx.text.rfind('\n', 0, s.start) + 1
        le = s.end + (2 if ctx.text[s.end:s.end + 2] == '\r\n' else 1 if ctx.text[s.end:s.end + 1] == '\n' else 0)
        edits.append((ls, le, '') if not ctx.text[ls:s.start].strip() else (s.start, s.end, ''))
        return ctx.replace(edits), f'dup into if arms {short(ctx, s)}'
    if kind == 'before':
        n, s = st[k], st[k - 1]
        txt = ctx.ntext(s)
        edits = [prepend_to(n.then, txt)]
        if n.els is not None:
            edits.append(prepend_to(n.els, txt))
        else:
            edits.append((n.end, n.end, ' else { ' + txt + ' }'))
        ls = ctx.text.rfind('\n', 0, s.start) + 1
        le = s.end + (2 if ctx.text[s.end:s.end + 2] == '\r\n' else 1 if ctx.text[s.end:s.end + 1] == '\n' else 0)
        edits.append((ls, le, '') if not ctx.text[ls:s.start].strip() else (s.start, s.end, ''))
        return ctx.replace(edits), f'dup into if arm heads {short(ctx, s)}'
    n = st[k]
    a, c = n.then.stmts[-1], n.els.stmts[-1]
    txt = ctx.ntext(a)
    return ctx.replace([(a.start, a.end, ''), (c.start, c.end, ''), (n.end, n.end, nl + ctx.indent(n) + txt)]), f'hoist common tail {short(ctx, a)}'


def expr_sites(ctx):
    """(node, Ex) for every parseable expression in the body."""
    ep = ExprParser(ctx.toks, ctx.unit.typedefs)
    out = []
    for n in ctx.root.walk():
        ranges = []
        if n.kind == 'expr':
            ranges.append((n.ti, n.tj - 1))
        elif n.kind == 'jump' and n.word == 'return' and n.tj > n.ti + 1:
            ranges.append((n.ti + 1, n.tj - 1))
        for name in ('cond', 'init', 'inc'):
            r = getattr(n, name, None)
            if r and r[1] >= r[0]:
                ranges.append(r)
        if n.kind == 'decl':
            toks = ctx.toks
            for k in range(n.ti, n.tj):
                if toks[k].text == '=' and toks[k + 1].text != '{':
                    ranges.append((k + 1, n.tj - 1))
                    break
        for r in ranges:
            try:
                out.append((n, ep.parse(*r)))
            except (ValueError, IndexError):
                pass
    return out


def etext(ctx, x):
    return ctx.text[ctx.toks[x.ti].start:ctx.toks[x.tj].end]


def m_commute(ctx, rng):
    """Swap operands of a commutative operator (or flip a relational one)."""
    flips = {'<': '>', '>': '<', '<=': '>=', '>=': '<='}
    sites = []
    for n, ex in expr_sites(ctx):
        for x in ex.walk():
            if x.kind == 'bin' and x.op in ('+', '*', '&', '|', '^', '==', '!=') + tuple(flips):
                if side_effect_free(ctx, x.kids[0]) and side_effect_free(ctx, x.kids[1]):
                    sites.append(x)
    if not sites:
        return None
    x = rng.choice(sites)
    a, c = x.kids
    op = flips.get(x.op, x.op)
    opi = next(k for k in range(a.tj + 1, c.ti) if ctx.toks[k].text == x.op)
    return ctx.replace([(ctx.toks[a.ti].start, ctx.toks[c.tj].end, etext(ctx, c) + ' ' + op + ' ' + etext(ctx, a))]), f'commute {etext(ctx, x)[:30]}'


def m_compound(ctx, rng):
    """a op= b <-> a = a op (b)."""
    sites = []
    for n, ex in expr_sites(ctx):
        for x in ex.walk():
            if x.kind == 'assign' and x.op != '=' and side_effect_free(ctx, x.kids[0]):
                sites.append(('expand', x))
            elif x.kind == 'assign' and x.op == '=' and x.kids[1].kind == 'bin' and x.kids[1].op in ('+', '-', '*', '/', '%', '&', '|', '^', '<<', '>>'):
                l = re.sub(r'\s+', '', etext(ctx, x.kids[0]))
                if re.sub(r'\s+', '', etext(ctx, x.kids[1].kids[0])) == l and side_effect_free(ctx, x.kids[0]):
                    sites.append(('fold', x))
    if not sites:
        return None
    kind, x = rng.choice(sites)
    if kind == 'expand':
        lv = etext(ctx, x.kids[0])
        r = etext(ctx, x.kids[1])
        return ctx.replace([(ctx.toks[x.ti].start, ctx.toks[x.tj].end, f'{lv} = {lv} {x.op[:-1]} ({r})')]), f'expand {x.op}'
    lv = etext(ctx, x.kids[0])
    b = x.kids[1]
    r = etext(ctx, b.kids[1])
    return ctx.replace([(ctx.toks[x.ti].start, ctx.toks[x.tj].end, f'{lv} {b.op}= {r}')]), f'fold {b.op}='


def m_shift(ctx, rng):
    """x << n <-> x * 2^n (literal n)."""
    sites = []
    for n, ex in expr_sites(ctx):
        for x in ex.walk():
            if x.kind == 'bin' and x.op == '<<' and x.kids[1].kind == 'num':
                v = int(etext(ctx, x.kids[1]).rstrip('uUlL'), 0)
                if 0 < v < 31:
                    sites.append((x, 'mul', v))
            if x.kind == 'bin' and x.op == '*' and x.kids[1].kind == 'num':
                try:
                    v = int(etext(ctx, x.kids[1]).rstrip('uUlL'), 0)
                except ValueError:
                    continue
                if v > 1 and v & (v - 1) == 0:
                    sites.append((x, 'shl', v.bit_length() - 1))
    if not sites:
        return None
    x, kind, v = rng.choice(sites)
    a = etext(ctx, x.kids[0])
    if x.kids[0].kind not in ('id', 'num', 'paren', 'index', 'member', 'call', 'cast', 'unary'):
        a = '(' + a + ')'
    new = f'({a} * {1 << v})' if kind == 'mul' else f'({a} << {v})'
    return ctx.replace([(ctx.toks[x.ti].start, ctx.toks[x.tj].end, new)]), f'{kind} {etext(ctx, x)[:30]}'


def m_cond(ctx, rng):
    """if (c) A else B -> if (!(c)) B else A; if (!x) <-> if (x == 0); i < N <-> i <= N-1."""
    sites = []
    for n in ctx.root.walk():
        if n.kind == 'if' and n.els is not None and not (n.els.kind == 'if'):
            sites.append(('swap', n, None))
    for n, ex in expr_sites(ctx):
        for x in ex.walk():
            if x.kind == 'unary' and x.op == '!':
                sites.append(('not', n, x))
            if x.kind == 'bin' and x.op in ('<', '<=', '>', '>=') and x.kids[1].kind == 'num':
                sites.append(('bound', n, x))
            if x.kind == 'bin' and x.op in ('==', '!=') and x.kids[1].kind == 'num' and etext(ctx, x.kids[1]) == '0':
                sites.append(('eq0', n, x))
    if not sites:
        return None
    kind, n, x = rng.choice(sites)
    if kind == 'swap':
        c = ctx.rtext(n.cond)
        then, els = ctx.ntext(n.then), ctx.ntext(n.els)
        return ctx.replace([(ctx.toks[n.cond[0]].start, ctx.toks[n.cond[1]].end, f'!({c})'),
                            (n.then.start, n.then.end, els), (n.els.start, n.els.end, then)]), 'swap if arms'
    if kind == 'not':
        inner = etext(ctx, x.kids[0])
        return ctx.replace([(ctx.toks[x.ti].start, ctx.toks[x.tj].end, f'({inner} == 0)')]), 'not -> == 0'
    if kind == 'eq0':
        a = etext(ctx, x.kids[0])
        new = f'!({a})' if x.op == '==' else f'({a})'
        return ctx.replace([(ctx.toks[x.ti].start, ctx.toks[x.tj].end, new)]), f'{x.op} 0 -> {new[:20]}'
    t = etext(ctx, x.kids[1])
    try:
        v = int(t.rstrip('uUlL'), 0)
    except ValueError:
        return None
    if t.lower().endswith('u'):
        return None
    a = etext(ctx, x.kids[0])
    new = {'<': f'{a} <= {v - 1}', '<=': f'{a} < {v + 1}', '>': f'{a} >= {v + 1}', '>=': f'{a} > {v - 1}'}[x.op]
    return ctx.replace([(ctx.toks[x.ti].start, ctx.toks[x.tj].end, new)]), f'bound {etext(ctx, x)} -> {new}'


def m_loop(ctx, rng):
    """for <-> while, for -> guarded do-while (bodies without continue)."""
    sites = []
    for n in ctx.root.walk():
        if n.kind == 'for' and not has_kind_jump(n.body, 'continue') and n.parent is not None and n.parent.kind == 'block':
            sites.append(n)
        if n.kind == 'while' and n.parent is not None and n.parent.kind == 'block':
            sites.append(n)
    if not sites:
        return None
    n = rng.choice(sites)
    body = ctx.ntext(n.body)
    inner = ctx.text[n.body.start + 1:n.body.end - 1] if n.body.kind == 'block' else body
    if n.kind == 'while':
        c = ctx.rtext(n.cond)
        if rng.random() < 0.5:
            return ctx.replace([(n.start, n.end, f'for (; {c}; ) {body}')]), 'while -> for'
        return ctx.replace([(n.start, n.end, f'if ({c}) do {{ {inner} }} while ({c});')]), 'while -> if do-while'
    init, c, inc = ctx.rtext(n.init), ctx.rtext(n.cond), ctx.rtext(n.inc)
    if not c:
        return None
    r = rng.random()
    pre = f'{init}; ' if init else ''
    post = f' {inc};' if inc else ''
    if r < 0.5:
        return ctx.replace([(n.start, n.end, f'{{ {pre}while ({c}) {{ {inner}{post} }} }}')]), 'for -> while'
    return ctx.replace([(n.start, n.end, f'{{ {pre}if ({c}) do {{ {inner}{post} }} while ({c}); }}')]), 'for -> if do-while'


def has_kind_jump(n, word):
    def rec(x, inloop):
        if x.kind == 'jump' and x.word == word and not inloop:
            return True
        return any(rec(c, inloop or x.kind in ('for', 'while', 'do')) for c in x.children())
    return any(rec(c, False) for c in n.children()) or (n.kind == 'jump' and n.word == word)


def m_decl_move(ctx, rng):
    """Move an uninitialised declaration into the innermost block holding all its uses
    (when the block writes it before any read), or out to the function block."""
    sites = []
    for b in ctx.blocks():
        for s in b.stmts:
            if s.kind != 'decl':
                continue
            toks = ctx.toks[s.ti:s.tj]
            if any(t.text in ('=', 'static', 'extern', 'typedef') for t in toks):
                continue
            names = ctx.declared(s)
            if len(names) != 1:
                continue
            sites.append((b, s, names[0][0]))
    rng.shuffle(sites)
    nl = '\r\n' if '\r\n' in ctx.text else '\n'
    for b, s, name in sites:
        uses = [k for k in range(b.ti, b.tj) if ctx.toks[k].text == name and not (s.ti <= k <= s.tj)]
        if not uses:
            continue
        # candidate inner blocks: blocks inside b containing all uses
        inner = [x for x in b.walk() if x.kind == 'block' and x is not b and x.ti < min(uses) and x.tj > max(uses)]
        if inner and rng.random() < 0.8:
            x = max(inner, key=lambda q: q.ti)
            f = ctx.first_stmt_index(x)
            if f >= len(x.stmts):
                continue
            first = min(uses)
            st = x.stmts[f]
            # first use must be a plain top-level assignment `name = ...` of the block
            stmt_of_first = next((q for q in x.stmts if q.ti <= first <= q.tj), None)
            if stmt_of_first is None or stmt_of_first.kind != 'expr' or ctx.toks[stmt_of_first.ti].text != name \
                    or ctx.toks[stmt_of_first.ti + 1].text != '=':
                continue
            if any(ctx.toks[k].text == '&' for k in [u - 1 for u in uses]):
                continue
            # no other use of name in the stmt before the assignment completes
            decl = ctx.ntext(s)
            ls = ctx.text.rfind('\n', 0, s.start) + 1
            le = s.end + (2 if ctx.text[s.end:s.end + 2] == '\r\n' else 1 if ctx.text[s.end:s.end + 1] == '\n' else 0)
            rem = (ls, le, '') if not ctx.text[ls:s.start].strip() else (s.start, s.end, '')
            tl = ctx.text.rfind('\n', 0, x.stmts[0].start) + 1
            if ctx.text[tl:x.stmts[0].start].strip():
                ins = (x.stmts[0].start, x.stmts[0].start, decl + ' ')
            else:
                ins = (tl, tl, ctx.indent(x.stmts[0]) + decl + nl)
            return ctx.replace([rem, ins]), f'decl {name} into block'
    return None


def m_block_copy(ctx, rng):
    """Add a block-local copy of a local scalar that a block only reads."""
    blocks = [b for b in ctx.blocks() if b is not ctx.root]
    rng.shuffle(blocks)
    addr = set()
    for k in range(1, len(ctx.toks)):
        if ctx.toks[k - 1].text == '&' and ctx.toks[k].kind == 'id':
            addr.add(ctx.toks[k].text)
    for b in blocks[:10]:
        e = ctx.eff(b)
        names = [v for v in e.sreads - e.swrites if v not in ctx.arrays and v not in addr]
        if not names:
            continue
        v = rng.choice(sorted(names))
        # v must not be written anywhere in the block (eff already) nor be a declared name inside it
        if any(v == nm for x in b.walk() if x.kind == 'decl' for nm, _ in ctx.declared(x)):
            continue
        new = fresh(ctx, 'c')
        inner = ctx.text[b.start + 1:b.end - 1]
        toks = [t for t in ctx.toks[b.ti + 1:b.tj] if t.text == v]
        member = {k for k in range(b.ti + 1, b.tj) if ctx.toks[k - 1].text in ('.', '->')}
        rel = []
        for k in range(b.ti + 1, b.tj):
            t = ctx.toks[k]
            if t.text == v and k not in member:
                rel.append((t.start - b.start - 1, t.end - b.start - 1))
        for s0, e0 in reversed(rel):
            inner = inner[:s0] + new + inner[e0:]
        return ctx.replace([(b.start, b.end, '{ __typeof__(%s) %s = %s;%s}' % (v, new, v, inner))]), f'block copy {v}'
    return None


def m_expand_macro(ctx, rng):
    """Replace a statement `MACRO(args);` (comma-list macro) by one statement per element."""
    sites = []
    for n in ctx.root.walk():
        if n.kind == 'expr' and ctx.toks[n.ti].kind == 'id' and ctx.toks[n.ti + 1].text == '(' \
                and match_close(ctx.toks, n.ti + 1) == n.tj - 1:
            mac = ctx.unit.macros.get(ctx.toks[n.ti].text)
            if mac and mac[0] is not None and n.parent is not None and n.parent.kind == 'block':
                sites.append(n)
    if not sites:
        return None
    n = rng.choice(sites)
    params, body = ctx.unit.macros[ctx.toks[n.ti].text]
    k = n.ti + 1
    c = n.tj - 1
    args, cur, d = [], [], 0
    for q in range(k + 1, c):
        y = ctx.toks[q]
        if y.text in '([{' and y.kind == 'op':
            d += 1
        elif y.text in ')]}' and y.kind == 'op':
            d -= 1
        if y.text == ',' and d == 0:
            args.append(cur)
            cur = []
        else:
            cur.append(y)
    args.append(cur)
    if len(args) != len(params):
        return None
    sub = {p: ctx.text[a[0].start:a[-1].end] if a else '' for p, a in zip(params, args)}
    # args used more than once must be side-effect free (they are, as expressions repeated by the macro)
    btoks = tokenize(body)
    out = ''
    last = 0
    for y in btoks:
        if y.kind == 'id' and y.text in sub:
            out += body[last:y.start] + sub[y.text]
            last = y.end
    out += body[last:]
    out = out.strip()
    # strip one outer pair of parens and split top-level commas
    t2 = tokenize(out)
    if t2 and t2[0].text == '(' and match_close(t2, 0) == len(t2) - 1:
        out = out[t2[0].end:t2[-1].start]
        t2 = tokenize(out)
    parts, last, d = [], 0, 0
    for y in t2:
        if y.text in '([{' and y.kind == 'op':
            d += 1
        elif y.text in ')]}' and y.kind == 'op':
            d -= 1
        elif y.text == ',' and d == 0:
            parts.append(out[last:y.start].strip())
            last = y.end
    parts.append(out[last:].strip())
    if len(parts) < 2:
        return None
    nl = '\r\n' if '\r\n' in ctx.text else '\n'
    ind = ctx.indent(n)
    return ctx.replace([(n.start, n.end, (nl + ind).join(p + ';' for p in parts))]), f'expand macro {ctx.toks[n.ti].text}'


MUTATIONS = [(m_swap, 30), (m_move, 10), (m_barrier, 6), (m_unwrap, 5), (m_wrap, 5), (m_temp, 10), (m_untemp, 4),
             (m_decl_split, 5), (m_type, 3), (m_dup_if, 5), (m_commute, 6), (m_compound, 3), (m_shift, 2),
             (m_cond, 4), (m_loop, 2), (m_decl_move, 4), (m_block_copy, 4), (m_expand_macro, 3)]


def focus_span(unit, text, focus):
    """(start, end) offsets of the focus region: from the line holding focus[0] to the end
    of the line holding focus[1] (searched inside the target function)."""
    fs, fe = unit.func_span(text)
    a = text.find(focus[0], fs, fe)
    b = text.find(focus[1], max(a, fs), fe)
    if a < 0 or b < 0:
        return fs, fe
    a = text.rfind('\n', 0, a) + 1
    e = text.find('\n', b)
    return a, (fe if e < 0 else e)


def mutate(unit, text, rng, aliases, weights=None, focus=None):
    try:
        ctx = Ctx(unit, text, aliases)
    except Exception:
        return None
    muts = weights or MUTATIONS
    region = focus_span(unit, text, focus) if focus else None
    for _ in range(60 if focus else 8):
        f = rng.choices([m for m, _ in muts], [w for _, w in muts])[0]
        try:
            r = f(ctx, rng)
        except (ValueError, IndexError, KeyError, StopIteration, AttributeError, TypeError):
            r = None
        if r and region:
            new = r[0]
            p = 0
            n = min(len(text), len(new))
            while p < n and text[p] == new[p]:
                p += 1
            q = 0
            while q < n - p and text[-1 - q] == new[-1 - q]:
                q += 1
            if p < region[0] or len(text) - q > region[1]:
                continue
        if r and r[0] != text:
            # the result must still parse
            try:
                Ctx(unit, r[0], aliases)
            except Exception:
                continue
            return r
    return None


# --------------------------------------------------------------------------- scoring


class Scorer:
    def __init__(self, source, va, work, size_penalty):
        self.source = source
        self.rel = source.relative_to(ROOT).as_posix()
        manifest = {u['source']: u for u in json.loads(mu.MANIFEST.read_text(encoding='utf-8'))['units']}
        self.policy = manifest[self.rel]
        if source.suffix == '.cpp':
            raise SystemExit('C units only')
        template = json.loads(mu.TEMPLATE.read_text(encoding='utf-8'))
        self.funcs = sorted((v.upper(), f) for v, f in template['functions'].items() if f.get('source') == self.rel)
        words = {r['va'].upper(): int(r['words']) for name in ('extra_funcs.tsv', 'ida_funcs.tsv')
                 for r in csv.DictReader((ROOT / 'config/US' / name).open(), delimiter='\t')}
        self.va = va.upper()
        self.symbol = next(f['symbol'] for v, f in self.funcs if v == self.va)
        self.slot = 4 * words[self.va]
        retail, load = mu.retail_image()
        a = int(self.va, 16)
        self.retail = retail[a - load + 0x800: a - load + 0x800 + self.slot]
        self.syms = symbol_map()
        self.work = work
        self.size_penalty = size_penalty
        self.lock = threading.Lock()
        self.cache = {}
        self.stats = dict(compiles=0, cache_hits=0, errors=0)

    def score(self, text, wid, keep=False):
        h = hashlib.sha256(text.encode()).hexdigest()
        with self.lock:
            if h in self.cache:
                self.stats['cache_hits'] += 1
                return self.cache[h]
        r = self._score(text, wid, keep)
        with self.lock:
            self.cache[h] = r
            self.stats['compiles'] += 1
            if r[0] >= FAIL:
                self.stats['errors'] += 1
        return r

    def _score(self, text, wid, keep):
        d = self.work / f'w{wid}'
        if d.exists():
            shutil.rmtree(d, ignore_errors=True)
        comp = d / 'compile'
        comp.mkdir(parents=True)
        src = d / self.source.name
        src.write_text(text, encoding='utf-8', newline='')
        p = self.policy
        try:
            preprocess(src, comp / 'source.i')
            # PsyQ's cc1 in DOSBox still writes OUT.S after front-end errors (and its messages
            # are not captured), so check the candidate with the native GCC 2.7.2 front end first.
            chk = subprocess.run([str(CC1), '-quiet', '-fsyntax-only', 'source.i'], cwd=comp,
                                 capture_output=True, stdin=subprocess.DEVNULL, timeout=60)
            if chk.returncode:
                raise SystemExit('syntax: ' + chk.stderr.decode(errors='replace')[-300:])
            obj, missing = mu.compile_candidate(p.get('compiler'), comp, p['delay_slots'], None, bool(p.get('extern_gp')),
                                                bool(p.get('fvolatile')), False, p.get('gp_size', 8), p.get('asm_post'), p.get('opt'))
            cand = self.link(obj, d)
        except BaseException as e:  # SystemExit from the compile helpers
            if isinstance(e, KeyboardInterrupt):
                raise
            return (FAIL, None, str(e)[-300:])
        return self.compare(cand) + (cand,)

    def compare(self, cand):
        ret = self.retail
        if len(cand) == len(ret) or (cand == ret[:len(cand)] and not any(ret[len(cand):])):
            n = min(len(cand), len(ret))
            w = sum(1 for i in range(0, n, 4) if cand[i:i + 4] != ret[i:i + 4])
            if len(cand) != len(ret) and w == 0:
                return (0, None)
            return (w + abs(len(cand) - len(ret)) // 4 + (self.size_penalty if len(cand) != len(ret) else 0), None)
        a = [cand[i:i + 4] for i in range(0, len(cand), 4)]
        b = [ret[i:i + 4] for i in range(0, len(ret), 4)]
        sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
        w = sum(max(i2 - i1, j2 - j1) for op, i1, i2, j1, j2 in sm.get_opcodes() if op != 'equal')
        return (w + self.size_penalty, None)

    def link(self, obj, d):
        exports, imports = mu.exports_imports(obj)
        by_name = {e['name']: e for e in exports}
        syms = self.syms
        bases = {}
        for va, f in self.funcs:
            e = by_name.get(f['symbol'])
            if not e:
                raise SystemExit(f'{f["symbol"]} not exported')
            bases.setdefault(e['section'], int(va, 16) - e['offset'])
        for e in exports:
            a = address_for(e['name'], syms)
            if a is None:
                m = SECTION_HEX.match(e['section'])
                a = int(m.group(1), 16) + e['offset'] if m else None
            if a is not None:
                bases.setdefault(e['section'], a - e['offset'])
        for sec in read_elf(obj)['sections']:
            m = SECTION_HEX.match(sec['name'])
            if m and sec['flags'] & 2 and sec['size'] and sec['name'] not in bases:
                bases[sec['name']] = int(m.group(1), 16)
        unresolved = [n for n in imports if address_for(n, syms) is None]
        if unresolved:
            raise SystemExit(f'unresolved imports: {unresolved}')
        script = d / 'unit.ld'
        placed = ' '.join('.s%d 0x%08X : { *(%s) }' % (n, b, s) for n, (s, b) in enumerate(sorted(bases.items())))
        defs = ' '.join('%s = 0x%08X;' % (n, address_for(n, syms)) for n in imports if n != '_gp') + ' _gp = 0x%08X;' % syms['_gp']
        script.write_text('SECTIONS { ' + placed + ' /DISCARD/ : { *(.reginfo .pdr .comment .gnu.attributes .mdebug*) } }\n' + defs + '\n', encoding='ascii')
        elfpath = d / 'unit.elf'
        q = subprocess.run([str(MIPS / 'mipsel-none-elf-ld.exe'), '-EL', '-T', str(script), '-o', str(elfpath), str(obj)],
                           capture_output=True, stdin=subprocess.DEVNULL)
        if q.returncode:
            raise SystemExit('link failed: ' + (q.stdout + q.stderr).decode(errors='replace')[-500:])
        elf = read_elf(elfpath)
        sym = global_symbol(elf, self.symbol)
        sec = elf['sections'][sym['section_index']]
        start = sym['address'] - sec['address']
        return sec['data'][start:start + sym['size']]

    def disasm(self, blob, wid):
        path = self.work / f'w{wid}' / 'dis.bin'
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(blob)
        out = subprocess.run([str(mu.OBJDUMP), '-D', '-b', 'binary', '-m', 'mips:3000', '-EL',
                              f'--adjust-vma=0x{self.va}', str(path)], capture_output=True, text=True,
                             stdin=subprocess.DEVNULL).stdout
        return [l.strip() for l in out.splitlines() if l.strip()[:2] == '80' and ':' in l]

    def insn_diff(self, cand, wid, limit=40):
        a = self.disasm(self.retail, wid)
        b = self.disasm(cand, wid)
        out = []
        for x, y in zip(a + [''] * max(0, len(b) - len(a)), b + [''] * max(0, len(a) - len(b))):
            if x.split(':', 1)[-1:] != y.split(':', 1)[-1:]:
                out.append(f'   R {x[:58]:58} | C {y[:58]}')
                if len(out) >= limit:
                    break
        return out


# --------------------------------------------------------------------------- search


def func_text(unit, text):
    s, e = unit.func_span(text)
    return text[s:e]


def splice(unit, text, ftext):
    s, e = unit.func_span(text)
    nl = '\r\n' if '\r\n' in text else '\n'
    ftext = ftext.replace('\r\n', '\n').replace('\n', nl)
    return text[:s] + ftext + text[e:]


class Search:
    def __init__(self, args):
        self.args = args
        self.source = (ROOT / args.source).resolve()
        self.orig = self.source.read_text(encoding='utf-8', newline='')
        self.unit = Unit(self.orig, args.va.upper(), [ROOT / 'include', ROOT / 'src', ROOT / 'toolchain/psyq330/include'])
        self.out = Path(args.out)
        self.out.mkdir(parents=True, exist_ok=True)
        self.name = self.unit.name
        self.log_path = self.out / f'{self.name}.log'
        self.state_path = self.out / f'{self.name}.state.json'
        self.scorer = Scorer(self.source, args.va, self.out / f'{self.name}.work', args.size_penalty)
        self.aliases = dict(a.split('=', 1) for a in args.alias)
        self.focus = tuple(args.focus) if args.focus else None
        self.weights = None
        if args.weights:
            w = dict(MUTATIONS)
            names = {m.__name__[2:]: m for m, _ in MUTATIONS}
            for item in args.weights.split(','):
                k, v = item.split('=')
                w[names[k]] = float(v)
            self.weights = [(m, x) for m, x in w.items() if x > 0]
        self.hist = {}
        self.lock = threading.Lock()
        self.best = None
        self.pool = []          # (score, text, chain)
        self.matches = []
        self.iters = 0
        self.improvements = 0
        self.mut_stats = {}
        self.start = time.time()
        self.prev_elapsed = 0.0
        self.base_text = func_text(self.unit, self.orig)

    def log(self, msg):
        with open(self.log_path, 'a', encoding='utf-8') as f:
            f.write(msg + '\n')

    def diff(self, text):
        a = self.base_text.replace('\r\n', '\n').splitlines()
        b = func_text(self.unit, text).replace('\r\n', '\n').splitlines()
        return '\n'.join(difflib.unified_diff(a, b, 'start', 'candidate', lineterm='', n=1))

    def save_state(self):
        st = dict(source=self.args.source, va=self.args.va, function=self.name,
                  best_score=self.best[0] if self.best else None,
                  elapsed_s=round(self.prev_elapsed + time.time() - self.start),
                  iterations=self.iters, improvements=self.improvements, matches=len(self.matches),
                  scorer=self.scorer.stats, mutations=self.mut_stats,
                  score_histogram={str(k): v for k, v in sorted(self.hist.items())[:40]},
                  pool=[dict(score=s, chain=c) for s, _, c in self.pool])
        for k, (s, t, c) in enumerate(self.pool):
            (self.out / f'{self.name}.pool{k}.c').write_text(func_text(self.unit, t), encoding='utf-8')
        self.state_path.write_text(json.dumps(st, indent=1) + '\n', encoding='utf-8')

    def consider(self, score, text, chain, wid, cand):
        with self.lock:
            better = self.best is None or score < self.best[0]
            if score < FAIL:
                if not any(t == text for _, t, _ in self.pool):
                    self.pool.append((score, text, chain))
                    self.pool.sort(key=lambda x: x[0])
                    del self.pool[self.args.pool:]
            if better:
                self.best = (score, text, chain)
                self.improvements += 1
                (self.out / f'{self.name}.best.c').write_text(func_text(self.unit, text), encoding='utf-8')
            if score == 0 and not any(t == text for t in self.matches):
                self.matches.append(text)
                p = self.out / f'{self.name}.match_{len(self.matches)}.c'
                p.write_text(func_text(self.unit, text), encoding='utf-8')
                self.log(f'=== MATCH {len(self.matches)} -> {p.name} (REVIEW FOR EQUIVALENCE) chain: {" | ".join(chain)}')
            if better:
                el = self.prev_elapsed + time.time() - self.start
                self.log(f'--- {time.strftime("%H:%M:%S")} t={el:.0f}s iter={self.iters} score={score} chain: {" | ".join(chain)}')
                self.log(self.diff(text))
                if cand:
                    self.log('\n'.join(self.scorer.insn_diff(cand, wid)))
                self.save_state()

    def seeds(self):
        out = [(self.orig, ['start (src)'])]
        for s in self.args.seed:
            ft = Path(s).read_text(encoding='utf-8', errors='replace')
            out.append((splice(self.unit, self.orig, ft), [f'seed {Path(s).name}']))
        if self.args.resume and self.state_path.exists():
            st = json.loads(self.state_path.read_text())
            self.prev_elapsed = st.get('elapsed_s', 0)
            self.iters = st.get('iterations', 0)
            for k, p in enumerate(st.get('pool', [])):
                f = self.out / f'{self.name}.pool{k}.c'
                if f.exists():
                    out.append((splice(self.unit, self.orig, f.read_text(encoding='utf-8')), p['chain']))
            for k in range(1, 100):
                f = self.out / f'{self.name}.match_{k}.c'
                if not f.exists():
                    break
                self.matches.append(splice(self.unit, self.orig, f.read_text(encoding='utf-8')))
        return out

    def worker(self, wid, deadline):
        rng = random.Random(self.args.rng_seed * 1000 + wid + int(time.time()))
        with self.lock:
            cur = self.pool[wid % len(self.pool)] if self.pool else self.best
        cur_score, cur_text, cur_chain = cur
        stale = 0
        while time.time() < deadline:
            if len(self.matches) >= self.args.stop_after:
                return
            # occasionally restart from a pool member
            if stale > self.args.restart or rng.random() < 0.02:
                with self.lock:
                    cur_score, cur_text, cur_chain = rng.choice(self.pool[:max(1, min(len(self.pool), 5))])
                stale = 0
            text, chain = cur_text, list(cur_chain)
            nmut = 1
            while rng.random() < 0.35 and nmut < 4:
                nmut += 1
            names = []
            ok = True
            for _ in range(nmut):
                r = mutate(self.unit, text, rng, self.aliases, self.weights, self.focus)
                if r is None:
                    ok = False
                    break
                text = r[0]
                names.append(r[1])
            if not ok or text == cur_text:
                stale += 1
                continue
            score, _, cand = self.scorer.score(text, wid)
            with self.lock:
                self.iters += 1
                self.hist[score] = self.hist.get(score, 0) + 1
                for nm in names:
                    k = nm.split()[0]
                    s = self.mut_stats.setdefault(k, [0, 0, 0])
                    s[0] += 1
                    if score < cur_score:
                        s[1] += 1
                    if score >= FAIL:
                        s[2] += 1
            chain2 = (chain + names)[-30:]
            self.consider(score, text, chain2, wid, cand if isinstance(cand, bytes) else None)
            T = self.args.temperature
            if score <= cur_score or (score < FAIL and rng.random() < math.exp(-(score - cur_score) / max(T, 1e-6))):
                if score < cur_score:
                    stale = 0
                else:
                    stale += 1
                cur_score, cur_text, cur_chain = score, text, chain2
            else:
                stale += 1
            if self.iters % 25 == 0:
                with self.lock:
                    self.save_state()

    def run(self):
        seeds = self.seeds()
        self.log(f'=== run {time.strftime("%Y-%m-%d %H:%M:%S")} {self.args.source} {self.args.va} jobs={self.args.jobs} '
                 f'minutes={self.args.minutes} seeds={len(seeds)}')
        for text, chain in seeds:
            score, _, cand = self.scorer.score(text, 0)
            self.log(f'seed {" ".join(chain)}: {score}' + (f' ({cand})' if isinstance(cand, str) else ''))
            if score < FAIL:
                self.consider(score, text, chain, 0, cand if isinstance(cand, bytes) else None)
        if not self.pool:
            raise SystemExit('no seed compiles')
        deadline = time.time() + 60 * self.args.minutes
        threads = [threading.Thread(target=self.worker, args=(w, deadline), daemon=True) for w in range(self.args.jobs)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        self.save_state()
        st = json.loads(self.state_path.read_text())
        self.log(f'=== end best={self.best[0]} iterations={self.iters} compiles={self.scorer.stats} matches={len(self.matches)}')
        print(json.dumps(dict(function=self.name, best=self.best[0], iterations=self.iters, improvements=self.improvements,
                              matches=len(self.matches), scorer=self.scorer.stats, elapsed_s=st['elapsed_s'])))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--source', required=True, help='maintained unit (manifest path), e.g. src/semantic/font_init.c')
    ap.add_argument('--va', required=True, help='target function address (its .text_<VA> section)')
    ap.add_argument('--out', required=True, help='output directory (logs, best/pool/match files, state, worker dirs)')
    ap.add_argument('--jobs', type=int, default=3, help='parallel compile workers')
    ap.add_argument('--minutes', type=float, default=60)
    ap.add_argument('--seed', action='append', default=[], help='file with a full replacement definition of the function')
    ap.add_argument('--resume', action='store_true', help='continue from <out>/<func>.state.json and its pool')
    ap.add_argument('--alias', action='append', default=[], help='LOCAL=OBJECT: pointer local LOCAL only points into OBJECT')
    ap.add_argument('--focus', nargs=2, metavar=('FROM', 'TO'),
                    help='only mutate the lines from the one containing FROM to the one containing TO')
    ap.add_argument('--weights', help='mutation weights, e.g. swap=40,temp=20,type=0 (names without m_)')
    ap.add_argument('--size-penalty', type=int, default=100)
    ap.add_argument('--temperature', type=float, default=1.5)
    ap.add_argument('--pool', type=int, default=12)
    ap.add_argument('--restart', type=int, default=120, help='restart from the pool after this many non-improving steps')
    ap.add_argument('--stop-after', type=int, default=3, help='stop after this many 0-word candidates')
    ap.add_argument('--rng-seed', type=int, default=1)
    ap.add_argument('--eval', help='score this replacement definition once and print the instruction diff')
    ap.add_argument('--mutate-test', type=int, metavar='N', help='print N random mutations of the start (no compiles)')
    args = ap.parse_args()
    if args.eval:
        s = Search(args)
        text = splice(s.unit, s.orig, Path(args.eval).read_text(encoding='utf-8', errors='replace'))
        score, _, cand = s.scorer.score(text, 'eval')
        print('score', score)
        if isinstance(cand, bytes):
            print('\n'.join(s.scorer.insn_diff(cand, 'eval')))
        else:
            print(cand)
        return
    if args.mutate_test:
        s = Search(args)
        rng = random.Random(args.rng_seed)
        text = s.orig
        if args.seed:
            text = splice(s.unit, s.orig, Path(args.seed[0]).read_text(encoding='utf-8', errors='replace'))
        for _ in range(args.mutate_test):
            r = mutate(s.unit, text, rng, s.aliases, s.weights, s.focus)
            if r:
                print('###', r[1])
                print(s.diff(r[0]))
        return
    Search(args).run()


if __name__ == '__main__':
    main()
