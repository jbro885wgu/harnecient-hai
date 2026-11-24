# hai-pal: the textbook (how to use it, how it works, where it goes next)

Written 2026-11-24. For users, for builders, and for the next agent. **Every statement is labelled:** **WORKS** (run and checked, with the evidence named), **PARTLY** (works with a stated limit), **DESIGN** (decided, not built), **NOT BUILT**. Where an example shows output, it is real output from the run named beside it.

Live pages: the pet <https://jbro885wgu.github.io/harnecient-hai/pet.html> and hai itself <https://jbro885wgu.github.io/harnecient-hai/>. Code and every test: <https://github.com/jbro885wgu/harnecient-hai> (branch `hai`).

---

## Part 1. What hai-pal is, in five minutes

**hai** is a small private learning chat that has no language model inside: it matches words you taught it, shows *why* it chose a reply, and learns from your grades. **hai-pal** puts hai in a world: small agents ("pals") that live on pages (rooms), walk, hold items, have needs, and are led by a **master**.

Three ideas carry everything:

1. **One ledger is the truth.** Everything that happens is a row in an append-only text file (`SPAWN|pip|home|5|3`, `MOVE|pip|1|0`, `TICK|pip|540`). The room you see, the hunger bar, the order of the pals are all *derived* by replaying those rows. Delete the picture and it comes back identical; edit a row by hand and the rules refuse the forged world.
2. **The order is the role.** The party is an ordered list. **Whoever is first is the master**; the rest follow. Move another pal to the front and it becomes the master.
3. **Bodies are data.** A pet is organic, robot or cyborg because of a small file listing its needs and what satisfies them, not because of code.

**What you can do today**

| You want to | Do this | Status |
|---|---|---|
| Look after a pet in your browser | open `pet.html` (Part 2) | WORKS (live, driven by real clicks in headless Chrome) |
| Talk to the master and have it act for you | the chat on the right of `pet.html`, `/do ...` | WORKS for the six `/do` verbs; replies are the starter ones |
| See the world inside the LyfeDesk editor | open the book `hai-pal` in rpg-edit (Part 4) | PARTLY (you see rooms, figures and the pals list; Play does nothing yet) |
| Run the rules from a terminal | `hai_world`, `hai_pet` (Part 3) | WORKS |
| Check C and JavaScript agree | case `hai_compat` (Part 6) | WORKS |

---

## Part 2. The web pet (`pet.html`)

Open it in any modern browser. Nothing is installed and nothing leaves your page (it fetches its own rules and data files and keeps your save in the browser's local storage).

### 2.1 The one view

Like the editor, everything is in one view:

- **Left: the pals** (the order is the role: slot 1 is the master, marked "master"), the **pet** (its body type and its need bars) and the **pantry & inventory**.
- **Centre: the world** (the room, drawn from the ledger), the arrow pad, the **time** buttons and a status line.
- **Right: chat with the master**, with your **save** (the ledger table, download, open, clear) below the world.

### 2.2 Controls

| Control | Effect | Row it writes |
|---|---|---|
| Arrow keys or the pad | the pet walks one cell | `MOVE|pip|dx|dy` (world) |
| `+10 min`, `+1 hour`, `+1 day` | time passes; needs rise | `TICK|pip|minutes` (pet) |
| "run time" | 10 game minutes each real second (off by default) | `TICK|pip|10` |
| `give 1` on an item | puts one item in the pet's inventory | `GIVE|pip|item|1` |
| `eat (n)` / `charge (n)` / `cool (n)` | uses one held item | `USE|pip|item` |
| the "on" box on a need | turns that need off (weight 0) or on (weight 10 = normal) | `WEIGHT|pip|need|tenths` |
| `body` selector + `new pet` | a new pet of that body, in a new world | `BORN`, `GIVE`, `SPAWN` |
| the up / down arrows in the party | moves a pal in the order | `REORDER|name|slot` |
| `download` / `open` | saves / reloads both ledgers as one text file | none (the file *is* the rows) |
| `clear` | starts over | none |

A request the rules refuse (walking into a wall, using an item you do not hold, a slot out of range) is **not hidden**: it appears in the save table in red with its reason, and it is stored as a `REFUSED` row.

### 2.3 The three bodies (from the real files in `docs/pet-data/`)

| Body | Needs (rate per 60 minutes) | Items it can use |
|---|---|---|
| organic | hunger +10 | `food.apple` (hunger −40), `food.bread` (−60) |
| robot | drain +8, heat +4 | `cell.small` (drain −50), `cell.full` (−100), `coolant.pack` (heat −50) |
| cyborg | hunger +6, drain +5 | `food.apple`, `food.bread`, `cell.small`, `cell.full` |

A need runs 0 to 100. Time carries remainders exactly (a rate of 10 per 60 minutes means 10 minutes adds 1 point *with* the left-over fraction kept), so ten 10-minute ticks equal one 100-minute tick. This is the house needs rule, and it was checked against the house's own needs program (case `hai_pet`).

### 2.4 Talking to the master (chat)

The chat is the same hai engine as the demo at the site root: type ordinary words and it answers with fixed starter replies and shows **why** it matched (`why: matched "hi" as greet: 1 of 1 words x trust 0.83`). Under each reply are grade buttons **0 to 4**; your grade changes that reply's weight for next time. You can teach it words (see the hai demo and book 22).

**`/do` makes the master act for you.** These six verbs are the master's "hands":

| You type | The master does | Real answer from a live run |
|---|---|---|
| `/do right` (also `left`, `up`, `down`) | walks the pet one step | `I walked pip right.` |
| `/do give` | gives the pet one of the body's first item | `I gave pip one food.apple.` |
| `/do wait 600` | lets 600 minutes pass | `600 minutes passed.` |
| `/do feed` | uses a held item that satisfies a need | `I fed pip food.apple.` |
| `/do left` into a wall | refuses and says why | `I could not walk left: a wall is in the way` |
| `/do dance` | does not know it | `I know: /do left, right, up, down, feed, give, wait N.` |

Every row the master writes is **tagged master** in your save (the table shows "master · world"), and the tag survives download and reopen. **Limit (NOT BUILT):** the master only does what `/do` says. It does not yet choose actions itself or understand "please feed the pet".

### 2.5 Saving and reopening

`download` writes `hai-pet-save.txt`: a header, the body, and one line per row, e.g. `p TICK|pip|540` and `w MOVE|pip|1|0<TAB>@master`. `open` replays the rows through the same rules. Evidence (run 2026-11-24): the world and pet state after reopen was byte-identical to before, including carried remainders and who acted. The browser also remembers your last session on reload.

### 2.6 Developer hooks

`pet.html?body=robot` starts with a robot. `?do=` applies rows at load, e.g. `?do=w:MOVE|pip|1|0;p:TICK|pip|540`. `window.hai.rows()`, `.world()`, `.pet()` expose the state to a test script.

---

## Part 3. The command-line tools (C, and the same in JavaScript)

All tools append to a ledger file and print one line. **Exit 0** ok, **3** refused (a `REFUSED` row is appended and the reason printed), **2** usage. The JavaScript ports (`docs/world.js`, `docs/pet.js`) take the same arguments through `node`.

### 3.1 `hai_world`: the room and the party

Needs `HAI_RULES=docs/world.rules.pdl` (the numbers: 16×10 cells, 32-pixel cells, 32 agents, names up to 16 characters, extra wall cells). Build: `gcc -std=gnu11 -Wall -Wextra -O2 -o hai_world ops/hai_world.c`.

```
hai_world do <ledger> SPAWN name page x y      a pal appears (the cell must be free)
hai_world do <ledger> MOVE name dx dy          one step, -1..1, not both 0
hai_world do <ledger> TELEPORT name page x y   the only way to change page
hai_world do <ledger> REORDER name slot        move to a 1-based slot; slot 1 is the master
hai_world party <ledger>                       the order, on one line
hai_world master <ledger>                      the first pal
hai_world dump <ledger>                        name page x y, one line per pal
hai_world pages <ledger>                       the pages that exist
hai_world frame <ledger> <page> <out_base> [active]   512x320 RGBA canvas frame
hai_world desk <ledger> <page> <desk_dir> <atlas.rgba> the editor's desk files (C only)
hai_world run <ledger> <script> [state_out]    apply a whole script (used by the differential test)
```

**A real run (2026-11-24):**

```
$ hai_world do w.ledger SPAWN hai home 3 3        ok SPAWN
$ hai_world do w.ledger SPAWN pip home 5 3        ok SPAWN
$ hai_world do w.ledger MOVE pip 1 0              ok MOVE
$ hai_world do w.ledger MOVE pip 1 0              ok MOVE
$ hai_world do w.ledger MOVE pip 1 1              REFUSED MOVE: a wall is in the way
$ hai_world do w.ledger TELEPORT pip garden 4 4   ok TELEPORT
$ hai_world pages w.ledger                        home garden
$ hai_world dump w.ledger                         hai home 3 3 / pip garden 4 4
```

The refusal reasons you will meet: `bad name`, `bad page name`, `that cell is a wall`, `that cell is taken`, `name already used`, `too many agents`, `no such agent`, `one step only`, `a wall is in the way`, `another agent is in the way`, `slot out of range`, `already in that slot`, `missing arguments`, `unknown verb`.

### 3.2 `hai_pet`: the needs

```
hai_pet <data_dir> <ledger> born <name> <type>       type = organic | robot | cyborg (a body file)
hai_pet <data_dir> <ledger> give <name> <item> <n>   n = 1..99
hai_pet <data_dir> <ledger> tick <name> <minutes>    1..100000
hai_pet <data_dir> <ledger> use <name> <item>        only an item the body knows and the pet holds
hai_pet <data_dir> <ledger> weight <name> <need> <tenths>   0 = off, 10 = normal, 40 = four times as fast
hai_pet <data_dir> <ledger> status <name>            NEEDS / BODY weights / INVENTORY
```

`<data_dir>` is `docs/pet-data/`. A body file is three row kinds: `BODY|type`, `RATE|need|points|per_minutes`, `USE|item|verb|need|delta` (the same shapes as the house `needs.pdl` and `itemdata.pdl`). Add a body by adding a file: no code changes.

### 3.3 The chat

`hai_chat` (C) and `docs/engine.js` (JavaScript) are the chat engine; the book `book/24` explains the pipeline (TURN, SAID, WHY, GRADE rows).

---

## Part 4. hai-pal inside the LyfeDesk editor (rpg-edit)

### 4.1 What you see

The book `hai-pal` (house folder `@.apps/hai-pal/`) opens like any other book. WORKS (photographed 2026-11-24): the **Books** dropdown lists `hai-pal`; opening it shows `book:hai-pal` and `page:home` in the header, the room drawn in 3D (a floor, a wall border, the inner wall block) with the figures standing on it; the minimap shows the room; the **pets bar** on the left lists **hai** (the first pal, the master) then **pip**.

### 4.2 How to open it

1. Click the **Books** cell (header cell 2) **twice** (the first click only focuses it), then click the row `hai-pal` **twice**.
2. For the pals list, switch the left slot from palettes to the pets bar with the bottom "pets" cell.
3. If the list still shows the old party, any action in the editor refreshes it.

(Driving the real window by relay is documented in `HQ-IQ-BOOK/16.game/OPEN-A-PROJECT-AS-A-BOOK-IN-RPG-EDIT-...-2026-11-23.md` section 10: use clicks, not typed digits.)

### 4.3 How a book gets its own party (the generic rule, house commits 8980d72c0 and 8873ffed9)

When the open book ships `@.apps/<book>/db/system.pdl` with a `BookParty` group, the pets bar reads the party from there: `Party member1` is the **first pal (the master)**, the `BookParty` members are the followers, in order. A book without that file is unchanged (proved: 17 pals for a no-db book and for the editor's own book on old and new binaries). hai-pal's file:

```
SYSTEM | name | Party       SYSTEM | member1 | hai
SYSTEM | name | BookParty   SYSTEM | member1 | pip
```

### 4.4 The files of the book

| File | What it is |
|---|---|
| `game.pdl`, `rooms.pdl`, `verbs.pdl`, `state.pdl` | the book's declaration; `state.pdl` classifies every file as LEDGER, PROJECTION, CONFIG or PREFERENCE |
| `ledger/world.ledger` | **the authority**: the world's rows |
| `hai-pal-pchq/.../maps/hai-pal/<page>/` | desk files for each page (cells, figure tokens): a **projection**, written by `hai_world desk` |
| `db/system.pdl`, `db/LAYER` | the party (to become a projection of the ledger's REORDER rows) |

### 4.5 What does not work yet (honest list)

Play does nothing (no `GAME | driver` row); the party file is hand-written, not yet derived from the ledger; the page-file check `book_check` passes 4, fails 2 (A1, A2), skips 8 (the same as every generated book); the `m` move mode is not built; the left slot opens on palettes until you choose the pets bar.

---

## Part 5. The data files (what is where)

| File | Read by | Contents |
|---|---|---|
| `docs/world.rules.pdl` | `hai_world.c`, `world.js` | `RULE|cols|16`, `rows`, `cell`, `max_agents`, `name_max`; `WALL|page|x|y` |
| `docs/pet-data/body_<type>.pdl` | `hai_pet.c`, `pet.js`, the page | `BODY`, `RATE`, `USE` rows |
| `docs/data/*.pdl` | `engine.js`, `hai_chat` | concepts, replies, states, rules, seeds, stop words, packs |
| `@.apps/hai-pal/db/system.pdl` | the editor | the party |
| any `*.ledger` | all tools | append-only rows, never edited |

**Rules for anyone editing data:** numbers live in data, not in C or JS; there is one authority per fact; a projection is never read back as input.

---

## Part 6. Testing: what is proved and how to run it

Run a case with the house harness: `harness_case_op.+x <case>.pdl <house>` from `cases/`; compare the number of `EXPECT_` rows with the `PASS` lines in `results/<case>.txt` (a missing file prints nothing, which reads as zero: check it exists).

| Case | Checks | What it proves |
|---|---|---|
| `hai_world` | 62 | the world rules and the desk projection |
| `hai_world_parity` | 213 | C and JS give the same words, exit codes, ledgers and frames |
| `hai_world_reorder` | 134 | REORDER, master, party, refusals, in C and JS |
| `hai_compat` | 205 | **differential**: 3000 hostile rows per seed (world: 5 seeds on the house rules, 3 on a smaller world; pet: 4 seeds), C and JS ledgers and end states byte-identical, frames identical, every refusal reason reached |
| `hai_pet` | 34 | the needs rules, checked against the house needs program |
| `hai_pet_parity` | 188 | C and JS agree on the pet |

Total today: **836 checks, all passing**, plus **13 deliberate one-sided breakages** (7 in the world, 6 in the pet) that each turned the differential case red. The method is written down in `HQ-IQ-BOOK/02-architecture/HAI-AND-PALS-REUSE-AND-COMPATIBILITY-GUIDE-2026-11-24.md` section 7.

**Not covered by a repeatable test:** the web page (verified by driving the real buttons in headless Chrome, but that script is not in the repo); the editor's left pals bar (verified in a scratch house and on the live window, not by a harness case); the `desk` command in JavaScript.

---

## Part 7. How hai-pal fits the house (integration map)

| House piece | How hai-pal uses it | State |
|---|---|---|
| **Ledger as authority** (`NO-GHOST-DATA...`) | every fact is a row; projections declared in `state.pdl` | WORKS; hash checkpoints (`ckpt` rows) NOT BUILT |
| **Rules as ops** (`STANDARD-RULES-AS-OPS...`) | each verb should be a pure module, an op, a registry row, a case and an event page | PARTLY: ops and cases exist; registry rows and event pages NOT BUILT |
| **Event rows** | the world should use `EVENT|round|actor|verb|target|at|result` | DESIGN (owner chose it); the actor column makes "the master did it" a row |
| **Trunk and branch** (`trunk_op`) | bodies as shared defaults, each agent's changes as a branch | DESIGN |
| **Needs** (`survivor_op`) | the pet's needs agree with it; reuse needs your quarantine release | PARTLY (oracle only) |
| **Party and bars** (`petslike_core`, `layouts.pdl`) | the pets bar reads the book's own party | WORKS (generic hook) |
| **Pet-to-pet speech** (`net_say.pdl`) | pals talking to each other | DESIGN (shipped OFF in the house) |
| **Books and pages** (`book_check`, rooms) | a page is a session, a book is the project | PARTLY (see 4.5) |
| **Presentations** | one place, standards, an mp3 beside every mp4 | WORKS (video `hai-pal-and-the-web-pet`) |

---

## Part 8. Roadmap

Each step needs a test that can fail, then the real window opened by eye (the house rule). Order matters: later steps assume earlier ones.

**Phase A: finish the party (days)**
1. **Move mode `m`** with the arrow keys (handoff: `HQ-IQ-BOOK/02-architecture/HOW-TO-BUILD-M-MOVE-MODE-AND-REORDER-HANDOFF-2026-11-24.md`, test spec T1 to T9). `^` stays the lock.
2. **The party projection:** an op that turns a reorder into a ledger row *and* rewrites `db/system.pdl`, so the order has one authority.
3. **A default bar:** a book `layouts.pdl` row so hai-pal opens on the pets bar, not palettes.
4. **A harness case for the book-party hook** (so the pals-on-left change stops depending on a manual scratch run).

**Phase B: make it playable (1 to 2 weeks)**
5. **Event rows and registry:** move the world to `EVENT` rows, add `ckpt` hash rows, give each verb a `COMMAND` registry row and an event page (visible in Blueprintz).
6. **The Play driver** (`GAME | driver`): Play runs the agents' loop, Stop pauses; pressing Play must not change your saved book.
7. **Page-file conformance** (`book_check` A1 to A10).
8. **One session, two doors:** the standalone window and the editor book open the same session and world; one case replays one ledger through both.

**Phase C: the master gets real powers (weeks)**
9. **Permissions:** hard rules and hand-set biases that can forbid any master verb (a refusal is a row); permission grows only by your rule.
10. **Interface verbs:** the master opens and closes bars, switches book and page, by calling the same ops your clicks call (every action a row with actor `master`).
11. **Plain-language `/do`:** map "please feed the pet" to verbs by matched concepts (the hai engine already shows its reasoning), never by a hidden model.
12. **Pals talk to each other:** `SAY|from|to|text` rows, each pal answering from its own bank, switchable off.

**Phase D: the pet's world (months)**
13. **Food as a created concept, farming:** plant, water, harvest, then cooking (book 26).
14. **Anatomy and bodies:** digest, waste, breathe; weight-adjustable bodies; cyborg parts; later evolving forms (the "terumon" mini sim).
15. **Sandbox for real folders:** an agent's body can be a real folder on the desktop or a data file on the web, with escape tests first.
16. **Shared data:** bodies and settings through `trunk_op` (trunk plus per-agent branches).
17. **Texting between phones:** reuse the house phones and router (design only today).

**Phase E: one implementation (when the third rule set arrives)**
18. Single source for the rules shared by C and the web (no C-to-wasm compiler exists here today; the house vendors wasm3), keeping the differential test as the guard.

**Standing decisions the owner still owes (defaults in brackets):** the party order authority [the ledger]; whether the master's memory follows the slot [yes]; reuse `survivor_op` [yes, needs the quarantine release]; a native speaker for the Japanese lines.

---

## Part 9. Troubleshooting and traps

- **The page says "could not load":** open it from a web server or the live site, not from a file (the browser blocks `fetch` on `file://`).
- **The left bar still shows palettes or the old party:** choose the pets bar (bottom "pets" cell), then do any action; the list is rewritten when the editor's core runs.
- **A test reads a stale file:** delete the file first. A file the program did not just write proves nothing.
- **`EXPECT_HAS` misses text in a big file:** it reads only the first ~8 KB; use `RUN | grep -q` and `EXPECT_EXIT`.
- **The CPU gate refuses a heavy job:** another agent holds the lock; wait, do not bypass.
- **Clicks do nothing in the editor:** the first click only focuses a cell; click twice; typed digits can select the wrong item.
- **Numbers differ between C and JS:** read both through the rules file; run `hai_compat`.

## Part 10. Glossary

**Ledger:** the append-only file of rows; the only truth. **Projection:** a file derived from the ledger by a named op, safe to delete. **Pal:** an agent in the world. **Master:** the first pal in the order. **Page:** a room; a page is a session. **Book:** a project that opens in the editor. **Body:** the data file that gives a pet its needs. **Weight:** a multiplier on a need (10 = normal, 0 = off). **REFUSED row:** a request the rules did not allow, kept so it can be audited. **Differential test:** two implementations run on the same random input and compared byte for byte.

## Part 11. Where to read more

hai repo: `book/22` (teaching), `23` (agents and sessions), `24` (how hai answers), `25` (embodied agents), `26` (the pet), `27` (master, slave and the order). House: `HQ-IQ-BOOK/02-architecture/HAI-AND-PALS-REUSE-AND-COMPATIBILITY-GUIDE-2026-11-24.md`, `.../HOW-TO-BUILD-M-MOVE-MODE-AND-REORDER-HANDOFF-2026-11-24.md`, `16.game/OPEN-A-PROJECT-AS-A-BOOK-IN-RPG-EDIT-...-2026-11-23.md`, `02-architecture/STANDARD-RULES-AS-OPS-FOUNDATION-DESIGN-2026-11-18.md`, `NO-GHOST-DATA-THE-LEDGER-IS-THE-AUTHORITY-2026-11-07.md`, `TRUNK-AND-BRANCH-SHARED-DATA-STANDARD-2026-11-16.md`.
