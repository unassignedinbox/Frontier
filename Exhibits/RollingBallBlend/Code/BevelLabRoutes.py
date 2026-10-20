#!/usr/bin/env python3
"""Replays BevelLab.arc against the live C++ geometry service and times every bevel. Run once per SOLIDARC_BLEND route."""
import sys, time, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).parent))
from NativeSession import NativeSession
Scene = pathlib.Path(sys.argv[1]).read_text().splitlines()
Session = NativeSession(); Session.Request('create')
for Line in (L.strip() for L in Scene):
    if not Line or Line.startswith('#'): continue
    Start = time.perf_counter(); Reply = Session.Commands(Line); Elapsed = time.perf_counter() - Start
    if Line.startswith('bevel'): print(f"{'OK ' if Reply['success'] else 'REFUSED'} {Elapsed*1000:8.0f} ms  {Line}")
print('log', Session.Identity)
