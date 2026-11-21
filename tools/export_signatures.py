#!/usr/bin/env python3
"""Export a machine-readable list of function signatures for the PC port.

For every function of config/semantic/selection-template.json ("functions": VA -> symbol -> source)
writes one row to config/US/signatures.tsv (return type, parameters, varargs, source, kind, ...),
the types those signatures name to config/US/signature_types.tsv, and prototypes that disagree
between units to config/US/signature_conflicts.tsv. Format: docs/SIGNATURES.md.

How the prototypes are obtained:
  * C units: preprocessed exactly as tools/semantic_compile.py does, then the native GCC 2.7.2 cc1
    (toolchain/gcc-2.7.2-psx) with -aux-info, which writes every function declaration and definition
    the unit sees, with full prototypes, typedef names as written and (for definitions) parameter
    names. K&R definitions are recorded with promoted parameter types.
  * C++ units (PsyQ CC1PLPSX has no -aux-info): a small parser over the preprocessed text;
    constructors/methods mapped by `#pragma tm2_symbol` get an explicit `this` first parameter.
  * Functions defined in file-scope __asm__ blocks: kind=asm, prototype taken from the callers'
    declarations in other units, else from the PsyQ headers under an alias name (extra_funcs.tsv,
    ida_names.tsv, names.tsv old name).

Nothing under src/ is changed; scratch output goes to work/signatures/.
"""
import argparse
import bisect
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
MIPS = ROOT / 'toolchain/mips/bin'
CPP = MIPS / 'mipsel-none-elf-cpp.exe'
CC1 = ROOT / 'toolchain/gcc-2.7.2-psx/cc1.exe'
PSYQ_INC = ROOT / 'toolchain/psyq330/include'
TEMPLATE = ROOT / 'config/semantic/selection-template.json'
OUT_SIGS = ROOT / 'config/US/signatures.tsv'
OUT_TYPES = ROOT / 'config/US/signature_types.tsv'
OUT_CONFLICTS = ROOT / 'config/US/signature_conflicts.tsv'

# Same preprocessor flags as tools/semantic_compile.compile_source.
CPP_FLAGS = ['-EL', '-nostdinc', '-undef', '-D__GNUC__=2', '-D__OPTIMIZE__',
             '-Dmips', '-D__mips__', '-D__mips', '-Dpsx', '-D__psx__', '-D__psx',
             '-D__EXTENSIONS__', '-D_MIPSEL', '-D__CHAR_UNSIGNED__',
             '-D_LANGUAGE_C', '-DLANGUAGE_C', '-DINCLUDE_ASM_USE_MACRO_INC=1',
             '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'), '-I' + str(PSYQ_INC)]
PSYQ_SKIP = {'ASM.H', 'GTEMAC.H', 'INLINE.H', 'GTENOM.H', 'R3000.H'}

KEYWORDS = {'struct', 'union', 'enum', 'class', 'const', 'volatile', 'unsigned', 'signed', 'int', 'char',
            'short', 'long', 'void', 'float', 'double', 'register', 'extern', 'static', 'inline', '__inline__',
            '__const__', '__volatile__', '__signed__', '__unsigned__'}
NARROW = {'char', 'signed char', 'unsigned char', 'short', 'unsigned short'}


def rel(path):
    p = str(path).replace('\\\\', '/').replace('\\', '/')
    root = str(ROOT).replace('\\', '/')
    if p.lower().startswith(root.lower() + '/'):
        return p[len(root) + 1:]
    if p.lower().startswith('../../'):
        return p[6:]
    return p


def split_top(text, sep=','):
    """Split at `sep` outside () [] {}."""
    out, depth, cur = [], 0, []
    for ch in text:
        if ch in '([{':
            depth += 1
        elif ch in ')]}':
            depth -= 1
        if ch == sep and depth == 0:
            out.append(''.join(cur).strip())
            cur = []
        else:
            cur.append(ch)
    if ''.join(cur).strip():
        out.append(''.join(cur).strip())
    return out


def close_paren(text, i):
    """Index of the ')' matching text[i] == '('."""
    depth = 0
    for j in range(i, len(text)):
        if text[j] == '(':
            depth += 1
        elif text[j] == ')':
            depth -= 1
            if not depth:
                return j
    return -1


def norm_space(text):
    text = re.sub(r'\s+', ' ', text).strip()
    text = re.sub(r'\s*\*\s*', ' *', text)          # "T * x" / "T *x" -> "T *x"
    text = re.sub(r'\(\s+', '(', text)
    text = re.sub(r'\s+\)', ')', text)
    text = text.replace(' *)', ' *)').replace('( *', '(*')
    return text.strip()


def strip_name(param, name):
    """Remove the declarator name from a 'type name' parameter."""
    if not name:
        return norm_space(param)
    return norm_space(re.sub(r'\b%s\b' % re.escape(name), '', param, count=1))


def guess_name(param):
    """Declarator name of a C++ 'type name' parameter (None for an abstract declarator)."""
    m = re.search(r'\(\s*\*\s*(\w+)\s*\)', param)
    if m:
        return m.group(1)
    p = re.sub(r'\[[^\]]*\]', '', param).strip()
    p = re.sub(r'=.*$', '', p).strip()
    m = re.search(r'(\w+)\s*$', p)
    if not m:
        return None
    word = m.group(1)
    before = p[:m.start()].strip()
    if word in KEYWORDS or not before or before.split()[-1] in ('struct', 'union', 'enum', 'class'):
        return None
    if before in ('const', 'volatile'):
        return None
    return word


# ---------------------------------------------------------------- preprocessed text scanning

class Source:
    """Preprocessed text with line markers removed, mapping offsets back to file:line."""

    def __init__(self, text):
        lines = text.split('\n')
        self.pragmas = []
        self.loc = []
        cur_file, cur_line = '?', 1
        clean = []
        for line in lines:
            m = re.match(r'#\s*(?:line\s+)?(\d+)\s+"([^"]*)"', line)
            if m:
                cur_file, cur_line = rel(m.group(2)), int(m.group(1))
                clean.append('')
                self.loc.append((cur_file, cur_line))
                continue
            if line.lstrip().startswith('#'):
                if 'pragma' in line:
                    self.pragmas.append(line.strip())
                clean.append('')
            else:
                clean.append(line)
            self.loc.append((cur_file, cur_line))
            cur_line += 1
        self.text = '\n'.join(clean)
        self.starts = [0]
        for line in clean[:-1]:
            self.starts.append(self.starts[-1] + len(line) + 1)

    def where(self, offset):
        i = bisect.bisect_right(self.starts, offset) - 1
        return self.loc[i]

    def statements(self):
        """Top-level statements: (text, offset, body or None). Function bodies and struct/initialiser
        braces are kept as `body` text; `extern "C" {` blocks are transparent."""
        t, n, i = self.text, len(self.text), 0
        out, start, buf = [], None, []
        while i < n:
            ch = t[i]
            if ch in '"\'':
                j = i + 1
                while j < n and t[j] != ch:
                    j += 2 if t[j] == '\\' else 1
                if start is None:
                    start = i
                buf.append(t[i:j + 1])
                i = j + 1
                continue
            if ch.isspace() and start is None:
                i += 1
                continue
            if start is None:
                start = i
            if ch == '{':
                head = ''.join(buf).strip()
                if re.fullmatch(r'extern\s*"C"', head):
                    start, buf = None, []
                    i += 1
                    continue
                depth, j = 0, i
                while j < n:
                    c = t[j]
                    if c in '"\'':
                        k = j + 1
                        while k < n and t[k] != c:
                            k += 2 if t[k] == '\\' else 1
                        j = k + 1
                        continue
                    if c == '{':
                        depth += 1
                    elif c == '}':
                        depth -= 1
                        if not depth:
                            break
                    j += 1
                body = t[i:j + 1]
                if not is_function_head(head):
                    # K&R definition: `f(a, b) T a; T b; {` was split at the parameter declarations
                    for k in range(len(out) - 1, max(len(out) - 16, -1), -1):
                        if out[k][2] is not None:
                            break
                        if KNR_HEAD.search(strip_attrs(out[k][0])):
                            head = '; '.join([s[0] for s in out[k:]] + ([head] if head else [])) + ';'
                            start = out[k][1]
                            del out[k:]
                            break
                if is_function_head(head):
                    out.append((head, start, body))
                    start, buf = None, []
                    i = j + 1
                    continue
                buf.append(body)
                i = j + 1
                continue
            if ch == '}':           # closing an extern "C" block
                start, buf = None, []
                i += 1
                continue
            if ch == ';':
                out.append((''.join(buf).strip(), start, None))
                start, buf = None, []
                i += 1
                continue
            buf.append(ch)
            i += 1
        return out


def strip_attrs(text):
    while True:
        m = re.search(r'__attribute__\s*\(', text)
        if not m:
            return text
        end = close_paren(text, m.end() - 1)
        text = text[:m.start()] + ' ' + text[end + 1:]


KNR_HEAD = re.compile(r'[A-Za-z_]\w*\s*\(\s*[A-Za-z_]\w*(?:\s*,\s*[A-Za-z_]\w*)*\s*\)\s*[A-Za-z_]')


def is_function_head(head):
    h = strip_attrs(head)
    if h.rstrip().endswith(';') and KNR_HEAD.search(h):
        return True
    h = re.sub(r'extern\s*"C"', '', h)
    if re.match(r'\s*(typedef|struct|union|enum|class)\b', h) and not re.search(r'\)\s*(:.*)?$', h, re.S):
        return False
    if '=' in re.sub(r'\(.*', '', h, flags=re.S):
        return False
    return bool(re.search(r'\)\s*(const\s*)?(:.*)?$', h, re.S) or re.search(r'\)\s*\w+[^;]*$', h, re.S))


def asm_symbols(text):
    """(name, VA or None, kind) for symbols defined in file-scope asm strings."""
    strings = ''.join(re.findall(r'"((?:[^"\\]|\\.)*)"', text))
    strings = strings.replace('\\n', '\n').replace('\\t', '\t').replace('\\"', '"')
    out, va, funcs = [], None, set(re.findall(r'\.(?:type\s+(\w+)\s*,\s*@function|ent\s+(\w+))', strings))
    funcs = {a or b for a, b in funcs}
    for line in strings.split('\n'):
        line = line.strip()
        m = re.match(r'\.section\s+\.text_([0-9A-Fa-f]{8})', line)
        if m:
            va = m.group(1).upper()
        elif re.match(r'\.(section|data|rdata|bss)\b', line):
            va = None
        m = re.match(r'\.glob(?:a)?l\s+(\w+)', line)
        if m:
            out.append((m.group(1), va, m.group(1) in funcs))
    return out


# ---------------------------------------------------------------- types

class Types:
    """Type definitions of one unit: name -> (kind, file:line, definition, underlying for typedefs)."""

    def __init__(self, src, statements):
        self.defs = {}
        for head, off, body in statements:
            text = strip_attrs(head if body is None else head + body)
            where = '%s:%d' % src.where(off)
            text = norm_space(text)
            if text.startswith('typedef'):
                inner = text[len('typedef'):].strip()
                m = re.search(r'\(\s*\*\s*(\w+)\s*\)', inner)
                name = m.group(1) if m else (re.search(r'(\w+)\s*(\[[^\]]*\])*$', inner) or [None, None])[1]
                if not name:
                    continue
                under = norm_space(re.sub(r'\b%s\b' % name, '', inner, count=1))
                if under and '{' in under:
                    tm = re.match(r'(struct|union|enum)\s+(\w+)\s*\{', under)
                    if tm:
                        self.defs.setdefault(tm.group(1) + ' ' + tm.group(2), (tm.group(1), where, text, None))
                        under = tm.group(1) + ' ' + tm.group(2)
                    else:
                        under = under.split('{')[0].strip() + ' {...}'
                self.defs[name] = ('typedef', where, text, under)
            else:
                m = re.match(r'(?:extern\s*"C"\s*)?(struct|union|enum|class)\s+(\w+)\s*(?::[^{]*)?\{', text)
                if m:
                    kind = 'struct' if m.group(1) == 'class' else m.group(1)
                    self.defs[kind + ' ' + m.group(2)] = (kind, where, text, None)
                    if src.cplusplus:          # C++: the tag is also a type name
                        self.defs.setdefault(m.group(2), (kind, where, text, kind + ' ' + m.group(2)))

    def resolve(self, typ, depth=0):
        """Canonical spelling for comparisons: typedefs expanded, int spellings unified, no qualifiers."""
        t = ' ' + typ + ' '
        for _ in range(8):
            changed = False
            for word in re.findall(r'(?<![\w.])(?<!struct )(?<!union )(?<!enum )([A-Za-z_]\w*)', t):
                d = self.defs.get(word)
                if d and d[0] == 'typedef' and d[3] and '{...}' not in d[3]:
                    t = re.sub(r'(?<!struct )(?<!union )(?<!enum )\b%s\b' % word, ' ' + d[3] + ' ', t, count=1)
                    changed = True
                    break
                if d and d[0] != 'typedef' and d[3]:      # C++ tag used as a type name
                    t = re.sub(r'(?<!struct )(?<!union )(?<!enum )\b%s\b' % word, ' ' + d[3] + ' ', t, count=1)
                    changed = True
                    break
            if not changed:
                break
        t = re.sub(r'\b(const|volatile|register|__const__|__volatile__)\b', ' ', t)
        t = re.sub(r'\bclass\b', 'struct', t)
        t = re.sub(r'\b__signed__\b', 'signed', t)
        t = re.sub(r'\blong\s+int\b|\blong\b', 'int', t)
        # cc1 -aux-info spells some types word-reversed ('short unsigned int')
        t = re.sub(r'\bshort\s+unsigned(\s+int)?\b|\bunsigned\s+short\s+int\b', 'unsigned short', t)
        t = re.sub(r'\bint\s+unsigned(\s+int)?\b', 'unsigned int', t)
        t = re.sub(r'\bchar\s+unsigned\b', 'unsigned char', t)
        t = re.sub(r'\bshort\s+int\b', 'short', t)
        t = re.sub(r'\bsigned\s+(int|short)\b', r'\1', t)
        t = re.sub(r'\bsigned\b(?!\s+char)', 'int', t)
        t = re.sub(r'\bunsigned\b(?!\s+(int|short|char))', 'unsigned int', t)
        t = re.sub(r'\bint\s+int\b', 'int', t)
        t = norm_space(t)
        t = re.sub(r'\[[^\]]*\]$', ' *', t)          # array parameter decays
        return norm_space(t)


def abi_class(canon):
    """Register-level class on the R3000: pointers and 32-bit integers share 'w32'."""
    c = canon.replace(' ', '')
    if '*' in c or '(' in c:
        return 'w32'
    if c == 'void':
        return 'void'
    if c in ('int', 'unsignedint') or c.startswith('enum'):
        return 'w32'
    if c in ('short', 'unsignedshort'):
        return 's16' if c == 'short' else 'u16'
    if c in ('char', 'signedchar', 'unsignedchar'):
        return 'u8' if c == 'unsignedchar' else 's8'
    if c in ('float', 'double'):
        return c
    return 'agg'          # struct/union by value (or an unresolved by-value typedef)


SEVERITY_ORDER = ('count', 'abi', 'sign', 'return-void', 'type', 'implicit', 'unprototyped')


def severity(protod, weak):
    """Worst difference between the prototyped variants (canon tuples), see docs/SIGNATURES.md."""
    found = set()
    if any(c[0] == 'implicit' for c in weak):
        found.add('implicit')
    elif weak:
        found.add('unprototyped')
    if len(protod) > 1:
        found.add('type')
        if len({(len(c[1]), c[2]) for c in protod}) > 1:
            found.add('count')
        rets = {abi_class(c[0]) for c in protod}
        if 'void' in rets and len(rets) > 1:
            found.add('return-void')
            rets.discard('void')
        positions = [rets] + ([{abi_class(c[1][k]) for c in protod} for k in range(len(protod[0][1]))]
                              if 'count' not in found else [])
        for s in positions:
            if len(s) < 2:
                continue
            if len(s) == 2 and len({x[1:] for x in s}) == 1 and all(x[0] in 'su' for x in s):
                found.add('sign')
            else:
                found.add('abi')
    return next(s for s in SEVERITY_ORDER if s in found)


def type_words(typ):
    words = []
    for m in re.finditer(r'\b(struct|union|enum|class)\s+(\w+)|\b([A-Za-z_]\w*)\b', typ):
        if m.group(1):
            words.append(('struct' if m.group(1) == 'class' else m.group(1)) + ' ' + m.group(2))
        elif m.group(3) not in KEYWORDS and not m.group(3).startswith('__'):
            words.append(m.group(3))
    return words


# ---------------------------------------------------------------- prototypes

class Proto:
    """One declaration or definition of a function."""

    def __init__(self, name, ret, params, varargs, prototyped, definition, where, unit, style, static=False,
                 knr=None):
        self.name, self.ret, self.params, self.varargs = name, ret, params, varargs
        self.prototyped, self.definition, self.where, self.unit = prototyped, definition, where, unit
        self.style, self.static, self.knr = style, static, knr or []
        self.qual = None            # C++ qualified name (Class::method) of a constructor/method

    def text(self):
        if not self.prototyped:
            return '%s %s()' % (self.ret, self.name)
        ps = [p[0] for p in self.params] + (['...'] if self.varargs else [])
        return '%s %s(%s)' % (self.ret, self.name, ', '.join(ps) or 'void')


AUX = re.compile(r'^/\* (.*):(\d+):([NOI])([CFD]) \*/ (.*?);(?: /\* (.*) \*/)?\s*$')


def parse_declarator(decl):
    """'extern int f (int a, ...)' -> (static, name, ret, inside params text)."""
    static = decl.startswith('static ')
    decl = re.sub(r'^(extern|static)\s+', '', decl)
    for m in re.finditer(r'([A-Za-z_]\w*)\s*\(', decl):
        if decl[m.end():].lstrip().startswith('*'):
            continue
        if m.group(1) in KEYWORDS:
            continue
        end = close_paren(decl, m.end() - 1)
        ret = norm_space(decl[:m.start()] + decl[end + 1:])
        return static, m.group(1), ret, decl[m.end():end]
    return static, None, None, None


def promote(typ, types):
    c = types.resolve(typ)
    if c.replace('signed char', 'char') in NARROW or c in NARROW:
        return 'int'
    if c == 'float':
        return 'double'
    return None


def parse_aux(path, unit, types):
    protos = []
    for line in path.read_text(encoding='latin-1').splitlines():
        m = AUX.match(line)
        if not m:
            continue
        where = '%s:%s' % (rel(m.group(1)), m.group(2))
        style, kind, decl, comment = m.group(3), m.group(4), m.group(5), m.group(6)
        static, name, ret, inner = parse_declarator(decl)
        if not name:
            continue
        definition = kind == 'F'
        prototyped = '/* ??? */' not in inner
        names = []
        if definition and comment:
            cm = re.match(r'\(([^)]*)\)', comment)
            if cm:
                names = [x.strip() for x in cm.group(1).split(',') if x.strip()]
        params, varargs = [], False
        if prototyped:
            items = [p for p in split_top(inner) if p]
            if items == ['void']:
                items = []
            if items and items[-1] == '...':
                varargs, items = True, items[:-1]
            for k, p in enumerate(items):
                nm = names[k] if k < len(names) else None
                params.append([norm_space(p), strip_name(p, nm), nm])
        knr = []
        if style == 'O' and definition:
            for p in params:
                promoted = promote(p[1], types)
                if promoted:
                    knr.append('%s:%s' % (p[2], p[1]))
                    p[0] = '%s %s' % (promoted, p[2]) if p[2] else promoted
                    p[1] = promoted
        st = {'N': 'ansi', 'O': 'knr' if definition else 'unprototyped', 'I': 'implicit'}[style]
        if style == 'O' and definition and not params:
            st = 'knr'
        protos.append(Proto(name, ret, params, varargs, prototyped or definition, definition, where, unit, st,
                            static, knr))
    return protos




def demangle_owner(mangled):
    """GCC 2.x mangling: '__<n>Class...' (constructor) or 'meth__<n>Class...' -> (class, method)."""
    m = re.match(r'(\w*?)__(?:Q\d)?(\d+)', mangled)
    if not m:
        return None
    n = int(m.group(2))
    rest = mangled[m.end():]
    cls = rest[:n]
    return cls, (m.group(1) or cls)


def parse_cpp(src, statements, unit):
    """Declarations/definitions of a preprocessed C++ unit."""
    pragmas = {}
    for p in src.pragmas:
        m = re.match(r'#\s*pragma\s+tm2_symbol\s+(\S+)\s+(\w+)', p)
        if m:
            owner = demangle_owner(m.group(1))
            if owner:
                pragmas[owner] = m.group(2)
    protos = []
    for head, off, body in statements:
        h = norm_space(strip_attrs(re.sub(r'extern\s*"C"', ' ', head)))
        if re.match(r'(typedef|struct|union|enum|class|template|using|namespace)\b', h):
            continue
        if body is not None:
            h = re.sub(r'\)\s*:(?!:).*$', ')', h, flags=re.S)   # constructor initialiser list
        m = None
        for mm in re.finditer(r'((?:\w+::)?~?[A-Za-z_]\w*)\s*\(', h):
            if h[mm.end():].lstrip().startswith('*') or mm.group(1) in KEYWORDS:
                continue
            m = mm
            break
        if not m:
            continue
        end = close_paren(h, m.end() - 1)
        if end < 0:
            continue
        ret = norm_space(h[:m.start()])
        static = ret.startswith('static ')
        ret = norm_space(re.sub(r'\b(static|extern|inline|virtual)\b', '', ret))
        qual = m.group(1)
        params, varargs = [], False
        items = [p for p in split_top(h[m.end():end]) if p]
        if items == ['void']:
            items = []
        if items and items[-1] == '...':
            varargs, items = True, items[:-1]
        for p in items:
            p = re.sub(r'\s*=.*$', '', p)
            nm = guess_name(p)
            params.append([norm_space(p), strip_name(p, nm), nm])
        style = 'cpp'
        name = qual
        if '::' in qual:
            cls, meth = qual.split('::')
            sym = pragmas.get((cls, meth))
            name = sym or qual
            params.insert(0, ['%s *this' % cls, '%s *' % cls, 'this'])
            if meth == cls:
                ret = '%s *' % cls
                style = 'cpp-ctor'
            else:
                style = 'cpp-method'
        elif not ret:
            continue
        protos.append(Proto(name, ret, params, varargs, True, body is not None, '%s:%d' % src.where(off),
                            unit, style, static))
        protos[-1].qual = qual
    return protos


# ---------------------------------------------------------------- per-unit processing

def run(cmd, cwd):
    return subprocess.run([str(c) for c in cmd], cwd=cwd, capture_output=True, stdin=subprocess.DEVNULL,
                          timeout=120)


def process_unit(unit, work):
    source = ROOT / unit
    d = work / re.sub(r'[^\w.]', '_', unit)
    d.mkdir(parents=True, exist_ok=True)
    cplusplus = source.suffix == '.cpp'
    pp = d / 'unit.i'
    proc = run([CPP, *CPP_FLAGS, *(['-x', 'c', '-D__cplusplus=1'] if cplusplus else []), source, '-o', pp], d)
    if proc.returncode:
        return dict(unit=unit, error='cpp: ' + proc.stderr.decode(errors='replace')[:500])
    src = Source(pp.read_text(encoding='latin-1'))
    src.cplusplus = cplusplus
    stmts = src.statements()
    types = Types(src, stmts)
    sections, asm = {}, []
    for head, off, body in stmts:
        if body is None and re.match(r'(__asm__|asm)\b', head):
            asm.extend((n, va, f, '%s:%d' % src.where(off)) for n, va, f in asm_symbols(head))
        else:       # definition, or a prototype carrying the section attribute
            m = re.search(r'section\s*\(\s*"\.text_"?\s*"?([0-9A-Fa-f]{8})"', head)
            if m and (body is not None or m.group(0) and re.search(r'\w\s*\(', head)):
                h = norm_space(strip_attrs(re.sub(r'extern\s*"C"', ' ', head)))
                nm = re.search(r'((?:\w+::)?~?[A-Za-z_]\w*)\s*\((?!\s*\*)', h)
                if nm:
                    sections[nm.group(1)] = m.group(1).upper()
    if cplusplus:
        protos = parse_cpp(src, stmts, unit)
        error = None
    else:
        aux = d / 'unit.aux'
        proc = run([CC1, '-quiet', '-fno-builtin', '-G8', '-aux-info', aux, pp, '-o', d / 'unit.s'], d)
        error = None
        if proc.returncode or not aux.is_file():
            error = 'cc1: ' + proc.stderr.decode(errors='replace')[:500]
            protos = []
        else:
            protos = parse_aux(aux, unit, types)
    return dict(unit=unit, protos=protos, types=types, asm=asm, sections=sections, error=error)


def psyq_protos(work):
    """Declarations in the PsyQ 3.3 headers (fallback prototypes for asm SDK routines)."""
    d = work / '_psyq'
    d.mkdir(parents=True, exist_ok=True)
    found = {}
    for h in sorted(PSYQ_INC.glob('*.H')):
        if h.name in PSYQ_SKIP:
            continue
        c = d / ('h_%s.c' % h.stem)
        c.write_text('#include <SYS/TYPES.H>\n#include <LIBGTE.H>\n#include <LIBGPU.H>\n#include <%s>\n' % h.name,
                     encoding='ascii', newline='\n')
        i, aux = d / ('h_%s.i' % h.stem), d / ('h_%s.aux' % h.stem)
        if run([CPP, '-w', '-nostdinc', '-undef', '-D__GNUC__=2', '-Dmips', '-D__mips__', '-Dpsx',
                '-D_LANGUAGE_C', '-DLANGUAGE_C', '-I' + str(PSYQ_INC), c, '-o', i], d).returncode:
            continue
        if run([CC1, '-quiet', '-aux-info', aux, i, '-o', d / 'h.s'], d).returncode or not aux.is_file():
            continue
        src = Source(i.read_text(encoding='latin-1'))
        src.cplusplus = False
        types = Types(src, src.statements())
        for p in parse_aux(aux, '_psyq', types):
            if p.prototyped or p.name not in found:     # prefer a prototype; else keep the return type
                if p.name not in found or not found[p.name][0].prototyped:
                    found[p.name] = (p, types)
    return found


# ---------------------------------------------------------------- main

def load_aliases():
    alias = defaultdict(set)
    for path, cols in (('config/US/extra_funcs.tsv', (0, 2)), ('config/US/ida_names.tsv', (0, 2)),
                       ('config/US/names.tsv', (0, 1))):
        for line in (ROOT / path).read_text(encoding='utf-8').splitlines():
            if line.startswith('#') or line.startswith('va\t'):
                continue
            f = line.split('\t')
            if len(f) > max(cols):
                alias[f[cols[0]].upper()].add(f[cols[1]])
    return alias


def load_sdk_ranges():
    rows = []
    for line in (ROOT / 'config/US/sdk_modules.tsv').read_text(encoding='utf-8').splitlines():
        if line.startswith('#') or line.startswith('lib\t'):
            continue
        f = line.split('\t')
        start, size = int(f[3], 16), int(f[4], 16)
        funcs = {x.upper() for x in f[6].split()} if len(f) > 6 else set()
        rows.append((start, start + size, '%s/%s' % (f[0], f[1]), funcs))
    return rows


def sdk_lib(va, ranges):
    v = int(va, 16)
    for start, end, name, funcs in ranges:
        if start <= v < end or va in funcs:
            return name
    return ''


def tsv(path, header, rows, comment):
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        for c in comment:
            f.write('# ' + c + '\n')
        f.write('\t'.join(header) + '\n')
        for r in rows:
            f.write('\t'.join(str(x).replace('\t', ' ').replace('\n', ' ') for x in r) + '\n')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--work', type=Path, default=ROOT / 'work/signatures')
    ap.add_argument('--jobs', type=int, default=os.cpu_count() or 4)
    args = ap.parse_args()
    work = args.work.resolve()
    if not work.is_relative_to(ROOT / 'work'):
        ap.error('--work must be beneath work/')
    if work.exists():
        shutil.rmtree(work)
    work.mkdir(parents=True)

    template = json.loads(TEMPLATE.read_text(encoding='utf-8'))
    functions = template['functions']
    units = sorted(set(template['units']) | {f['source'] for f in functions.values()})
    with ThreadPoolExecutor(args.jobs) as pool:
        results = {r['unit']: r for r in pool.map(lambda u: process_unit(u, work), units)}
    errors = {u: r['error'] for u, r in results.items() if r.get('error')}
    for u, e in errors.items():
        print('warning: %s: %s' % (u, e.splitlines()[0] if e else e), file=sys.stderr)
    # Pure-C reference versions of the hand-asm units (build_source_only --all-c): prototypes for
    # their asm functions. Not part of the conflict pool (they are not in the normal build).
    sys.path.insert(0, str(ROOT / 'tools'))
    try:
        from build_source_only import ALL_C_UNITS
    except Exception:            # pragma: no cover - keep working if that tool changes
        ALL_C_UNITS = {}
    reference = {}
    for unit, ref in ALL_C_UNITS.items():
        if (ROOT / ref).is_file():
            r = process_unit(ref, work)
            if not r.get('error'):
                reference[unit] = r
    psyq = psyq_protos(work)
    aliases = load_aliases()
    ranges = load_sdk_ranges()

    # Pool of all prototypes. Statics are keyed by unit; header declarations seen by many units
    # are deduplicated by location.
    pool = defaultdict(dict)
    for r in results.values():
        for p in r.get('protos', []):
            key = (p.name, p.unit) if p.static else (p.name, None)
            pool[key].setdefault((p.where, p.definition), (p, r['types']))

    rows, used_types, checks = [], {}, Counter()
    for va, f in sorted(functions.items()):
        name, unit = f['symbol'], f['source']
        r = results[unit]
        types = r['types']
        lib = sdk_lib(va, ranges)
        kind = 'sdk' if lib else 'game'
        defs = [p for p in r.get('protos', []) if p.name == name and p.definition]
        asm = [a for a in r.get('asm', []) if a[0] == name]
        proto, proto_from, check = None, '', 'ok'
        if defs:
            proto = defs[0]
            proto_from = proto.where
            sec = r['sections'].get(proto.qual or name)
            if sec is None:
                check = 'ok (no section attribute)'
            elif sec != va:
                check = 'section .text_%s' % sec
        elif asm:
            kind = 'asm'
            if asm[0][1] is None:
                check = 'ok (no section attribute)'
            elif asm[0][1] != va:
                check = 'section .text_%s' % asm[0][1]
            cands = [(p, t) for (p, t) in pool.get((name, None), {}).values() if not p.definition]
            cands.sort(key=lambda pt: (not pt[0].prototyped, pt[0].where))
            ref = reference.get(unit)
            ref_def = None
            if ref:
                by_va = {n for n, v in ref['sections'].items() if v == va}
                ref_def = next((p for p in ref.get('protos', []) if p.definition and
                                (p.name == name or p.name in by_va)), None)
            if ref_def:
                proto, types = ref_def, ref['types']
                proto_from = 'reference C %s%s' % (proto.where, '' if proto.name == name else ' as ' + proto.name)
            elif cands:
                counts = Counter(pt[1].resolve(pt[0].text()) for pt in cands if pt[0].prototyped)
                best = counts.most_common(1)[0][0] if counts else None
                pick = next((pt for pt in cands if pt[0].prototyped and pt[1].resolve(pt[0].text()) == best),
                            cands[0])
                proto, types = pick
                proto_from = 'decl %s (%d decl%s)' % (proto.where, len(cands), 's' if len(cands) > 1 else '')
            else:
                for al in sorted(aliases.get(va, ())):
                    if al in psyq:
                        proto, types = psyq[al]
                        proto_from = 'psyq %s as %s' % (proto.where, al)
                        break
            if not proto:
                proto_from = asm[0][3]
        else:
            check = 'not defined in source'
        checks[check] += 1
        if proto:
            ret = proto.ret
            params = ', '.join(p[0] for p in proto.params)
            if proto.style in ('unprototyped',) or not proto.prototyped:
                params = '?'
            varargs = 'yes' if proto.varargs else 'no'
            style = proto.style if kind != 'asm' else 'asm-' + proto.style
            linkage = 'static' if proto.static else 'global'
            knr = ' '.join(proto.knr)
            tnames = []
            for t in [ret] + [p[1] for p in proto.params]:
                for w in type_words(t):
                    if w not in tnames:
                        tnames.append(w)
            tcol = []
            for w in tnames:
                d = types.defs.get(w)
                if d:
                    used_types.setdefault((w, d[1]), (d, types))
                    canon = types.resolve(w)
                    if d[0] == 'typedef' and abi_class(canon) != 'agg' and '*' not in canon and '(' not in canon:
                        continue            # scalar typedef (s32, u8, ...): see signature_types.tsv
                    tcol.append('%s=%s' % (w, d[1]))
                else:
                    tcol.append(w)
            types_col = ', '.join(tcol)
        else:
            ret, params, varargs, style, linkage, knr, types_col = '?', '?', '?', 'asm-unknown' if asm else '?', \
                'global', '', ''
        rows.append([va, name, ret, params, varargs, unit, kind, lib, style, linkage, proto_from, knr, types_col,
                     check])

    # transitive closure of the referenced type definitions
    queue = list(used_types.items())
    while queue:
        (w, where), (d, types) = queue.pop()
        for w2 in type_words(re.sub(r'^typedef\s+', '', d[2])):
            if w2 == w or w2 == (w.split(' ')[-1]):
                continue
            d2 = types.defs.get(w2)
            if d2 and (w2, d2[1]) not in used_types:
                used_types[(w2, d2[1])] = (d2, types)
                queue.append(((w2, d2[1]), (d2, types)))
    # identical definitions (the s32/u8 typedefs repeated in every unit, shared headers) collapse to one row
    same = defaultdict(list)
    for (w, where), (d, _) in used_types.items():
        same[(w, d[0], d[2])].append(where)
    type_rows = []
    for (w, kind, text), wheres in sorted(same.items()):
        wheres.sort(key=lambda x: (not x.startswith('include/'), x))
        type_rows.append([w, kind, wheres[0] + (' (+%d more)' % (len(wheres) - 1) if len(wheres) > 1 else ''),
                          text])

    # conflicts: same function, different canonical prototypes
    conflict_rows, conflict_funcs, sev_count = [], 0, Counter()
    va_of = {f['symbol']: va for va, f in functions.items()}
    for (name, static_unit), variants in sorted(pool.items(), key=lambda kv: (kv[0][0], kv[0][1] or '')):
        groups = defaultdict(list)
        for (where, definition), (p, types) in variants.items():
            if p.prototyped:
                canon = (types.resolve(p.ret), tuple(types.resolve(x[1]) for x in p.params), p.varargs)
            else:
                canon = ('unprototyped' if p.style != 'implicit' else 'implicit',)
            groups[canon].append((p, types))
        protod = [c for c in groups if len(c) == 3]
        weak = [c for c in groups if len(c) == 1]
        if len(protod) < 2 and not (weak and protod and any(c[1] for c in protod)):
            continue
        sev = severity(protod, weak)
        conflict_funcs += 1
        sev_count[sev] += 1
        for k, (canon, items) in enumerate(sorted(groups.items(), key=lambda kv: -len(kv[1]))):
            p = items[0][0]
            sites = ', '.join(sorted({'%s%s' % (q.where, ' (def)' if q.definition else '') for q, _ in items}))
            conflict_rows.append([name, va_of.get(name, ''), sev, k + 1, p.text(),
                                  canon[0] if len(canon) == 1 else '%s (%s%s)' % (
                                      canon[0], ', '.join(canon[1]), ', ...' if canon[2] else ''),
                                  len(items), sites])

    conflict_rows.sort(key=lambda r: (SEVERITY_ORDER.index(r[2]), r[0], r[3]))
    tsv(OUT_SIGS, ['va', 'name', 'return_type', 'params', 'varargs', 'source', 'kind', 'sdk_module', 'style',
                   'linkage', 'proto_from', 'knr_declared', 'types', 'check'], rows,
        ['Generated by tools/export_signatures.py; do not edit. One row per function of',
         'config/semantic/selection-template.json. Format: docs/SIGNATURES.md.'])
    tsv(OUT_TYPES, ['name', 'kind', 'defined_at', 'definition'], type_rows,
        ['Generated by tools/export_signatures.py; do not edit. Type definitions named by',
         'config/US/signatures.tsv (transitively). Format: docs/SIGNATURES.md.'])
    tsv(OUT_CONFLICTS, ['name', 'va', 'severity', 'variant', 'prototype', 'canonical', 'sites', 'where'],
        conflict_rows,
        ['Generated by tools/export_signatures.py; do not edit. Functions declared with different',
         'prototypes in different places; one row per distinct prototype. Format: docs/SIGNATURES.md.'])
    kinds = Counter(r[6] for r in rows)
    print('signatures: %d rows (%s); checks %s' % (len(rows), ', '.join('%s %d' % kv for kv in sorted(kinds.items())),
                                                   dict(checks)))
    print('unknown prototypes: %d' % sum(r[2] == '?' for r in rows))
    print('types: %d definitions' % len(type_rows))
    print('conflicts: %d functions, %d rows (%s)' % (conflict_funcs, len(conflict_rows),
                                                     ', '.join('%s %d' % kv for kv in sorted(sev_count.items()))))
    if errors:
        print('units with errors: %d' % len(errors))
    return 0


if __name__ == '__main__':
    sys.exit(main())
