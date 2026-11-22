# haibot_<pid>: party, PC box, elements and hangman (design, 2026-11-22)

Labels: **BUILT** (repo, harness case, observed) · **HOUSE** (already in the house, checked by reading) · **DESIGN** (written, not running) · **NOT FOUND** (searched the house docs and code; nothing exists).
Owner's words are quoted as **OWNER 2026-11-22**.

## 0. The idea (OWNER 2026-11-22)
> "a pet that plays hangman ... user can have multiple agents or pets ... these pets are the muchipets/terumon ... they can use elemental magic and learn it for healing or harm depending on its body / armor ... up to 6 agents in party like pokemon + a pc for storing more ... call it haibot_<pid>."

So a **haibot** is a hai agent that also has a body, a level, armour, and spells. It plays hangman against other haibots. A user can own several. Up to **6 in the party** (active), and a **PC box** keeps the rest.

## 1. What already exists (checked)
| Thing | Where | Status |
|---|---|---|
| hai agent: FSM chat, grades, weights, ledger replay | this repo | BUILT |
| hangman battle, ELO | book `06-viewer-tools-links-watch-battle-p2p.md` | DESIGN |
| party of 6 | `&.widgits/db-hq/data/system.pdl` (SYSTEM PetParty member1..6), rpg-pet / rpg-space `party.pdl` | HOUSE |
| piece = one uid, a user's pets are pieces (`piece_mint`, Master Pal List) | memory `project-pieces-master-pal-list-2026-10-28` | HOUSE |
| terumon: levels by MP skills, forms, shapeshifting, tree lint | `XO/30.rpg-dust-work/oct-28/TERUMON-SHAPESHIFT-AND-EVOLUTION-TREES-DESIGN.md` | HOUSE (design; 2 parts BUILT: `tree_lint` 53 checks, `views_to_voxel` 60 checks) |
| PC box / storage of pets | searched house docs and code | **NOT FOUND** (designed here) |
| elemental magic (elements, spells, heal or harm) | searched house docs and code | **NOT FOUND** (designed here) |
| body and armour as stat rows | the terumon doc has body plans and stat multipliers; the rpg-pet actors have `def`, `mdf` | HOUSE (partial) |

## 2. Identity: haibot_<pid> (DESIGN, uses the house piece rule)
- `<pid>` is the **piece id** of the house (one uid per piece, minted by `piece_mint`). A haibot is therefore a piece, so the house's identity, ownership and audit rules apply without new code.
- The name is `haibot_<pid>`. The nickname a user gives is a row, not the id (the id never changes).
- Each haibot has its **own ledger** `haibots/haibot_<pid>/ledger.txt` (its learning, matches, XP, badges). The user's **roster ledger** `roster/ledger.txt` holds who is in the party and who is in the PC. Nothing is kept in memory.

## 3. Party and PC box (DESIGN)
- **Party**: at most **6** active haibots (`party_max` = 6, a RULE row). Only party members play battles.
- **PC box**: unlimited in principle; capacity is a RULE row (`pc_boxes` × `pc_slots_per_box`). A haibot in the PC keeps learning only when it is trained by a grade, not by idle time (no free growth).
- Roster rows (append-only): `PARTY|add|pid|slot|unix`, `PARTY|remove|pid|slot|unix`, `PC|deposit|pid|box|unix`, `PC|withdraw|pid|slot|unix`. A refusal (party full, PC full, pid not owned) writes nothing and says why.
- Battle picks the **first live party member**; a fainted haibot (no HP) goes to the back, as in the familiar party games. The rule is a RULE row (`faint_rule = back`).

## 4. Body, armour and stats (DESIGN; numbers as rows)
- **Body** (stat row): `BODY | pid | hp | mp | atk | def | mdf | agi` from the base body type (data file `bodies.pdl`) and the level. Four body types to start (for example `sprout`, `stone`, `ember`, `mist`); names are data.
- **Armour** (item rows): `ARMOR | id | slot | def_add | mdf_add | element_resist=<element>:<percent>`. Equipped armour is a `EQUIP|pid|slot|armour_id` row.
- **Level**: XP from grades and matches, level table in RULE rows (see book 06).

## 5. Elemental magic: learn, heal or harm (DESIGN)
- **Elements** are concept groups (data, `elements.pdl`): for example fire, water, leaf, stone, light, dark to start. A spell belongs to one element and is **learned** when the haibot's grades in that element's words pass a RULE threshold (`spell_learn_grade_sum`). Learning is the same grade path as chat: no new mechanism.
- **Heal or harm is a sign, not a separate spell**: each spell row says what it does per point of power, for example `SPELL | ember_spark | element=fire | power=3 | effect=harm`, and `SPELL | leaf_mend | element=leaf | power=3 | effect=heal`. The same element can heal or harm depending on the **target's armour**: an armour row with `element_resist=fire:50` halves fire harm. So "depending on its body and armour" is a lookup in rows, not an if-statement in C.
- **Cost**: each spell costs MP (RULE row per spell). MP regenerates with rest turns (RULE row), so spells are a budget, not spam.
- **Where magic shows in hangman** (the game is the test of magic): a spell is an extra move. `heal` restores one wrong-guess life to the caster (up to its max). `harm` removes one life from the opponent, reduced by armour. `reveal` (an element spell) shows one hidden letter of the opponent's word. Each spell use is a `MOVE` row with the spell id and the effect, so the battle log explains itself.
- Learning in battle: a spell that won a round gets a grade-like reward row (from the battle's result), so the weight of that spell rises. This is the RL signal the owner asked for, applied to magic.

## 6. Hangman battle between haibots (DESIGN, book 06 has the rules)
- Two party members, one from each user (or two of one user's party for practice), play one round. Each secret word comes from its owner's concept list.
- Guesses use the weights (book 06 section 4). Spells are extra moves (section 5).
- Each haibot has its **own ELO** (book 06 section 4 rules). ELO is per haibot, not per user, so a user's strong haibot is visible on its own.
- Rounds and results are `MATCH` and `MOVE` rows in both haibots' ledgers. Nothing is shared except the rows the two owners agree to exchange.

## 7. Shapeshifting, and the terumon link (OPEN)
- The terumon doc's forms (wear a form from an unlocked set, pay MP, revert at 0) could apply to a haibot as **a body change**: a form is a body row with its own armour resist. It is attractive, but it is **not decided**. The owner's question list in the terumon doc applies (temporary or permanent; can a bought haibot start with forms).
- Recommendation: do not build forms in the first version. Body types and armour cover the "heal or harm by body" request.

## 8. Shared rules (house, unchanged)
- Every number is a RULE row or a data row; every rule an op with `--describe`; a harness case per rule that can fail (refusal cases included).
- Ledger is the only authority: party, PC, body, armour and spell state are projections of the roster and haibot ledgers. Delete a projection and it rebuilds identical bytes.
- No Python, no new `.sh` logic, no network in a harness case.
- A haibot in the house is a piece: no file that holds a haibot's state outside the ledgers.

## 9. Build order (proposal)
| # | Piece | Test that can fail |
|---|---|---|
| H1 | roster ledger: party (max 6) and PC, refusals | add a 7th to party → refused; withdraw from empty PC → refused; replay gives identical roster |
| H2 | body and armour as rows; stats from rows | armour resist halves fire harm (hand-computed); remove armour → harm back to full |
| H3 | elements and spells as data; learning by grade sum | a spell unlocks at the threshold, not one below; a spell with no element refused |
| H4 | hangman with spells (needs book 06 G2) | heal cannot exceed max; harm reduced by resist; reveal shows one letter |
| H5 | ELO per haibot (book 06) | known result gives known rating; K changed → case red |
| H6 | viewer shows party, PC, level, badges (book 05/06) | html parity with C, as in M5 |
| H7 | p2p battles between two users (book 06 section 6, needs authentication first) | duplicate and reordered move refused |

## 10. Decisions for the owner
1. **Party size 6** (given) and **PC capacity** (proposal: 5 boxes of 30).
2. **Element list** to start (proposal: fire, water, leaf, stone, light, dark).
3. **Spells on a grade threshold** (proposal: sum of grades in the element's words ≥ 10) or **bought/found items**?
4. **Shapeshift for haibots** now, or later (proposal: later).
5. **Faint rule**: a fainted haibot goes to the back of the party (proposal), or leaves the battle only?
6. **Armour**: crafted from grades, or bought? (Proposal: found in battle rewards, one row each.)
