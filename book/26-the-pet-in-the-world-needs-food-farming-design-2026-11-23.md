# 26. The pet in the world: needs, food, farming, cooking (DESIGN, 2026-11-23)

**DESIGN. Nothing here is built except what section 1 says is BUILT.** Labels: **BUILT** (ran), **READ** (read the file today), **OBS** (observed today), **DESIGN**, **OPEN** (needs the owner), **NOT FOUND** (searched the house, nothing there).

Owner, 2026-11-23: *"the pet is on a 2d/3d world. they can walk around, use inventory (food); you can give them food as a created concept entity; it can also create entities, it can plant, water, harvest etc. later it can learn to cook ingredients. That would help hunger because we pre-program concepts and meanings and weights and events."* And: *"digital pet mechanics early on like hunger etc (they can be turned off by the user) or used to try to teach the pet to eat and do other things; on the web page ASAP because I promised that to users; we want to acclimate them to using hai with systems they are familiar with; present this in the video with the website address."*

## 1. What exists (checked 2026-11-23)
- **BUILT, hai:** the teach loop (unknown word, ask, meaning stored, trust moves with grades), `ops/hai_world.c` (agents on pages, walls, collisions, teleport; case `hai_world`, 43 checks).
- **READ + OBS, house `survivor_op`** (`@.apps/rpg-edit-hq/ops/survivor_op.c`, 129 lines, data driven): `tick` (needs rise by `needs.pdl` `RATE|need|points|per_minutes`; I ran 120 minutes and hunger went 0 to 20), `eat` / `drink` (burns one item through `item_ledger_op`, lowers the need named by `itemdata.pdl` `USE|item|eat|hunger|-40`; refused if none held), `craft` (recipes with tool, skill, level, minutes), `smash` (furniture into items), `wield`, `light`. It refuses a folder without a `QUARANTINE` file (exit 4, observed). Its needs store is the file `state/needs.txt`.
- **READ, house `pet-trainer`:** a map char that gives an item when you stand next to it and regrows after N seconds (`ai.pdl` `SOURCE|map char|item|amount|regrow seconds`), and a `farm` building (`buildings.pdl`). A proto-harvest only.
- **NOT FOUND:** plant, water, growth stages, harvest, cooking as a chain (searched `@.apps` data and ops, and the 16.game docs).

## 2. The loop the owner described (DESIGN)
```
hungry  -> does it hold food?        yes -> eat            (hunger down)
        -> is there food on the page? yes -> walk to it, take it, eat
        -> neither: plant a seed -> water it -> (time passes: growth events) -> harvest -> eat
        later: ingredients -> cook (a recipe) -> better food
```
Every step is a **verb** that writes a ledger row, so the user can watch the pet learn to do the loop, and a person can do each step by hand.

## 3. Concepts, meanings, weights, events (what is pre-programmed)
- **Concept entities** (data, in the same bank format hai already uses): `food`, `seed`, `water`, `soil`, `plant`, `fruit`, `ingredient`, `meal`, each with **meanings** (words that point at them: "eat", "hungry", "apple") and **weights** (how strongly a meaning ties to a concept, moved by grades, as for every other word).
- **Created entities:** a verb can mint an entity (a seed, a plant on a cell, food in the inventory). The creation is a ledger row; the entity's kind is a concept.
- **Events** (clock rows, shipped on or off by data): `hunger_tick` (needs rise), `grow_tick` (a planted cell advances a stage if watered, a dry one does not), `dry_tick` (water level falls). Stages and rates are rows, never code constants.
- **Off switch:** `needs.pdl` rates of 0 (or an `ENABLED | 0` row) = no hunger, so the pet is just a chat toy in a world. The user can turn it off, as the owner asked.

## 4. The hybrid (answer to "what is wrong with the needs op")
The op is not broken; it is gated (quarantine, owner lifts per book), tied to an item ledger, native only, and its store is a file. **Reuse its DATA FORMATS and its rules; do not fork a second format.** One small core, C and JS, reads the same `needs.pdl`, `itemdata.pdl`, `recipes.pdl` shapes, with items and needs derived from ledger rows. **`survivor_op` is the test oracle:** a case runs both on the same inputs in a quarantined scratch copy and requires identical numbers; a mutant in either turns it red. Once the owner releases a hai-pal book from quarantine, an event page can call the real op on the desktop; the port serves the web and CLI-only users. Farming (section 2) is new rows and verbs in the same core; it is not in `survivor_op`, so it has no oracle and gets its own case with mutants.

## 5. Web first (promised to users)
The web demo gets: one room with the pet drawn on a grid (the same frame idea as the desktop canvas), arrow or click to walk, an inventory strip, a hunger bar, buttons `feed`, `plant`, `water`, `harvest`, an on/off switch for needs, the teach loop for the words ("what does feed mean?"), a table view of the same data, and download / reopen of the session file. Needs advance only while the page is open (OPEN: owner picks). The video shows it and ends on the website address (`https://jbro885wgu.github.io/harnecient-hai/`).

## 6. Questions for the owner (OPEN)
1. **Time:** hunger rises only while the page is open (simple, fair, my default), or also while it is closed (needs a stored timestamp and can starve a pet overnight)?
2. **First version scope:** walk + eat + a food entity on the map + hunger only, with plant / water / harvest in the next step; or plant / water / harvest in the first version because they are the point?
3. **Can the pet die or only get sad?** My default: it never dies; hunger lowers mood and slows learning, and the user can switch it off.
4. **Who plants first:** the user commands it ("plant a seed here"), or may it try on its own when hungry and see what happens (self-directed, graded afterwards)? My default: commanded first.
5. **Is a created food a `concept` the user can name** ("make a thing called apple")? That is the teach loop plus a create verb; I assume yes.
6. **Quarantine:** will the owner release a hai-pal book from quarantine later so the real `survivor_op` can run on the desktop, or should hai-pal never depend on it?

## 7. Basic anatomy: digest, waste, breathe (owner 2026-11-23; DESIGN, built on what the house already decided)
**What the house already says (READ 2026-11-23):**
- **Owner principle 2026-10-25 (opt-in fidelity):** every system has a cheap default (a number that drifts, a need that ticks, tuning in a `.pdl`) and an optional detailed layer switched on per entity or case; behaviour lives in event pages, C ops only do what a page calls. The world rule `fidelity_max` sets the ceiling (`16.game/TERUMON-SHAPESHIFT-AND-EVOLUTION-TREES-DESIGN.md` section 5; `16.game/RPG-SPACE-COLLISION-AND-DESTRUCTION-DESIGN.md` section 4b).
- **A body plan is an anatomy part tree** (bones, muscle, lungs, brain ...); parts decide behaviour (wings enable flight) and **needs (air, food) follow the parts**. That is the `part_tree_lint` / "anatomy v0" roadmap item; the linter for any `NODE` tree, `&.widgits/_shared-lib/ops/tree_lint.c`, is BUILT (harness `tree_lint.pal`, 53 checks, 0 failing).
- **NOT FOUND / NOT BUILT:** an anatomy generator, a reaction resolver, a molecular view (`16.game/RPG-DUST-DESIGN.md` says so). The anatomy docs I found are designs, not code.
**For the pet (DESIGN):**
- **Cheap default (first version):** the needs of section 3 (hunger, plus a `waste` level that rises when it eats and falls when it "goes", and a `breath`/energy level that is always fine on land). Just numbers with rates in a `.pdl`.
- **Opt-in body layer (later):** a small part tree per pet as `NODE` rows (`mouth`, `stomach`, `gut`, `lungs`, `heart`, `brain`), checked by `tree_lint`. Eating moves a food item mouth to stomach to gut over `digest` events (minutes of game time), producing energy and then waste; `lungs` give breath; a missing or damaged part changes what the pet can do. Each stage is an event row; the part tree is data, so another game can swap in a different body.
- **Why it fits teaching:** "digest", "waste", "breathe" become concepts with meanings and weights like "eat", so the pet can be taught to use the right verb ("go" / "clean up") when its waste level is high, and a person can watch the chain in the Viz tab.
- **OPEN (owner):** (a) is waste visible in the world (an entity on the map the user can clean, which can feed a plant, closing the farm loop) or only a number? (b) does a sick or full-of-waste pet only get sad, or can it fall ill? (c) is the part-tree layer in the first web version or later? My defaults: visible waste entity that fertilises a plant; sad not ill; part tree later.
