# 🧩 SolidArcArchitecture — is each existing feature fit for a parametric SolidArcWorkspace?

**TL;DR** — 176 corpus rows through the SolidArc command stream (the same one the browser UI sends to the geometry service), 129 document body states audited, 21 defect specimens. After phase 3 fixes: 0 defective audit states, 21/21 specimens, 1 defective corpus row (legacy `fillet`, to be retired), 19 honest refusals that map to the kernel gaps in the plan.

| Artefact | What it proves | Result |
|---|---|---|
| `Results/FeatureSuitability.md` | every authoring feature: runs? solid? audit-sound? exact vs closed form? persistent naming? | 176 rows · 19 refused · 1 defective · 4 volume misses (3 probe pole artefacts, 1 approximate arc pipe) |
| `Results/BrepDefects.md` | one specimen per B-rep defect fixed (D5b, D7–D12, rim fillet) with the pre-fix value | 21 / 21 ✔️ |
| `Results/DocumentAudit.md` | Validate + GeometricAudit after every command of BevelLab, MixedCapCorner, all `public/examples`, all `Samples` | 129 states · 0 defective (18 before) |
| `Results/Regeneration.txt` | editing a sketch regenerates a plain extrusion, not a bevelled one (gap G1), and the console now says so | as stated |

## Code

| File | Role |
|---|---|
| `Code/GenerateFeatureCorpus.py` | writes `Code/FeatureCorpus.arc`; holds every closed-form volume (`#= V`) |
| `Code/FeatureCorpus.arc` | sections A primitives · B sketch→solid · C booleans · D blends · E face ops · F transforms · G naming · H regeneration |
| `Code/RegenerationProbe.cpp` | section H in isolation, with and without `beveladd` |
| `Experimental/SolidArc/Service/Verification/FeatureSuitabilityVerification.cpp` | corpus runner; `{id:}`, `{face:}`, `{rim:}` tokens resolved against the live body |
| `Experimental/SolidArc/Service/Verification/BrepDefectVerification.cpp` | defect specimens |
| `Experimental/SolidArc/Service/Verification/DocumentAuditVerification.cpp` | document replay + audit |
| `Experimental/SolidArc/Service/Verification/QuadratureVolume.h` | exact-surface divergence-theorem volume shared by all harnesses |

## Run (repository root)

```
OBJS=$(ls _ClaudeScratch/build/SolidArcNative/*.o | grep -vE "GeometryHost|GeometryExchange|/main\.o")
for T in FeatureSuitabilityVerification BrepDefectVerification DocumentAuditVerification; do
  g++ -std=c++20 -O2 -IExperimental/SolidArc/Service -o _ClaudeScratch/build/$T Experimental/SolidArc/Service/Verification/$T.cpp $OBJS; done
python3 Exhibits/SolidArcArchitecture/Code/GenerateFeatureCorpus.py
./_ClaudeScratch/build/FeatureSuitabilityVerification Exhibits/SolidArcArchitecture/Code/FeatureCorpus.arc > FeatureSuitability.md   # ~50 s
./_ClaudeScratch/build/BrepDefectVerification > BrepDefects.md                                                                         # ~10 s
./_ClaudeScratch/build/DocumentAuditVerification Exhibits/RollingBallBlend/Code/BevelLab.arc Experimental/SolidArc/Service/Host/Fixtures/MixedCapCorner.arc \
  Experimental/SolidArc/public/examples/*.arc Experimental/SolidArc/Service/Samples/*.arc > DocumentAudit.md                               # ~2 min
```

The objects come from `python3 Native/CompileSequence.py --native` (cwd `Experimental/SolidArc`).

## ⚠️ Known measurement limits

- `QuadratureVolume` integrates over the tessellation's (u,v) triangles; near a sphere pole they under-cover by ~1.7e-4 relative. Ball, DomedPedestal and ScoopedSocket "misses" are this artefact: a natural-domain Gauss rule gives the kernel sphere 2.5e-8, and D7's area specimen checks the hemisphere directly.
- `Validate().Volume` is tessellation-based and `Validate().Solid()` is topological only; neither is used as proof of exactness here.
