# SolidArcRemastered — BooleanValidation

| | Scene | Checks | Failures | B2 max rel. ΔV | B3 identity residual | B4 oracle wrong / points | General Fuse | ms |
|---|---|---|---|---|---|---|---|---|
| 🟢 | BlockCorner | 25 | 0 | 1.5e-15 | 1.2e-14 | 0 / 480 | 6/36 pairs · 6 sect · 6 paves · 0 common · 9+9 frag (I6 O12 S0 X0) | 3860 |
| 🟢 | BlockStacked | 21 | 0 | 7.8e-16 | 2.7e-15 | 0 / 480 | 7/36 pairs · 0 sect · 4 paves · 0 common · 7+7 frag (I0 O12 S0 X2) | 3671 |
| 🟢 | BlockRotated | 25 | 0 | 1.5e-15 | 3.6e-15 | 0 / 480 | 16/36 pairs · 16 sect · 16 paves · 0 common · 14+14 frag (I10 O18 S0 X0) | 7636 |
| 🟢 | DrilledBlock | 25 | 0 | 1.2e-15 | 3.2e-14 | 0 / 480 | 2/18 pairs · 2 sect · 2 paves · 0 common · 8+5 frag (I3 O10 S0 X0) | 6007 |
| 🟢 | SphereCrater | 25 | 0 | 8.9e-16 | 8.9e-16 | 0 / 480 | 1/6 pairs · 1 sect · 1 paves · 0 common · 7+2 frag (I2 O7 S0 X0) | 4333 |
| 🟢 | SphereCraterTilted | 25 | 0 | 3.1e-15 | 1.8e-15 | 0 / 480 | 1/6 pairs · 1 sect · 1 paves · 0 common · 7+2 frag (I2 O7 S0 X0) | 6453 |
| 🟢 | SphereLens | 25 | 0 | 1.4e-15 | 1.8e-15 | 0 / 480 | 1/1 pairs · 2 sect · 2 paves · 0 common · 3+2 frag (I3 O2 S0 X0) | 9835 |
| 🟢 | SteppedShaft | 25 | 0 | 3.3e-16 | 1.8e-15 | 0 / 480 | 3/9 pairs · 1 sect · 1 paves · 0 common · 4+4 frag (I3 O5 S0 X0) | 4806 |
| 🟢 | ConeInCylinder | 25 | 0 | 5.1e-16 | 6.7e-16 | 0 / 480 | 2/6 pairs · 2 sect · 3 paves · 0 common · 4+5 frag (I3 O6 S0 X0) | 5084 |
| 🟢 | TorusSlab | 25 | 0 | 1.9e-15 | 5.3e-14 | 0 / 480 | 1/6 pairs · 2 sect · 2 paves · 0 common · 3+8 frag (I2 O9 S0 X0) | 9879 |
| 🟢 | TorusEquator | 25 | 0 | 1.8e-15 | 5.7e-14 | 0 / 480 | 1/6 pairs · 1 sect · 1 paves · 0 common · 2+8 frag (I2 O8 S0 X0) | 9130 |
| 🟢 | DShaft | 25 | 0 | 3.5e-15 | 2.1e-14 | 0 / 480 | 3/18 pairs · 4 sect · 4 paves · 0 common · 7+7 frag (I5 O9 S0 X0) | 3946 |
| 🟢 | BlockCavity | 21 | 0 | 1.1e-15 | 5.0e-14 | 0 / 480 | 0/36 pairs · 0 sect · 0 paves · 0 common · 6+6 frag (I6 O6 S0 X0) | 2869 |
| 🟢 | BlockApart | 21 | 0 | 4.4e-16 | 8.9e-16 | 0 / 480 | 0/36 pairs · 0 sect · 0 paves · 0 common · 6+6 frag (I0 O12 S0 X0) | 2073 |
| 🟢 | NapkinRing | 25 | 0 | 1.8e-15 | 5.3e-15 | 0 / 480 | 1/3 pairs · 2 sect · 4 paves · 0 common · 3+5 frag (I3 O5 S0 X0) | 4050 |
| 🟢 | RandomBlocks | 680 | 0 | 5.6e-16 | 5.6e-16 | 0 / 3840 | 40 random pairs | 29895 |
| 🟢 | RandomTurnedPrisms | 508 | 0 | 1.2e-15 | 3.3e-15 | 0 / 2880 | 30 random pairs | 26749 |
| 🟢 | RandomSpheres | 510 | 0 | 3.6e-15 | 4.4e-15 | 0 / 2880 | 30 random pairs | 33922 |
| 🟢 | RandomBlockSphere | 390 | 0 | — (no closed form) | 2.2e-15 | 0 / 2880 | 30 random pairs | 25604 |
| 🟢 | RandomDrills | 389 | 0 | — (no closed form) | 2.2e-15 | 0 / 2880 | 30 random pairs | 17066 |
| 🟢 | OpenArgument (negative control) | 1 | 0 | — (no closed form) | 0.0e+00 | 0 / 0 | refused: GeneralFuse: fragment winding 0.075447 is not integral (argument not closed?) | 12 |

**PASS** — 0 failing scene(s); allowances: audit sound, |ΔV|/max(1, V) ≤ 1e-9, identities ≤ 1e-9·max(1, V_A + V_B), 0 oracle disagreements

## Results per operation

| Scene | Operation | Outcome | Faces | Shells | Solids | Volume | Closed form | ms |
|---|---|---|---|---|---|---|---|---|
| BlockCorner | Union | 🟢 sound | 12 | 1 | 1 | 15.000000000000 | 15.000000000000 | 34.8 |
| BlockCorner | Intersection | 🟢 sound | 6 | 1 | 1 | 1.000000000000 | 1.000000000000 | 14.9 |
| BlockCorner | Difference | 🟢 sound | 9 | 1 | 1 | 7.000000000000 | 7.000000000000 | 34.7 |
| BlockCorner | ReversedDifference | 🟢 sound | 9 | 1 | 1 | 7.000000000000 | 7.000000000000 | 43.0 |
| BlockStacked | Union | 🟢 sound | 12 | 1 | 1 | 6.000000000000 | 6.000000000000 | 33.1 |
| BlockStacked | Intersection | 🟢 empty (refused) | 0 | 0 | 0 | 0.000000000000 | 0.000000000000 | 0.0 |
| BlockStacked | Difference | 🟢 sound | 7 | 1 | 1 | 4.000000000000 | 4.000000000000 | 27.5 |
| BlockStacked | ReversedDifference | 🟢 sound | 7 | 1 | 1 | 2.000000000000 | 2.000000000000 | 15.2 |
| BlockRotated | Union | 🟢 sound | 18 | 1 | 1 | 10.928203230276 | 10.928203230276 | 92.9 |
| BlockRotated | Intersection | 🟢 sound | 10 | 1 | 1 | 5.071796769724 | 5.071796769724 | 35.9 |
| BlockRotated | Difference | 🟢 sound | 14 | 1 | 1 | 2.928203230276 | 2.928203230276 | 64.4 |
| BlockRotated | ReversedDifference | 🟢 sound | 14 | 1 | 1 | 2.928203230276 | 2.928203230276 | 64.4 |
| DrilledBlock | Union | 🟢 sound | 10 | 1 | 1 | 38.283185307180 | 38.283185307180 | 127.4 |
| DrilledBlock | Intersection | 🟢 sound | 3 | 1 | 1 | 6.283185307180 | 6.283185307180 | 47.3 |
| DrilledBlock | Difference | 🟢 sound | 7 | 1 | 1 | 25.716814692820 | 25.716814692820 | 113.8 |
| DrilledBlock | ReversedDifference | 🟢 sound | 6 | 2 | 2 | 6.283185307180 | 6.283185307180 | 60.9 |
| SphereCrater | Union | 🟢 sound | 7 | 1 | 1 | 8.113097335529 | 8.113097335529 | 49.3 |
| SphereCrater | Intersection | 🟢 sound | 2 | 1 | 1 | 0.410501440069 | 0.410501440069 | 32.7 |
| SphereCrater | Difference | 🟢 sound | 7 | 1 | 1 | 7.589498559931 | 7.589498559931 | 63.1 |
| SphereCrater | ReversedDifference | 🟢 sound | 2 | 1 | 1 | 0.113097335529 | 0.113097335529 | 18.8 |
| SphereCraterTilted | Union | 🟢 sound | 7 | 1 | 1 | 8.113097335529 | 8.113097335529 | 84.3 |
| SphereCraterTilted | Intersection | 🟢 sound | 2 | 1 | 1 | 0.410501440069 | 0.410501440069 | 84.1 |
| SphereCraterTilted | Difference | 🟢 sound | 7 | 1 | 1 | 7.589498559931 | 7.589498559931 | 110.5 |
| SphereCraterTilted | ReversedDifference | 🟢 sound | 2 | 1 | 1 | 0.113097335529 | 0.113097335529 | 61.5 |
| SphereLens | Union | 🟢 sound | 2 | 1 | 1 | 7.068583470577 | 7.068583470577 | 134.8 |
| SphereLens | Intersection | 🟢 sound | 3 | 1 | 1 | 1.308996938996 | 1.308996938996 | 109.7 |
| SphereLens | Difference | 🟢 sound | 2 | 1 | 1 | 2.879793265791 | 2.879793265791 | 95.0 |
| SphereLens | ReversedDifference | 🟢 sound | 3 | 1 | 1 | 2.879793265791 | 2.879793265791 | 144.8 |
| SteppedShaft | Union | 🟢 sound | 5 | 1 | 1 | 7.068583470577 | 7.068583470577 | 51.9 |
| SteppedShaft | Intersection | 🟢 sound | 3 | 1 | 1 | 0.785398163397 | 0.785398163397 | 16.6 |
| SteppedShaft | Difference | 🟢 sound | 5 | 1 | 1 | 5.497787143782 | 5.497787143782 | 48.0 |
| SteppedShaft | ReversedDifference | 🟢 sound | 3 | 1 | 1 | 0.785398163397 | 0.785398163397 | 16.6 |
| ConeInCylinder | Union | 🟢 sound | 6 | 1 | 1 | 3.403392041389 | 3.403392041389 | 47.2 |
| ConeInCylinder | Intersection | 🟢 sound | 3 | 1 | 1 | 1.047197551197 | 1.047197551197 | 21.3 |
| ConeInCylinder | Difference | 🟢 sound | 3 | 1 | 1 | 1.047197551197 | 1.047197551197 | 35.6 |
| ConeInCylinder | ReversedDifference | 🟢 sound | 6 | 2 | 2 | 1.308996938996 | 1.308996938996 | 44.9 |
| TorusSlab | Union | 🟢 sound | 9 | 1 | 1 | 38.865223667945 | 38.865223667945 | 191.8 |
| TorusSlab | Intersection | 🟢 sound | 2 | 1 | 1 | 4.147006669624 | 4.147006669624 | 66.3 |
| TorusSlab | Difference | 🟢 sound | 3 | 1 | 1 | 10.065223667945 | 10.065223667945 | 96.3 |
| TorusSlab | ReversedDifference | 🟢 sound | 8 | 1 | 1 | 24.652993330376 | 24.652993330376 | 173.2 |
| TorusEquator | Union | 🟢 sound | 8 | 1 | 1 | 43.106115168784 | 43.106115168784 | 132.1 |
| TorusEquator | Intersection | 🟢 sound | 2 | 1 | 1 | 7.106115168784 | 7.106115168784 | 80.9 |
| TorusEquator | Difference | 🟢 sound | 2 | 1 | 1 | 7.106115168784 | 7.106115168784 | 76.5 |
| TorusEquator | ReversedDifference | 🟢 sound | 8 | 1 | 1 | 28.893884831216 | 28.893884831216 | 179.2 |
| DShaft | Union | 🟢 sound | 9 | 1 | 1 | 36.082892306765 | 36.082892306765 | 111.0 |
| DShaft | Intersection | 🟢 sound | 5 | 1 | 1 | 1.341885654005 | 1.341885654005 | 62.0 |
| DShaft | Difference | 🟢 sound | 4 | 1 | 1 | 8.082892306765 | 8.082892306765 | 28.8 |
| DShaft | ReversedDifference | 🟢 sound | 10 | 1 | 1 | 26.658114345995 | 26.658114345995 | 77.3 |
| BlockCavity | Union | 🟢 sound | 6 | 1 | 1 | 64.000000000000 | 64.000000000000 | 50.5 |
| BlockCavity | Intersection | 🟢 sound | 6 | 1 | 1 | 8.000000000000 | 8.000000000000 | 14.6 |
| BlockCavity | Difference | 🟢 sound | 12 | 2 | 1 | 56.000000000000 | 56.000000000000 | 99.4 |
| BlockCavity | ReversedDifference | 🟢 empty (refused) | 0 | 0 | 0 | 0.000000000000 | 0.000000000000 | 0.0 |
| BlockApart | Union | 🟢 sound | 12 | 2 | 2 | 3.000000000000 | 3.000000000000 | 11.6 |
| BlockApart | Intersection | 🟢 empty (refused) | 0 | 0 | 0 | 0.000000000000 | 0.000000000000 | 0.0 |
| BlockApart | Difference | 🟢 sound | 6 | 1 | 1 | 1.000000000000 | 1.000000000000 | 4.7 |
| BlockApart | ReversedDifference | 🟢 sound | 6 | 1 | 1 | 2.000000000000 | 2.000000000000 | 15.0 |
| NapkinRing | Union | 🟢 sound | 5 | 1 | 1 | 5.235454593163 | 5.235454593163 | 61.4 |
| NapkinRing | Intersection | 🟢 sound | 3 | 1 | 1 | 0.963954909921 | 0.963954909921 | 34.1 |
| NapkinRing | Difference | 🟢 sound | 2 | 1 | 1 | 3.224835294866 | 3.224835294865 | 49.3 |
| NapkinRing | ReversedDifference | 🟢 sound | 6 | 2 | 2 | 1.046664388377 | 1.046664388377 | 45.6 |
