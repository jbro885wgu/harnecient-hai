# hai: the gamified viewer, example tools, links, watching, hangman battles and p2p (design, 2026-11-22)

Labels: **BUILT** (repo, harness case, observed) · **DESIGN** (written, not running) · **HOUSE** (already in the house, reused or checked) · **OWNER** (said by the owner, date).

## 0. What the owner asked for (OWNER 2026-11-22)
- The demo should show example tool calls, example links, and example watching, in a playful, educational space: "not a game" in the words users see, but not boring either.
- A gamified viewer of your HAI agent: it levels up, learning skills move stats up, badges appear.
- The same viewer in the X11 software ("id really like that").
- A battle mode: your agent plays another agent at hangman, with its own ELO score.
- Two hai agents talking over the existing p2p layer.

## 1. Honest status first (what can and cannot run today)
| Piece | Status |
|---|---|
| Typed chat, FSM, ledger replay | BUILT |
| Grades 0 to 4, Laplace weights, options by weight | BUILT (M5, `hai_learn` 16/16) |
| Tool calls chosen by rank (M4) | DESIGN. Nothing calls a tool yet. A demo that shows tool calls today would be fake. |
| Linked documents | DESIGN |
| Watching other models | DESIGN |
| Gamified viewer (levels, stats, badges) | DESIGN (section 3). Derivable from the ledger, so it can be built next. |
| Hangman battle with ELO | DESIGN (section 4). No network needed for the first version. |
| Two hai agents over p2p | DESIGN (section 6). Blocked by authentication (below). |
| X11 viewer | DESIGN, built on W1 (the window's typed input, still to verify) |

Rule for the demo: **any feature that is DESIGN must be shown labelled "preview, not running yet".** A demo that acts out a feature is the ghost-data problem in another form, and it would mislead the people we want to trust hai.

## 2. Example tools, links and watching (what the demo shows, once each exists)
- **Tool call**: the user types a request, the engine picks a tool by rank (weights), the tool runs, its result is written as a row, and a harness check says whether it worked. Example tool: `list_pages` (lists the window's pages). Shown in the demo as a row with its PASS/FAIL.
- **Link**: the user pastes a short text; hai splits it into sentences, writes `SOURCE` rows, and uses them at once. The demo shows the before and after answer.
- **Watching**: an observer reads a text file another model wrote and writes candidate rows (capped weight). The demo shows the candidate, then the grade that confirms or drops it.
- Each example is a fixed script in `docs/examples/`, labelled as an example, and the same script runs in the real engine in a harness case, so the demo and the engine cannot drift.

## 3. The gamified viewer: levels, stats, badges (DESIGN, all derived from the ledger)
The viewer is a **progress view**. It is never stored: it is recomputed from the ledger each time, the same way the state is.

- **XP**: one point per graded reply (grade 1 = 1 XP, grade 2 to 4 = grade XP, grade 0 = 0 XP but counted). Plus one point per verified tool call once M4 exists. XP per grade is a RULE row.
- **Level**: a table of XP thresholds in a RULE file (`level | xp_needed`). The level number is the only output; no level name is hard-coded in C.
- **Stats** (skills) are **concept groups**: each group is a set of concepts (for example greetings, navigation, help, accounts). A stat is the mean weight of its concepts, shown as a bar. The group list is data (`stats.pdl`), so a teacher can add a skill without code.
- **Badges** are rows in `badges.pdl`: `BADGE | id | name | rule`, where a rule is a count over the ledger, for example "first graded reply", "ten grades of 3 or more", "three bad grades turned into a good one". The badge is earned when the count is reached, and the earning is written as a `BADGE|...` row so it stays earned.
- Nothing in the viewer changes the chat. It only reads the ledger.

Harness plan: a case per rule (XP from a known ledger; level at a threshold, one below and one above; a stat from known weights; a badge earned once and not twice). Negative: a hand-made progress file is ignored (the viewer must rebuild from the ledger).

## 4. Hangman battle with ELO (DESIGN)
- **Game**: two agents, A and B, each holds a secret word from its own concept list (the words it knows). They take turns guessing letters of the other's word. A wins by spelling the word first; B wins the same way; a fixed number of wrong guesses ends a round as a draw.
- **Guessing uses the weights**: a guess is the letter that appears most in the agent's own words, weighted by the weight of the concept it came from. So the learning shows in play: an agent that is graded well guesses better. This makes the battle a real test of the learning, not a coin flip.
- **Moves are rows**: `MOVE|match|turn|from|to|letter|hit|unix`. The match result is one row: `MATCH|id|a|b|winner|unix`. Each agent's ledger holds the rows of its own matches, and the other agent's moves come in over the link (section 6). Nothing is kept in memory.
- **ELO**: the house ELO formula (chess-style, expected score from the rating difference). Two notes from reading the house code:
  1. `TSC_ELO` (`@.apps/TSC_ELO/ops/tsc_elo.c`) writes its ratings into **user data** (`xyzfs/users`). hai must not reuse that file; it keeps its own ratings file in its own ledger.
  2. `tsc_elo` hard-codes K=32 and a start of 1000. In hai both are RULE rows (`elo_k`, `elo_start`), per the house rule that numbers live in rows.
- Ratings are a projection of `MATCH` rows: delete them and they rebuild the same bytes.
- Harness plan: a scripted match with a known outcome gives known ratings (computed by hand, written in the case, as in `hai_learn`); a mutant with K changed turns the case red; a replayed ledger gives the same rating twice.

## 5. Shared rules (house, unchanged)
- Every number in a RULE row; every rule an op with a `--describe` line; a harness case per rule that can fail.
- Ledger is the only authority. Views (the viewer, ratings, stats) are projections: delete them and they rebuild.
- No Python, no new `.sh` logic, no network in a harness case.

## 6. Two hai agents talking: p2p (DESIGN, with a security stop)
- The house has a p2p layer (`palnet_peer`, in several apps). Read from the source: it **binds to 127.0.0.1 by default**; a LAN bind is opt-in (`PALNET_BIND`); and the code says there is **no authentication yet** (PAL-NET-STANDARD section 7). So today it is a same-machine or trusted-LAN test only.
- **Stop before the internet**: two agents on the open network need authentication first. Plan: a pairing step where each agent prints a short code and the other types it, then every message carries a key derived from that pairing, and a message with a bad key or an old sequence number is refused. Until that is built and tested, two agents talk only on one machine, or on a LAN you trust.
- Message contract for the battle: `MOVE` and `MATCH` rows only, with a sequence number, so a dropped or repeated message is detected and refused. A harness case replays a duplicate and a reordered message and expects a refusal.
- A "talk" between agents (not just battle) is a chat row with the sender's name; the same contract, same checks.

## 7. Build order (proposal; OWNER to confirm)
| # | Piece | Needs the network? | Harness |
|---|---|---|---|
| G1 | progress from the ledger: XP, levels, stats, badges (projection) | no | `hai_progress` |
| G2 | hangman match engine, two agents on one machine, ELO from MATCH rows | no | `hai_hangman` |
| G3 | the html viewer shows G1 and the examples (labelled) | no | parity with C, as in M5 |
| G4 | the X11 viewer: stats bars and badge shelf on the window | no (needs display) | screenshot, opened and described |
| G5 | pairing and authenticated messages | yes | replay and reorder cases |
| G6 | two agents over the link | yes | two-process case on one machine first |
| M4 | tool calls (needed for the tool example) | no | `hai_tools` |
| L1 | linked documents (needed for the link example) | no | `hai_links` |
| W1 | typed input in the window | no | screenshot |

## 8. Decisions the owner needs to make
1. **First playspace**: hangman battle (my recommendation: it teaches vocabulary, it is the cleanest test of learning, and it has no art to make) or a chemistry shelf (elements as concepts, reactions as rules) or a voxel pet (art-heavy, later).
2. **Badge list** (the first ten), and **XP per grade** (my default: 1 for grade 1, grade value for 2 to 4).
3. **ELO start and K** (my default: 1000 and 32, both rows, so they can change).
4. **Pairing for p2p**: short code typed by a person (my recommendation) versus a key file copied across.
5. **Demo honesty**: OK to show DESIGN features only as labelled previews? (My recommendation: yes, labelled; never as working.)
