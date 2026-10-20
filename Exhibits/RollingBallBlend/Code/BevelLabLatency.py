#!/usr/bin/env python3
"""Interactive-latency benchmark of the live C++ geometry service on BevelLab.arc: scene load, idle frames, selection."""
import sys, time, json, pathlib, urllib.request
sys.path.insert(0, str(pathlib.Path(__file__).parent))
from NativeSession import NativeSession
References = '--full' not in sys.argv
Session = NativeSession()
def Post(Body):
    Start = time.perf_counter()
    Data = json.dumps({'width': 960, 'height': 600, 'geometryReferences': References, **Body}).encode()
    Reply = urllib.request.urlopen(urllib.request.Request(Session.Base + '/native', data=Data, method='POST',
            headers={'Content-Type': 'application/json', 'X-SolidArc-Session': Session.Identity}), timeout=300).read()
    return len(Reply), (time.perf_counter() - Start) * 1000, json.loads(Reply)
Post({'action': 'create', 'geometryReset': True})
Scene = [L.strip() for L in pathlib.Path(sys.argv[1]).read_text().splitlines() if L.strip() and not L.startswith('#')]
Rows = []
Size, Ms, R = Post({'action': 'command', 'commands': Scene}); Rows.append(('scene load (one batch)', Size, Ms, R['success']))
for K in range(3): Size, Ms, R = Post({'action': 'render'}); Rows.append((f'idle frame {K+1}', Size, Ms, R['success']))
for Name in ['RoundedBlock', 'BossPlate', 'none']:
    Size, Ms, R = Post({'action': 'command', 'commands': [f'select {Name}']}); Rows.append((f'select {Name}', Size, Ms, R['success']))
    Size, Ms, R = Post({'action': 'command', 'commands': [f'select {Name}']}); Rows.append((f'select {Name} again', Size, Ms, R['success']))
print(f"| Request ({'digest transport' if References else 'full transport'}) | Reply | Latency | OK |\n|---|---|---|---|")
for Name, Size, Ms, Ok in Rows: print(f"| {Name} | {Size/1e6:.3f} MB | {Ms:.0f} ms | {'✔️' if Ok else '🔴'} |")
