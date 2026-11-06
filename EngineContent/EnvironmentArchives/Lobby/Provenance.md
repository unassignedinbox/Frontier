# Lobby archive provenance

The Lobby FSP1 topology and material and vehicle/effect uploads were captured
from `Experimental/VehicleHmiPrototype` at commit `4590e435a14c141375c333e792bb5bdd35a0a893` using the upstream
ReSTIR parity capture hook (`Projects/Everest/Tools/CaptureReference.mjs`,
from native renderer commit `118dc3e40ff7ccc6a1ceb42d5b4eca7601e437c5`). The
null backend is used only to serialize scene construction and buffer uploads;
it does not render these images. To reproduce, from the repository root:

```sh
ROOT="$PWD"
cd Experimental/VehicleHmiPrototype/Tools
CAPTURE_OUT=/tmp/lobby-reference HOOKS="$ROOT/Projects/Everest/Tools/CaptureReference.mjs" \
CAPTURE_SETTINGS='{"SmokeEnabled":false,"Sparks":false,"TyreMarks":false,"FlareEnabled":false,"CloudsEnabled":false}' \
node HeadlessHarness.mjs .. 2 '?capture=1&lobby=1&quality=low'
cd "$ROOT"
python3 Projects/Everest/Tools/ExportLobby.py /tmp/lobby-reference /tmp/lobby-export EngineContent/EnvironmentArchives/ProvingGround
```

The current JS material layout is 64 bytes while this older native renderer
uses 48 bytes. The export retains the shared first 48 bytes of every record;
the newer refraction index is not supported. The 208-byte reference vehicle
record is zero-padded to the native renderer's 10,448-byte profile. Disabled
cloud profiles come from the existing native Proving Ground archive. The five
studio lights are ported to the native light register in `RestirPresentation`.
This is not a claim of complete rendering or UI parity with the current JS
Lobby: its camera animation, HMI, and newer shader features are not ported.
