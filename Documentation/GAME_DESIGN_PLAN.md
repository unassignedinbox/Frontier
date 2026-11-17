# Frontier — Teleport Gauntlet Racing Game Design Plan

> **Core Identity:** No police. No tracks. No circuits. No laps. Players are teleported straight from one hostile, high-chaos physics gauntlet into the next — keeping their momentum, damage, flames, and latched creatures straight through every portal.
>
> **Design Rule:** Every level must have **violence, panic, scale, and something actively trying to grab, crush, burn, eat, or trap your vehicle every few seconds** while you stay on the gas.

---

## 1. Core Level Roster (User's Original 6)

1. **Conveyor Belt (`foundry` — Implemented)**
   - Massive multi-lane industrial conveyor gauntlet with counter-moving belts, turbo strips, moving steel cargo, stamping presses, side-pushers, rotary sweepers, and giant articulated electromagnet robots pulling both cargo and your car.
2. **Dead Forest**
   - Twisted dead woodland where massive gnarled roots violently erupt straight out of the earth in front of your bumper, whipping across lanes and blocking escape routes.
3. **Overgrown Lab**
   - Ruined botanical/industrial facility overrun by colossal carnivorous plants actively lunging out to snap up, trap, and *eat* vehicles whole.
4. **Demolition Run (`demolish` — Implemented)**
   - Procedurally generated & procedurally fractured urban demolition neighborhood (randomized every entry): players choose crevices, blast straight through drive-through garages, glass storefronts, interior partitions, and brick walls while giant wrecking-ball cranes and C4 demolition blasts bring buildings down around them.
5. **D-Day (`dday` — Implemented)**
   - Omaha Beach gauntlet: players race to breach The Wall while diving aircraft drop bombs, shore turrets track and shell the vehicle, and tanks/trenches provide temporary cover.
6. **Nuclear Wasteland City + Alien Plants**
   - Irradiated ruined metropolis overrun by hostile alien flora, toxic spores, acid rain squalls, and parasitic creatures latching onto vehicles and bleeding fuel dry.

---

## 2. Approved High-Chaos Teleport Levels

1. **The Capsizing Mega-Ship (Sliding Container Avalanche)**
   - Teleport onto the massive open deck of a container ship actively rolling sideways into the ocean (`10° → 30°` live deck tilt).
   - 40-foot shipping containers snap their chains and slide across the deck like steel avalanches. Weave between sliding 20-ton metal walls, blast through open container tunnels and closing bulkhead doors, and launch off the rising bow ramp into the exit portal before the ship capsizes.
2. **The Derailing Freight Yard (Live 80-Car Train Wreck)**
   - Race across a multi-track rail yard while full-speed runaway freight trains actively derail straight across your path.
   - Boxcars jackknife sideways into steel walls, fuel tankers chain-detonate into rolling fireballs, and tilted flatcars become sudden jump ramps — smash through wooden boxcar walls or vault burning locomotives.
3. **The Live Scrapyard Shredder (Claw Cranes & Crusher Trench)**
   - Dumped inside an automated car-breaking yard that classifies your car as scrap.
   - Giant hydraulic claw arms lunge down to grab your roof and drop you into a spinning metal shredder trench; overhead crushers slam across narrow lanes; smash through stacked junker cars and duck under corrugated steel sheds where the claws cannot reach.
4. **The Failing Hydro-Dam Spillway (Wall of Water Chasing Your Bumper)**
   - A colossal concrete dam ruptures behind you at spawn. A 20-meter wall of churning floodwater, snapped steel turbines, and tumbling concrete surges down the valley floor faster than a hesitant driver.
   - Zero slowing down — clip a pillar and the wave catches your rear bumper and tumbles the car end-over-end. Smash through turbine control booths and jump snapped bridge spans staying ahead of the surge.
5. **The Hollowed-Out Insect Highway (Predatory Swarm Biome)**
   - Broken subterranean highway infested with giant burrowing insects.
   - Drill-worms erupt vertically through asphalt like living mines, launching slabs of road skyward; packs of fuel-mosquitoes dive-bomb your hood; amber-spitters fire hardening glue across exits forcing you to smash through brittle honeycomb side tunnels.
6. **The Rogue Weapons Proving Range**
   - Automated military testing canyon locked onto your vehicle as a live target dummy: rail-mounted sled guns race alongside leading your speed, pneumatic steel flipper-plates snap out of the floor to launch cars sky-high, and parachuting concrete tank-traps rain down from cargo drops shifting safe lanes every second.
7. **The Logging Mountain Slide (Downhill Chaos)**
   - Steep downhill mud-and-timber run while a logging mountain collapses around you: 10-ton stripped tree trunks bounce end-over-end down the slope right over your hood, runaway logging trailers jackknife through wooden cabins, and players smash straight through plank sawmills to escape the main log chute.

---

## 3. New Level Additions (City, Giant Shifting Concrete Maze & Acid Rain)

### A. High-Chaos City Level — **"Sinkhole Downtown (The Tilting Skyline)"**
*A city level built around panic, collapsing infrastructure, and vertical route choices — zero police, zero circuits.*
- **What's actively trying to kill you:** The entire downtown grid is collapsing into a chain of **live subterranean sinkholes and ruptured utility mains**.
  - Entire asphalt intersection slabs **crack and drop 15 meters downward** seconds after your tyres cross them (forcing constant commitment).
  - **High-pressure water & steam mains** blow manhole covers sky-high and erupt into towering geysers that can flip a car onto two wheels.
  - **Skyscraper glass curtain-walls & billboards** shear off leaning towers above, shattering across avenues while **automated rooftop HVAC units and elevators** crash into streets.
  - **Natural Multi-Level Route Choices:** Drop into a flooded subway concourse and smash through ticket turnstiles, blast up a multi-story parking garage spiral and jump through the 3rd-floor parapet onto an elevated monorail deck, or cut straight through ground-floor glass bank lobbies to bypass a freshly opened sinkhole crater.

### B. Giant Concrete Maze — **"The Piston Labyrinth"**
*Colossal, oppressive brutalist concrete corridors (`18–25 m` tall monolith walls) that physically shift, crush, and reconfigure around you — procedurally generated every run.*
- **What's actively trying to kill you:**
  - **Sliding Hydraulic Megawalls:** Enormous 20-meter-tall concrete monoliths grind sideways on recessed steel floor rails with deafening warning horns, **sealing corridors shut** or **trying to pancake slow cars** between closing faces.
  - **Moving Floor Slats & Drop Pits:** Dead-end traps suddenly retract their floor slabs unless you flick a 180° handbrake spin or punch through a thin cracked escape panel.
  - **Breakable Cheat Partitions:** Thin weathered concrete/masonry maintenance doors hidden inside corners let observant drivers **smash shortcuts straight across maze rings** instead of taking the long outer corridor.
  - **Overhead Stalker Bridges & Sweeper Blades:** Elevated steel catwalks carry roving floodlights and swinging concrete pendulum counterweights through long straightaways, so even straight corridors force aggressive weaving.
  - **Vertical Sightlines & Exit Beacon:** Narrow sky shafts let beams of sun/acid rain stab down into the canyon, and a tall pulsing green/amber extraction beacon peeks over the shifting walls so players navigate by instinct and sight rather than staring at a minimap.

### C. Global / Biome Hazard — **Acid Rain**
- **Direct Vehicle Physics Effect:**
  - Coat the road in corrosive slick runoff that **drops tyre friction coefficient (`μ`) dramatically** — longer braking distances, wild cornering drift, and easy wheelspin on throttle.
  - Slowly **corrodes exposed tyre rubber** (accelerating tread wear & puncture risk) and mists/etches the windshield in Cockpit/Glass HUD view while you stay under open sky.
- **Tactical Risk-Reward & Shelter Play:**
  - Ducking under **overpasses, drive-through garages, concrete maze arches, subway tunnels, or crane sheds** temporarily shields your car from acid exposure and restores dry patches of traction.
  - **Brilliant Parasite Synergy:** Acid rain **burns and dissolves latched ticks, mosquitoes, vines, and spore-crusts right off your hull**! Covered in heavy weight-ticks inside a tunnel? Deliberately swerve out into the open acid squall for 3 seconds to melt them off, then dive back under cover before your tyres shred.

---

## 4. Parasites, Creatures & Viruses (Violent, Physical, Skill-Removable)

> **Rule:** Never make a creature just a passive "slower/health bar" debuff. Every creature physically mutates how the car drives AND has a visceral, skill-based driving maneuver to rip, burn, or weaponize it.

### Core Parasites & Counterplay
1. **Fuel-Sucking Mosquitoes (Weaponizable):**
   - Dive-bomb and clamp onto hood/fuel lines, visibly swelling bright orange as they siphon fuel.
   - **Removal / Payoff:** Violent drift-whip (`Space + steer`) flings them off; **slamming a fuel-bloated mosquito into a wall, root barrier, or giant plant detonates it on impact**, blowing open blocked routes or staggering a vehicle-eating plant.
2. **Corner-Latching Ballast Ticks (Asymmetric Weight):**
   - Clamp onto a *specific* corner (front-left fender, rear-right quarter), visibly compressing that suspension strut and pulling steering/braking to that side.
   - **Removal:** Deliberately **grind that exact flank against a concrete wall, house corner, or steel rail at high speed** to shear them off in a shower of sparks — or dive through fire / acid rain to roast/melt them off.
3. **Unhinged Viruses (Chaos Modifiers, Not Just "Slow"):**
   - **Redline Virus:** Jams throttle at 100% + boosts engine power (`+30%`) while cutting brakes for 5 seconds — pure white-knuckle steering survival.
   - **Heavy Armor-Tick Swarm:** Kills agility and adds massive weight, *but* turns the car into a 3-ton battering ram that smashes straight through thick brick/concrete walls that would normally stop you.
   - **Misfire / Backfire Purge:** Burn a small shot of fuel on demand to fire a **rear exhaust flame blast** — roasts chasing mosquitoes or grabbing roots on your rear bumper and gives an emergency shove forward.

### Additional Original Creature Mechanics
4. **Harpoon-Tongue Anchor Grubs (Bumper Snatchers — Overgrown Lab / Dead Forest / Wasteland):**
   - Burrowed ambushers beside alleys and rubble piles shoot an elastic tendril that hooks onto your **rear bumper or axle**, visibly stretching taut and trying to reel you backward into a pit or carnivorous plant maw.
   - **Counterplay:** Floor the throttle + swing sideways to snap the tendon against a sharp corner/pillar (releasing a huge elastic slingshot boost forward!), or fire your **Rear Backfire Purge** / **Sawblade Bumper** to sever it.
5. **Wheel-Well Leech Slugs (Single-Wheel Lock / Asymmetric Drag):**
   - Slither out of wet rubble/sewers and wrap directly around **one wheel hub/brake rotor**, seizing or dragging that single tyre so the car violently pulls into a spin.
   - **Counterplay:** Tap throttle + brake together (burnout/rotor heat) to glow the brake disc red-hot and cook the slug straight out of the wheel arch, or smash through a hard kerb/rubble mound to knock it loose.
6. **Chitin Ram-Beetles (Living Battering Rams — Insect Highway / Maze / Wasteland):**
   - Car-sized armored ambush beetles that charge sideways out of side crevices to T-bone you off course or shove you into closing maze walls.
   - **Counterplay:** Bait them into charging straight into a breakable brick wall, concrete partition, or rival carnivorous plant — they smash open shortcuts *for* you.
7. **Siphon-Barnacle Spores (Windshield & Radiator Chokers):**
   - Explosive spore pods splatter thick translucent goo across your hood, radiator grille (rising engine temp), and windshield.
   - **Counterplay:** Smash straight through hanging barriers, wooden fences, glass storefronts, or drive through acid rain / water geysers to wipe your windshield & radiator clean.
8. **Aggressive Fuel Recovery (Always Forward, Never Stalled):**
   - Recover fuel at 140 km/h by smashing through red fuel drums, ruptured tanker trailers, glowing alien sap pods, or **crushing bloated fuel-ticks under your tyres** to splash fuel directly back into the tank.

---

## 5. Tool-Based Pickups (Physical Vehicle Survival Tools — NOT Mario-Kart Weapons)

*No homing rockets or generic "destroy other cars" powerups. Pickups are **tactile, mechanical survival tools** bolted onto your chassis with limited charges or short bursts that physically interact with hazards, weather, walls, and creatures:*

1. **Pneumatic Roof/Hull Shock-Purger ("Pulse Jack")**
   - Fires a single violent pneumatic chassis jolt + electro-arc across your fenders: **instantly blasts every latched tick, mosquito, vine, and anchor tendril off your car** AND hops the chassis 0.6 m vertically (enough to clear an erupting root, low rubble lip, or closing floor crack).
2. **Front Hydraulic Ram-Wedge ("Breach Plow")**
   - Temporarily bolts a heavy angled steel demolition plow onto your front bumper (3 impacts or 12 seconds): lets you **cleave straight through reinforced concrete walls, stacked junker cars, thick root walls, and giant plant jaws** with almost zero speed loss.
3. **Studded Spike-Deploy Tyres ("Grip Claws")**
   - Extends retractable tungsten carbide studs out of all 4 tyres for 10 seconds: **completely neutralizes Acid Rain slickness, oil, mud, and tilted ship decks**, letting you carve impossible corners and climb counter-moving conveyor belts at full grip.
4. **Siphon-Harvester Scoop ("Fuel Skimmer")**
   - Magnetized intake scoop under the nose for 12 seconds: automatically vacuums fuel vapor from nearby drums, burning wreckage, and crushed fuel-mosquitoes/ticks without needing a direct dead-center hit, rapidly refilling your tank on the move.
5. **Thermal Roof Shield / Ablative Canopy**
   - Drops armored louvered shutters over the roof and windshield for 10 seconds: **blocks Acid Rain corrosion, falling masonry/glass shards from skyscrapers, aerial shrapnel, and overhead scrapyard crane claws** trying to grab your roof.
6. **Emergency Rear Anchor-Thruster ("Extraction Kick")**
   - Single-use solid-fuel booster cartridge strapped to the rear frame: fires a violent 1.8-second straight-line thrust + scorching rear flame cone — specifically designed to **rip your car free from a giant plant's jaws, outrun a closing concrete maze wall, clear a collapsing sinkhole gap, or roast everything latched to your tail**.
7. **Magnetic Tow-Winch Hook (Environmental Grapple)**
   - Automatically snaps a steel cable onto overhead girders, crane hooks, or maze corner posts on hard turns — letting you **whip-pivot a 90° concrete maze corner at 120 km/h** without scrubbing speed or pull yourself up a tilting ship deck.

---

## 6. Teleport Portal Meta-Mechanic: **Momentum & State Carry-Over**

Because there are no laps or reset grids, **every portal preserves your exact physical state**:
- Enter a portal at 160 km/h mid-air → burst out of the exit portal in the next biome still flying at 160 km/h.
- Carry latched parasites, fire, acid slickness, and mounted survival tools right across worlds — turning cross-biome interactions into clutch survival tricks (e.g., dragging Dead Forest weight-ticks through a portal straight into a Demolish Run C4 fireball or Wasteland Acid Rain squall to wipe your chassis clean in one second).
