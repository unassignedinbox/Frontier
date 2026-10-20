# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 39 frames · 4.64917 s · slowest frame 826.961 ms
- Script: Experimental/DraftingWorkspace/Scripts/RecoveryWalkthrough.txt
- Expectations: 12 / 12 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 6 | `welcome == 1` | 1 | 🟢 |
| 7 | `steps == 0` | 0 | 🟢 |
| 8 | `orthographic == 1` | 1 | 🟢 |
| 9 | `grid == 0` | 0 | 🟢 |
| 15 | `welcome == 0` | 0 | 🟢 |
| 16 | `steps == 12` | 12 | 🟢 |
| 17 | `refused == 0` | 0 | 🟢 |
| 18 | `genus == 2` | 2 | 🟢 |
| 19 | `sound == 1` | 1 | 🟢 |
| 30 | `document-path == _ClaudeScratch/DraftingWorkspace/Bracket.drafting.toml` | _ClaudeScratch/DraftingWorkspace/Bracket.drafting.toml | 🟢 |
| 31 | `steps == 12` | 12 | 🟢 |
| 32 | `status contains Opened` | Opened _ClaudeScratch/DraftingWorkspace/Bracket.drafting.toml · 12 steps | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 16WelcomeBack | 4 | `c28e33d7e10188e4` |
| 17Recovered | 13 | `95e421359acadbbb` |
| 18Opened | 39 | `42dbc96abb9b6e32` |

## Journal

- Opened _ClaudeScratch/DraftingWorkspace/Recovery.drafting.toml · 12 steps
- Opened _ClaudeScratch/DraftingWorkspace/Bracket.drafting.toml · 12 steps
