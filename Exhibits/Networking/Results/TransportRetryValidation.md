# Transport Retry Validation

| Check | Result |
|---|---|
| Rejected sends remain queued across repeated exchanges | Pass |
| Incoming traffic is still received while sending is blocked | Pass |
| Recovered sends retain order and sequence numbers | Pass |
| Accepted sends are not sent twice | Pass |

8 / 8 checks passed on Linux x86-64, GCC 12.2, with the uncommitted networking changes atop `61c1064`.

```bash
g++ -std=c++20 -Wall -Wextra -Werror -O1 -ffunction-sections -fdata-sections -I. -IFrontier \
  -IExternalPackages/tomlpp/include Frontier/Extensions/Networking/NetworkingSpace.cpp \
  Frontier/Extensions/Networking/NetworkingExtension.cpp Exhibits/Networking/Code/TransportRetryValidation.cpp \
  -Wl,--gc-sections -o _ClaudeScratch/build/TransportRetryValidation
_ClaudeScratch/build/TransportRetryValidation
```

This only retries when the transport immediately rejects a send. Transport acceptance is not remote delivery, and the
retry queue currently has no size limit. EOS gameplay packets, Photon gameplay receive, acknowledgements, remote peer
identity, reconnect/resync and cross-machine replication remain unimplemented or unverified. Do not expose this as a
working online session toggle.
