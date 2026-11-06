# Smoke/fire port — work in progress

This starts from commit `8540cf3`; the earlier CPU/ImGui approximation was
reverted. This folder does **not** yet implement the complete editor or renderer.

Implemented so far:

- `GasEditorModel` ports `ConstructGasParameters`, `GasPresets`, and
  `MapPlumeSettings` from `Experimental/VehicleHmiPrototype/Source/SmokeSequence.js`
  and `InspectorPages.js`. `GasEditorData.inl` is generated directly from those
  modules and `PlumeDefaults` with `node Validation/Tools/GenerateGasEditorData.mjs`.
- `PlumeSchedule` ports the host-side residency, relocation, timing, and 256-byte
  uniform encoding in `PlumeSpace.js`. It does **not** submit GPU commands.
- `GasEditorParity` compares the catalogue, presets and mappings with the actual
  JavaScript modules. `PlumeScheduleParity` compares four grades over 44 frames,
  including every uniform word, against the JavaScript scheduler.

Build and run the checks from the repository root:

```sh
cmake -S Frontier -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Node.js must be available when configuring CMake to enable the differential tests.

Still required: native outliner selection and exact editor UI, a native WebGPU
backend and the original shader dependency graph/passes, scene SDF and moving
occupant data, source projection and tyre/fire telemetry, scene composition, and
interactive and visual parity testing. The original WGSL files have not been
translated or integrated into a native renderer. No screenshot here is evidence
of the requested UI or rendered smoke/fire.
