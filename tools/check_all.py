"""Run every static check that applies to a build and print one summary (HUMANS16_PLAN.md H6; SOURCE_BUILD.md
"Static checks of the port options").

    py -3.14 tools/check_all.py                          # the latest full build (work/build-*)
    py -3.14 tools/check_all.py work/build-<stamp>       # a full build: its port variant and placement links
    py -3.14 tools/check_all.py work/pv/<tag>_P4 [...]   # port variant links (build_source_only.py --output)
    py -3.14 tools/check_all.py work/pv/<tag>            # <tag>_P4 and <tag>_P2 if they exist
    options: --e0 DIR (the E0 link the proofs compare with), --no-proof, --models (the disc models of
             check_car_groups.py), -j N (checks run in parallel, default 4), -v (print every check's output)

A full build directory (build.py --relocatable --port-variant) contributes its retail images (hybrid, source-only,
--sdk-from-lib: sha256 against BUILD_MANIFEST.json), its port variant link (source_reloc_port) and its placement link
(source_reloc_port2); its E0 is its own source_reloc_e0. A port variant link outside a full build is compared with
--e0, else with the latest full build's E0 (the objects build_source_only.py --build-dir last links).

What runs, by the link's port defines (report.json port_variant.port_defines):
  proof        tools/verify_reloc.py LINK --e0 E0 (every link; un-relocating it gives E0 outside the changed
               functions)
  placement    a link with the limits forced to 2 (--port-limits 2): the changed functions are exactly the
               $gp users of moved arrays plus the port defines' own (build.py's placement check)
  humans       tools/check_humans.py with every limits-4 link (every port variant: portHumanOfId; the H1-H6 checks
               of the links built with TM2_PORT_HUMANS); its source checks run once
  car groups   tools/check_car_groups.py LINK --expect-functions when a car define is in it (TM2_PORT_CAR_GROUPS,
               _CARS, _CAR_SWAP, _TEXTURE_BANKS, _CAR_SPECIALS, _DARK_TOOTH, _CAR_SOUND, _CAR_CLUTS, _CAR_DROPIN,
               _SPECIAL_AS, _DT_PLAYER)
  hazards      tools/check_hazards.py LINK --expect-functions with TM2_PORT_HAZARDS
  balance      tools/check_balance.py LINK --expect-functions with TM2_PORT_BALANCE
  rollback     tools/check_rollback.py LINK --expect-functions --e0 E0 with TM2_PORT_ROLLBACK (request 48: the
               online stage-1 to stage-3 gates, no new rand(), the srand sites on the port's seed, the HOLD and split
               latches, the split step's order, the sound gate, the palette shadow, rollback_regions.tsv against
               the step's stores; request 49's placed cars with TM2_PORT_CARS; the drawing's and the view side's
               stores through pointers and this machine's humans, request 58)
  custom       tools/check_custom.py LINK --expect-functions --e0 E0 with TM2_PORT_DARK_TOOTH, _DT_PLAYER, _VISUAL or
               _CAR_NUMBERS (request 42: the ordinary Dark Tooth's bit-10 gate, the arcs' depth gate and rand() census,
               the car-number and car-select functions' callers); request 46 also with TM2_PORT_FAR_LOD or
               _FAR_DISTANCE (check 5: portFarLodNearScale, portMirrorFar,
               the render capacity counter portRenderStats) and the visual bits 3 / 4; request 47 also with
               TM2_PORT_RENDER_SLOTS (check 6: the C walker, the slot arrays, the packet buffer sets); request 61
               also with TM2_PORT_FAR_DISTANCE (check 7: the range pass's sprites) and its addendum with
               TM2_PORT_C_EMITTER (check 8: the C emitter against the asm on a MIPS interpreter, its precise corners)
  models       with --models: tools/check_car_groups.py --dmd --key-model --bank-model --clut-model --unload-model
               (the disc)
TM2_PORT_SPU48 has no check tool of its own: the proof and the placement check cover it (TM2_PORT_FAR_LOD and
TM2_PORT_FAR_DISTANCE: request 46's gates in check_custom.py check 5, the rest the proof and the placement check). Every check's full output goes to work/check_all/<link>.<check>.log.

Exit status 0 when every check passes."""
import argparse
import hashlib
import json
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT))
PY = sys.executable
LOGS = ROOT / 'work' / 'check_all'
CAR_DEFINES = {'TM2_PORT_CAR_GROUPS', 'TM2_PORT_CARS', 'TM2_PORT_CAR_SWAP', 'TM2_PORT_TEXTURE_BANKS',
               'TM2_PORT_CAR_SPECIALS', 'TM2_PORT_DARK_TOOTH', 'TM2_PORT_CAR_SOUND', 'TM2_PORT_CAR_CLUTS',
               'TM2_PORT_CAR_DROPIN', 'TM2_PORT_SPECIAL_AS', 'TM2_PORT_DT_PLAYER'}
# request 42 (CUSTOM_TOURNAMENT_ANSWERS.md): the ordinary Dark Tooth (bit 10), the extended car select, car numbers,
# the visual switches
CUSTOM_DEFINES = {'TM2_PORT_DARK_TOOTH', 'TM2_PORT_DT_PLAYER', 'TM2_PORT_VISUAL', 'TM2_PORT_CAR_NUMBERS',
                  'TM2_PORT_FAR_LOD', 'TM2_PORT_FAR_DISTANCE',   # request 46: check_custom.py check 5
                  'TM2_PORT_RENDER_SLOTS',                       # request 47: check 6 (the matrix slots)
                  'TM2_PORT_C_EMITTER',                          # request 61 addendum: check 8 (the C emitter)
                  'TM2_PORT_FALL_WHEELS',                        # request 107: check 20 (the pit death)
                  'TM2_PORT_CAR_POSE',                           # request 108: check 21 (a car posed)
                  'TM2_PORT_WHOLE_MAPS',                         # request 111: check 23 (the floes)
                  'TM2_PORT_FWY_PARAPETS', 'TM2_PORT_FWY_GROUND',  # request 112: check 24 (LA's ground)
                  'TM2_PORT_MOD_ARENA',                           # request 114: check 26 (mod arena rules)
                  'TM2_PORT_WRECK_FALL',                          # request 117: check 28 (a falling wreck)
                  'TM2_PORT_GLACIER_FIRE'}                        # request 120: check 30 (the fire missile)
# full build: image -> BUILD_MANIFEST.json key
RETAIL_IMAGES = [('psx/TM2_MAXIMAL_HYBRID.EXE', 'expected_image_sha256', 'hybrid'),
                 ('source/TM2_SOURCE.EXE', 'expected_source_only_sha256', 'source-only'),
                 ('source_sdklib/TM2_SOURCE.EXE', 'expected_source_only_sdk_lib_sha256', 'sdk-lib')]


def rel(p):
    try:
        return Path(p).resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return Path(p).as_posix()


def latest_build():
    builds = sorted(p for p in (ROOT / 'work').glob('build-*') if (p / 'source_reloc_e0' / 'report.json').exists())
    return builds[-1] if builds else None


def is_link(p):
    return (p / 'report.json').exists() and (p / 'functions.tsv').exists()


def port_variant(link):
    return json.loads((link / 'report.json').read_text(encoding='utf-8')).get('port_variant') or {}


def collect(args):
    """[(kind, path)]: kind 'build' (a full build directory) or 'link'."""
    targets = args.targets or ['last']
    out = []
    for t in targets:
        p = latest_build() if t == 'last' else (Path(t) if Path(t).is_absolute() else ROOT / t)
        if p is None:
            raise SystemExit('no full build in work/ (build.py --source-only --relocatable --port-variant)')
        if (p / 'source_reloc_e0').is_dir():
            out.append(('build', p))
        elif p.is_dir() and is_link(p):
            out.append(('link', p))
        else:
            found = [p.parent / (p.name + s) for s in ('_P4', '_P2') if is_link(p.parent / (p.name + s))]
            if not found:
                raise SystemExit('%s: neither a full build directory nor a link directory (report.json)' % t)
            out.extend(('link', f) for f in found)
    return out


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(cmd, log):
    t0 = time.time()
    r = subprocess.run([PY, '-u'] + cmd, cwd=ROOT, capture_output=True, text=True, encoding='utf-8',
                       errors='replace')
    text = r.stdout + (('\n[stderr]\n' + r.stderr) if r.stderr.strip() else '')
    log.write_text(text, encoding='utf-8', newline='\n')
    return r.returncode, text, time.time() - t0


def last_line(text):
    lines = [l for l in text.splitlines() if l.strip() and not l.startswith('[stderr]')]
    return lines[-1].strip() if lines else ''


def fail_lines(text, limit=12):
    bad = [l.rstrip() for l in text.splitlines() if l.startswith('FAIL') or ' FAIL' in l or 'Traceback' in l
           or l.startswith('Error') or 'error:' in l.lower()]
    return bad[:limit]


def placement(link):
    """build.py's placement check on a limits-2 link (its proof is the 'proof' job)."""
    import build
    pv = port_variant(link)
    changed = {s['symbol'] for s in pv['changed_sections'] if s['code']}
    port = {s['symbol'] for s in pv['changed_sections'] if s.get('port_changed')}
    expected = build.gp_functions_of_moved_arrays()
    data = [s['symbol'] for s in pv['changed_sections'] if not s['code'] and not s.get('port_changed')]
    unexpected, missing = sorted(changed - expected - port), sorted(expected - changed)
    ok = not unexpected and not missing and not data
    return ok, ('%d changed functions = %d $gp users of moved arrays + %d of the port defines; unexpected %s, '
                'missing %s, changed data %s' % (len(changed), len(expected & changed), len(port), unexpected or 'none',
                                                missing or 'none', data or 'none'))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('targets', nargs='*', help='full build directories, link directories, work/pv/<tag> or last')
    ap.add_argument('--e0', help='E0 link for the proofs of links outside a full build (default: the latest build\'s)')
    ap.add_argument('--no-proof', action='store_true', help='skip verify_reloc.py')
    ap.add_argument('--models', action='store_true', help='also run check_car_groups.py\'s disc model checks')
    ap.add_argument('-j', type=int, default=4, help='checks run in parallel (default 4)')
    ap.add_argument('-v', '--verbose', action='store_true', help='print every check\'s full output')
    a = ap.parse_args()
    LOGS.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((ROOT / 'BUILD_MANIFEST.json').read_text(encoding='utf-8'))
    results = []          # (target, check, ok, seconds, summary, failures, log)
    jobs = []             # (target, check, cmd, log, parse)
    links = []            # (link, e0)
    for kind, p in collect(a):
        if kind == 'build':
            for image, key, label in RETAIL_IMAGES:
                f = p / image
                if not f.exists():
                    results.append((rel(p), 'retail ' + label, False, 0, '%s missing' % image, [], None))
                    continue
                h = sha(f)
                results.append((rel(p), 'retail ' + label, h == manifest[key], 0, '%s %s (want %s)' % (
                    image, h[:12], manifest[key][:12]), [], None))
            for name in ('source_reloc_port', 'source_reloc_port2'):
                if is_link(p / name):
                    links.append((p / name, p / 'source_reloc_e0'))
        else:
            if a.e0:
                e0 = Path(a.e0) if Path(a.e0).is_absolute() else ROOT / a.e0
            elif (p.parent / 'source_reloc_e0').is_dir():
                e0 = p.parent / 'source_reloc_e0'
            else:
                b = latest_build()
                e0 = b / 'source_reloc_e0' if b else None
            links.append((p, e0))
    humans = []
    for link, e0 in links:
        tag = rel(link)
        stem = tag.replace('/', '_')
        pv = port_variant(link)
        if not pv:
            results.append((tag, 'port variant', False, 0, 'not a port variant link', [], None))
            continue
        names = {d.split('=')[0] for d in pv.get('port_defines') or []}
        forced = (pv.get('limits') or {}).get('humans') == 2
        if not a.no_proof:
            if e0 is None:
                results.append((tag, 'proof', False, 0, 'no E0 link (--e0)', [], None))
            else:
                image = json.loads((link / 'report.json').read_text(encoding='utf-8')).get('sha256') or ''
                jobs.append((tag, 'proof', ['tools/verify_reloc.py', str(link), '--e0', str(e0)],
                             LOGS / (stem + '.proof.log'), 'image %s, vs %s' % (image[:12], rel(e0))))
        if forced:
            t0 = time.time()
            ok, text = placement(link)
            results.append((tag, 'placement', ok, time.time() - t0, text, [], None))
            continue
        humans.append(str(link))
        if names & CAR_DEFINES:
            jobs.append((tag, 'car groups', ['tools/check_car_groups.py', str(link), '--expect-functions'],
                         LOGS / (stem + '.car_groups.log'), None))
        if 'TM2_PORT_HAZARDS' in names:
            jobs.append((tag, 'hazards', ['tools/check_hazards.py', str(link), '--expect-functions'],
                         LOGS / (stem + '.hazards.log'), None))
        if 'TM2_PORT_BALANCE' in names:
            jobs.append((tag, 'balance', ['tools/check_balance.py', str(link), '--expect-functions'],
                         LOGS / (stem + '.balance.log'), None))
        if 'TM2_PORT_ROLLBACK' in names:
            cmd = ['tools/check_rollback.py', str(link), '--expect-functions'] + (['--e0', str(e0)] if e0 else [])
            jobs.append((tag, 'rollback', cmd, LOGS / (stem + '.rollback.log'), None))
        if names & CUSTOM_DEFINES:
            cmd = ['tools/check_custom.py', str(link), '--expect-functions'] + (['--e0', str(e0)] if e0 else [])
            jobs.append((tag, 'custom', cmd, LOGS / (stem + '.custom.log'), None))
    if humans:
        jobs.append((', '.join(rel(h) for h in humans), 'humans', ['tools/check_humans.py'] + humans,
                     LOGS / 'humans.log', None))
    if a.models:
        jobs.append(('(disc)', 'car models', ['tools/check_car_groups.py', '--dmd', '--key-model', '--bank-model',
                                              '--clut-model', '--unload-model'], LOGS / 'car_models.log', None))

    def go(job):
        target, check, cmd, log, note = job
        rc, text, secs = run(cmd, log)
        summary = last_line(text)
        if note:
            summary = '%s: %s' % (note, summary)
        return target, check, rc == 0, secs, summary, ([] if rc == 0 else fail_lines(text)), log, text

    print('check_all: %d links, %d jobs (logs in %s)' % (len(links), len(jobs), rel(LOGS)))
    with ThreadPoolExecutor(max_workers=max(1, a.j)) as pool:
        for target, check, ok, secs, summary, bad, log, text in pool.map(go, jobs):
            if a.verbose:
                print('==== %s: %s\n%s' % (target, check, text))
            results.append((target, check, ok, secs, summary, bad, log))
    print()
    width = max(len(r[0]) for r in results) if results else 10
    width = min(width, 60)
    for target, check, ok, secs, summary, bad, log in results:
        print('%-4s %-*s %-18s %5.0fs  %s' % ('ok' if ok else 'FAIL', width, target, check, secs, summary))
        for b in bad:
            print('       %s' % b)
        if not ok and log:
            print('       full output: %s' % rel(log))
    good = all(r[2] for r in results) and bool(results)
    print('\nCHECK ALL %s (%d checks, %d failed)' % ('OK' if good else 'FAILED', len(results),
                                                      sum(1 for r in results if not r[2])))
    return 0 if good else 1


if __name__ == '__main__':
    sys.exit(main())
