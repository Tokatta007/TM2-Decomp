#!/usr/bin/env python3
"""List the level-of-detail (LOD) records of the DMD model files (PC port request 30, docs/FAR_LOD.md).

Reads `DMD\\*.DMD` straight from the USA disc image (`cdimage/... (Track 01).bin`, MODE2/2352, ISO 9660;
read-only) and walks every root of each file the way the renderer's tree walker does (`renderWalkModel`,
src/semantic/engine_render_scene.c; C form in reference/renderer_walker_engine_render_scene.c).

For every opcode-2 node (LOD) the listing gives its records in file order: [min, max) as squared distances
(the walker's d2, world units squared) and as distances, and whether the record's subtree reaches a mesh
(opcode 0). Node flags:

  NONVISUAL  no record reaches a mesh (only hit volumes, opcode 8): nothing to see at any distance.
  FINITE     the record with the largest max is not open-ended (max < 0xFFFFFFFF): past that distance
             retail draws nothing for the node.
  NEARGAP    no record holds d2 = 100 (the retail LOD override, gp+1008, forces d2 = 100).
  PICKUP     a record holds a pickup node (ids 401..430, 441..480; docs/PICKUP_LOOKS.md).
  EXT        the far-LOD switch (portFarLod != 0, --port-define TM2_PORT_FAR_LOD) extends the record
             marked `ext` to an unbounded max while the renderer walks (the same rule as
             portFarLodSet in src/semantic/port_variant_arrays.c): the node's largest-max record, when it
             draws and no drawing record of its set starts at or beyond that max. A set is the LOD
             children of one node whose children are all drawn (opcodes 1, 4, 5, 12), else one LOD node:
             split LODs keep each band in a node of its own ([0,200) | [200,500) | [500,1000)).

`--summary` prints one line per file and the totals: LOD nodes, top distances, records extended, pickups
under LODs, zone nodes (opcode 11: hide = only the outside child draws, show = only the inside one, swap =
both, none = neither) and the whole-file cost (meshes / primitives) the walker would queue with every LOD
at its nearest record (d2 = 0, like the override), and past every record in retail and with the switch on
(zones: the outside child, i.e. a camera outside every region; animations: the current frame; selects:
the current child). Upper bounds: frustum and far culling keep only part of a file.

Usage:
  py -3.14 tools/dmd_lod_dump.py [--bin PATH] [--files FREEWAY,RK,...] [--summary] [--out FILE]
"""
import argparse
import math
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_BIN = os.path.join(ROOT, "cdimage", "Twisted Metal 2 (USA) (Track 01).bin")
UNBOUNDED = 0xFFFFFFFF
LEVELS = ["FREEWAY", "DISH", "PARIS", "PARIS2", "ISLANDS", "ROOF", "ROOF2", "GLACIER", "GLACIER2",
          "DEN", "HKONG", "SWAMP", "BURB", "SROOF"]
PICKUP_IDS = set(range(401, 431)) | set(range(441, 481))
ALL_CHILDREN = (1, 4, 5, 12)      # opcodes whose children are all walked


class Disc:
    """Read-only ISO 9660 reader for a MODE2/2352 track (user data at +24 of each 2352-byte sector)."""

    def __init__(self, path):
        self.f = open(path, "rb")
        self.files = {}
        pvd = self.sector(16)
        root = pvd[156:156 + 34]
        self._walk(struct.unpack_from("<I", root, 2)[0], struct.unpack_from("<I", root, 10)[0], "")

    def sector(self, lba):
        self.f.seek(lba * 2352 + 24)
        return self.f.read(2048)

    def read(self, lba, size):
        out = bytearray()
        while len(out) < size:
            out += self.sector(lba)
            lba += 1
        return bytes(out[:size])

    def _walk(self, lba, size, path):
        data = self.read(lba, size)
        i = 0
        while i < len(data):
            n = data[i]
            if n == 0:
                i = (i // 2048 + 1) * 2048
                continue
            rec = data[i:i + n]
            i += n
            name = rec[33:33 + rec[32]].decode("latin1")
            if name in ("\x00", "\x01"):
                continue
            p = path + "/" + name.split(";")[0]
            elba, esz = struct.unpack_from("<I", rec, 2)[0], struct.unpack_from("<I", rec, 10)[0]
            if rec[25] & 2:
                self._walk(elba, esz, p)
            else:
                self.files[p.upper()] = (elba, esz)

    def get(self, path):
        lba, size = self.files[path.upper()]
        return self.read(lba, size)


class Dmd:
    """A PS1 DMD image ("PSXD", version 0x43): pointers built for the address at +0x0C (0x80019C40),
    roots at +0x1C (count at +0x18). Node layouts as the walker reads them."""

    def __init__(self, name, data):
        self.name, self.d = name, data
        if struct.unpack_from("<I", data, 0)[0] != 0x50535844:
            raise ValueError("%s: not a PSXD image" % name)
        self.origin = self.u32(0x0C)
        count = self.u32(0x18)
        self.roots = []
        for i in range(count):
            r = self.u32(0x1C + 4 * i)
            if r == 0:
                break
            self.roots.append(r)
        self._draws, self._pickup = {}, {}

    def u16(self, o):
        return struct.unpack_from("<H", self.d, o)[0]

    def u32(self, o):
        return struct.unpack_from("<I", self.d, o)[0]

    def off(self, p):
        o = p - self.origin
        if not 0 <= o < len(self.d):
            raise ValueError("%s: pointer %08X outside the image" % (self.name, p))
        return o

    def type(self, p):
        return self.d[self.off(p)]

    def children(self, p):
        """Every child the walker can push from node p (all frames, both zone children, every record)."""
        o = self.off(p)
        t = self.d[o]
        ptrs = lambda a, n: [self.u32(a + 4 * i) for i in range(n)]
        if t == 1:
            return ptrs(o + 24, self.d[o + 20])
        if t == 2:
            return [r[2] for r in self.records(p)]
        if t == 3:
            return ptrs(o + 32, self.d[o + 11])
        if t == 4:
            return ptrs(o + 20, self.d[o + 18])
        if t == 5:
            return ptrs(o + 40, self.d[o + 38])
        if t == 9:
            return ptrs(o + 16, self.d[o + 6])
        if t == 11:
            return [self.u32(o + 8), self.u32(o + 12)]
        if t == 12:
            return ptrs(o + 8, self.u16(o + 6))
        return []

    def records(self, p):
        """(max, min, child, record address) of LOD node p, in file order (the walker's order)."""
        o = self.off(p)
        out = []
        for i in range(min(struct.unpack_from("<i", self.d, o + 16)[0], 32)):
            r = self.u32(o + 20 + 4 * i)
            ro = self.off(r)
            out.append((self.u32(ro), self.u32(ro + 4), self.u32(ro + 8), r))
        return out

    def draws(self, p):
        """Does the walker reach a mesh (opcode 0) below p, whatever the run-time state?"""
        if p == 0:
            return False
        if p not in self._draws:
            self._draws[p] = False          # a cycle counts as empty
            self._draws[p] = self.type(p) == 0 or any(self.draws(c) for c in self.children(p))
        return self._draws[p]

    def has_pickup(self, p):
        if p == 0:
            return False
        if p not in self._pickup:
            self._pickup[p] = False
            o = self.off(p)
            self._pickup[p] = (self.d[o] in (1, 4) and self.u16(o + 2) in PICKUP_IDS) or \
                any(self.has_pickup(c) for c in self.children(p))
        return self._pickup[p]

    def nodes(self):
        """Every node reachable from the roots, once, with the parents it was reached from."""
        parents, order = {}, []
        stack = [(r, None) for r in reversed(self.roots)]
        while stack:
            p, parent = stack.pop()
            if p in parents:
                parents[p].append(parent)
                continue
            parents[p] = [parent]
            order.append(p)
            for c in reversed(self.children(p)):
                if c:
                    stack.append((c, p))
        return order, parents

    def beams_only(self, p, depth=0):
        """1: every mesh the walker can reach below p holds only beams (primitive kind 1) and there is one; -1: another
        mesh is reached; 0: none (portFarLodBeams)."""
        if p == 0 or depth > 32:
            return 0
        o = self.off(p)
        if self.d[o] == 0:
            n, q = self.u32(o + 16), self.off(self.u32(o + 12))
            if n == 0 or n & 0x80000000:
                return -1
            for _ in range(n):
                if (self.u32(q + 12) >> 16) & 7 != 1:
                    return -1
                q += self.d[q + 2] * 4
            return 1
        r = 0
        for c in self.children(p):
            k = self.beams_only(c, depth + 1)
            if k < 0:
                return -1
            r = r or k
        return r

    def kept_retail(self):
        """Request 90 (RENDER_ANSWERS.md R6.1): the LOD nodes the far-LOD scan leaves at their retail bands (neither
        scaled nor extended): beam-only LODs reached, from their nearest LOD above, through a record with a coarser
        version (a drawing record of that LOD starting at or beyond its max): Holland's tall grass."""
        kept, seen = set(), set()
        stack = [(r, False) for r in self.roots]
        while stack:
            p, coarse = stack.pop()
            if not p or (p, coarse) in seen:
                continue
            seen.add((p, coarse))
            if self.type(p) == 2:
                if coarse and self.beams_only(p) > 0:
                    kept.add(p)
                rs = self.records(p)
                for r in rs:
                    c = any(q is not r and q[1] >= r[0] and self.draws(q[2]) for q in rs)
                    stack.append((r[2], c))
            else:
                stack += [(c, coarse) for c in self.children(p)]
        return kept

    def fine_field(self):
        """Request 91 (FAR_LOD.md 13): (fine, never), the record addresses the far-LOD scan flags (bit 1) for a beam
        field's finest level: the records of every kept_retail() node and, on each walk path to one, every LOD record
        with a coarser version (portFarLodWalk's path) are fine (with portFarLod: [0, ..)); those records' coarser
        versions are never (with portFarLod: max 0)."""
        fine, never, seen = set(), set(), set()
        stack = [(r, False, ()) for r in self.roots]
        while stack:
            p, coarse, path = stack.pop()
            if not p or (p, coarse, path) in seen:
                continue
            seen.add((p, coarse, path))
            if self.type(p) == 2:
                rs = self.records(p)
                if coarse and self.beams_only(p) > 0:
                    fine |= {r[3] for r in rs}
                    for l, i in path:
                        ls = self.records(l)
                        fine.add(ls[i][3])
                        never |= {q[3] for j, q in enumerate(ls)
                                  if j != i and q[1] >= ls[i][0] and self.draws(q[2])}
                for i, r in enumerate(rs):
                    c = any(q is not r and q[1] >= r[0] and self.draws(q[2]) for q in rs)
                    stack.append((r[2], c, path + ((p, i),) if c else path))
            else:
                stack += [(c, coarse, path) for c in self.children(p)]
        return fine, never

    def extended(self):
        """{record address: LOD node} of the records the far-LOD switch extends (portFarLodSet's rule; request 90: not
        the kept_retail() nodes')."""
        order, _ = self.nodes()
        ext = {}
        kept = self.kept_retail()

        def do_set(lods):
            lods = [l for l in lods if l and self.type(l) == 2]
            recs = [r for l in lods for r in self.records(l)]
            for l in lods:
                rs = self.records(l)
                if not rs or l in kept:
                    continue
                top = rs[0]
                for r in rs[1:]:
                    if r[0] > top[0]:
                        top = r
                if top[0] == UNBOUNDED or not self.draws(top[2]):
                    continue
                if any(r[1] >= top[0] and self.draws(r[2]) for r in recs):
                    continue
                ext[top[3]] = l

        for r in self.roots:
            do_set([r])
        for p in order:
            kids = [c for c in self.children(p) if c]
            if self.type(p) in ALL_CHILDREN:
                do_set(kids)
            else:
                for c in kids:
                    do_set([c])
        return ext

    def cost(self, p, mode, ext, memo):
        """(meshes, primitives) queued below p. mode: 'near' every LOD at the record holding d2 = 0;
        'past' past every record (retail: nothing; with ext: the extended record). Animations: frame
        +12; selects: child +7; zones: the outside child (+8)."""
        key = (p, mode)
        if key in memo:
            return memo[key]
        memo[key] = (0, 0)
        o = self.off(p)
        t = self.d[o]
        if t == 0:
            res = (1, self.u32(o + 16))
        else:
            if t == 2:
                rs = self.records(p)
                if mode == "near":
                    kids = [r[2] for r in rs if r[1] == 0][:1]
                else:
                    kids = [r[2] for r in rs if r[3] in ext][:1]
            elif t == 3:
                kids = self.children(p)[self.d[o + 12]:self.d[o + 12] + 1]
            elif t == 9:
                kids = self.children(p)[self.d[o + 7]:self.d[o + 7] + 1]
            elif t == 11:
                kids = [self.u32(o + 8)]
            else:
                kids = self.children(p)
            res = (0, 0)
            for c in kids:
                if c:
                    m, n = self.cost(c, mode, ext, memo)
                    res = (res[0] + m, res[1] + n)
        memo[key] = res
        return res


def dist(v):
    return "inf" if v == UNBOUNDED else "%d" % round(math.sqrt(v))


def bucket(v):
    if v == UNBOUNDED:
        return "inf"
    d = math.sqrt(v)
    return "<1000" if d < 1000 else "<2400" if d < 2400 else "<4800" if d < 4800 else ">=4800"


BUCKETS = ["<1000", "<2400", "<4800", ">=4800", "inf"]


def analyse(name, dmd):
    order, parents = dmd.nodes()
    ext = dmd.extended()
    lods = [p for p in order if dmd.type(p) == 2]
    info = []
    for p in lods:
        rs = dmd.records(p)
        flags = []
        if not any(dmd.draws(r[2]) for r in rs):
            flags.append("NONVISUAL")
        else:
            if max(r[0] for r in rs) != UNBOUNDED:
                flags.append("FINITE")
            if not any(r[1] <= 100 < r[0] for r in rs):
                flags.append("NEARGAP")
            if any(dmd.has_pickup(r[2]) for r in rs):
                flags.append("PICKUP")
            if any(r[3] in ext for r in rs):
                flags.append("EXT")
        info.append(dict(node=p, id=dmd.u16(dmd.off(p) + 2), recs=rs, flags=flags))
    zones = [p for p in order if dmd.type(p) == 11]
    zk = {"hide": 0, "show": 0, "swap": 0, "nonvisual": 0}
    for z in zones:
        o = dmd.off(z)
        out, inn = dmd.draws(dmd.u32(o + 8)), dmd.draws(dmd.u32(o + 12))
        zk["swap" if out and inn else "hide" if out else "show" if inn else "nonvisual"] += 1
    s = dict(lod=len(lods), visual=sum("NONVISUAL" not in i["flags"] for i in info),
             finite=sum("FINITE" in i["flags"] for i in info), neargap=sum("NEARGAP" in i["flags"] for i in info),
             ext=len(ext), pickup_lods=sum("PICKUP" in i["flags"] for i in info),
             tops=dict.fromkeys(BUCKETS, 0), zones=len(zones), zk=zk)
    for i in info:
        if "NONVISUAL" not in i["flags"]:
            s["tops"][bucket(max(r[0] for r in i["recs"]))] += 1
    for mode in ("near", "past"):
        for label, e in (("retail", {}), ("farlod", ext)):
            if mode != "past" and label == "farlod":
                continue
            memo, tot = {}, (0, 0)
            for r in dmd.roots:
                m, n = dmd.cost(r, mode, e, memo)
                tot = (tot[0] + m, tot[1] + n)
            s["%s_%s" % (mode, label)] = tot
    return info, ext, s


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--bin", default=DEFAULT_BIN, help="disc image track 1 (MODE2/2352)")
    ap.add_argument("--files", help="comma-separated DMD stems (default: every DMD\\*.DMD)")
    ap.add_argument("--summary", action="store_true", help="one line per file and the totals only")
    ap.add_argument("--out", help="write to this file instead of stdout")
    a = ap.parse_args()
    disc = Disc(a.bin)
    names = sorted(p.split("/")[-1][:-4] for p in disc.files if p.startswith("/DMD/") and p.endswith(".DMD"))
    if a.files:
        names = [n.upper() for n in a.files.split(",")]
    out = open(a.out, "w", newline="\n") if a.out else sys.stdout
    out.write("%-9s %-5s %5s %6s %6s %7s %4s %6s  %-28s %-26s %-11s %-11s %-11s\n" % (
        "file", "kind", "LOD", "visual", "finite", "neargap", "ext", "pickup",
        "top <1000/2400/4800/more/inf", "zones: hide/show/swap/none", "d2=0", "past retail", "past switch"))
    totals = {}
    for name in names:
        dmd = Dmd(name, disc.get("/DMD/%s.DMD" % name))
        info, ext, s = analyse(name, dmd)
        kind = "level" if name in LEVELS else "car"
        t = totals.setdefault(kind, dict(files=0, lod=0, visual=0, finite=0, neargap=0, ext=0, pickup_lods=0,
                                          zones=0, ext_max=0))
        t["files"] += 1
        for k in ("lod", "visual", "finite", "neargap", "ext", "pickup_lods", "zones"):
            t[k] += s[k]
        t["ext_max"] = max(t["ext_max"], s["ext"])
        out.write("%-9s %-5s %5d %6d %6d %7d %4d %6d  %-28s %-26s %-11s %-11s %-11s\n" % (
            name, kind, s["lod"], s["visual"], s["finite"], s["neargap"], s["ext"], s["pickup_lods"],
            "/".join(str(s["tops"][b]) for b in BUCKETS),
            "%d: %d/%d/%d/%d" % (s["zones"], s["zk"]["hide"], s["zk"]["show"], s["zk"]["swap"], s["zk"]["nonvisual"]),
            "%d/%d" % s["near_retail"], "%d/%d" % s["past_retail"],
            "%d/%d" % s["past_farlod"]))
        if a.summary:
            continue
        for i in info:
            out.write("  %08X id %4d  %s\n" % (i["node"], i["id"], " ".join(i["flags"])))
            for mx, mn, child, addr in i["recs"]:
                out.write("      [%10d, %10s)  dist [%5s, %5s)  child %08X %-6s%s\n" % (
                    mn, "inf" if mx == UNBOUNDED else mx, dist(mn), dist(mx), child,
                    "draws" if dmd.draws(child) else "empty", "  ext" if addr in ext else ""))
    out.write("totals (meshes/primitives columns: whole file, every root; zones outside):\n")
    for kind, t in sorted(totals.items()):
        out.write("  %-5s files %2d  LOD nodes %5d  visual %5d  finite top %5d  near gap %3d  extended %5d "
                  "(most in one file %d)  LOD nodes holding pickups %3d  zone nodes %3d\n" % (
                      kind, t["files"], t["lod"], t["visual"], t["finite"], t["neargap"], t["ext"], t["ext_max"],
                      t["pickup_lods"], t["zones"]))
    if a.out:
        out.close()


if __name__ == "__main__":
    main()
