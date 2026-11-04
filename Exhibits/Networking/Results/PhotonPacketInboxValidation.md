# Photon Packet Inbox Validation

| Check | Result |
|---|---|
| Valid decoded frame copies payload and room sender slot | Pass |
| Arrival order and independent callback memory | Pass |
| Truncated frame and invalid sender rejected | Pass |
| Bounded 256-packet queue rejects overflow without displacing accepted packets | Pass |
| Drain, clear and unlinked Photon stub | Pass |

11 / 11 checks passed on Linux x86-64, GCC 12.2 with AddressSanitizer and UndefinedBehaviorSanitizer, at `4e2ea03`
plus the uncommitted Photon inbox changes. The real Photon implementation also passed a Linux syntax-only compile against
the pinned `defold/extension-photon-realtime` headers at `8dad867470f2eb75aeae0c437e10ee723d8d1ccf`; it was not linked
or run against the proprietary SDK.

```bash
g++ -std=c++20 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -I. \
  Frontier/Projects/Project-Networking/Source/PhotonLinkStub.cpp Exhibits/Networking/Code/PhotonPacketInboxValidation.cpp \
  -o _ClaudeScratch/build/PhotonPacketInboxValidation
ASAN_OPTIONS=detect_leaks=1 _ClaudeScratch/build/PhotonPacketInboxValidation
```

The callback enqueues decoded frames for an eventual consumer on the transport tick thread. Nobody in the editor currently
drains this queue into `NetworkingExtension`. A Photon room slot is not an authenticated engine peer identity; the queue
is not authority validation. This is not an online replication test, delivery acknowledgement, or evidence that a game
session works. EOS direct packets remain unimplemented; real Photon libraries, credentials, room policies and two-machine
validation remain necessary.
