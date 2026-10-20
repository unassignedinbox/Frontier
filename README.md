# Frontier

- **`Experimental/VehicleHmiPrototype/`**: the combined WebGPU prototype: a software BVH path tracer with ReSTIR GI, Lumen-style screen traces, two clustered-light systems
  (real lights with IES photometry, and emissive surfaces) and XPBD tyres. Start with its [README](Experimental/VehicleHmiPrototype/README.md); the plan, the provenance of every
  ported part, the test and benchmark results and the known issues are in [COMBINED_PLAN.md](Experimental/VehicleHmiPrototype/COMBINED_PLAN.md).
- **`.github/workflows/`**: CI for it (unit tests, the null-backend validation of every URL mode, executed GPU tests on SwiftShader) and a toggle-benchmark matrix. GitHub runners have
  no GPU; see the comments in the workflows for what the numbers do and do not mean.
