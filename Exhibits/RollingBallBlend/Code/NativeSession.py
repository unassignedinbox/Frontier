#!/usr/bin/env python3
"""Minimal client for the SolidArc C++ geometry service (/native on port 8081): the same JSON protocol the browser uses."""
import json, sys, urllib.request, uuid

class NativeSession:
    def __init__(Self, Base='http://127.0.0.1:8081'):
        Self.Base, Self.Identity, Self.Token = Base, 'proof' + uuid.uuid4().hex[:12], 0
    def Request(Self, Action, **Fields):
        Self.Token += 1
        Body = json.dumps({'action': Action, 'width': 960, 'height': 600, 'token': Self.Token, **Fields}).encode()
        Call = urllib.request.Request(Self.Base + '/native', data=Body, method='POST',
                                      headers={'Content-Type': 'application/json', 'X-SolidArc-Session': Self.Identity})
        with urllib.request.urlopen(Call, timeout=300) as Reply:
            return json.loads(Reply.read())
    def Commands(Self, *Lines):
        return Self.Request('command', commands=list(Lines))
    def Close(Self):
        urllib.request.urlopen(urllib.request.Request(Self.Base + '/native', method='DELETE', headers={'X-SolidArc-Session': Self.Identity}), timeout=30)

if __name__ == '__main__':
    S = NativeSession(); S.Request('create')
    for Line in sys.argv[1:]:
        R = S.Commands(Line)
        print(Line, '->', R['success'], (R.get('description', {}).get('status') or R.get('notice') or '')[:200])
    print(S.Request('save')['file'])
