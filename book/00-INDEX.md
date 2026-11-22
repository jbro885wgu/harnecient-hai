# HAI-IQ-BOOK (index)

hai (aka harnecient-hai): a small, local, private chat terminal that opens from
a shortcut or the `hai` command, makes an account instantly, chats with a local
model, and can run tools. Mini house: same install shape and docs shape as
LyfeDesk, minimal.

Labels used in every doc: **SAID** (owner said it, date), **READ** (I opened
the file), **RAN** (I ran it and saw the output), **OPEN** (unknown),
**PROPOSAL** (my idea, not decided). Anything unlabelled is a claim to check.

| File | What |
|---|---|
| `01-MVP-PLAN.md` | What the MVP is, milestones, acceptance tests that can fail |
| `02-DECISIONS-AND-OPEN-QUESTIONS.md` | Decisions made, decisions open, licence, install |
| `03-HOUSE-REUSE-MAP.md` | What we reuse from the house and where it lives |

Target repo: `github.com/jbro885wgu/harnecient-hai` (public, `main`; owner has
ADMIN; checked 2026-11-22 with `gh repo view`). Local folder: `#.hai/`.

## 04-hai-model/ (2026-11-22)
- `0.hai-model-architecture.md`: what the hai-model is (FSM + readable weights, no neural model), its parts with status, the learning sources (user input, grades, RL scores, observed models, optional distillation).
- `1.hai-user-tutorial.md`: the CLI and the window, step by step, what to check.
- `2.teach-the-hai-model.md`: how to teach concepts and replies, grades, RL, linked documents, observed models, distillation (optional).
Copies also live in `XO/%.HARNECIENT/hai-user-dox/` (docs only). This book is the authority.

## 06 and 07 (2026-11-22)
- `06-viewer-tools-links-watch-battle-p2p.md`: gamified viewer, hangman battle with ELO, examples, p2p with an authentication stop.
- `07-haibot-party-pc-elements.md`: haibot_<pid>, party of 6, PC box, body and armour, elemental magic (heal or harm by rows), per-haibot ELO. Copies in XO hai-user-dox.
- `08-events-export-element-faint-credits-armour.md`: shared event pages for C and JS, export and reload (pdl or csv), element pick, faint and healing, fake credits with a test faucet, armour by research and model proposal (rules set numbers, human review). Copy in XO hai-user-dox.
- `haibot/data/elements.pdl`, `haibot/hai_elements.c`, `cases/hai_elements.pdl`: the ten Terumon elements from the owner's card (8/8 PASS).
- `09-event-pages.md`: E1, the event interpreter (fixed row set), C op and JS port, parity on 4 runs, harness 12/12, mutant red. Gaps listed.
- `10-bundle-export.md`: E2, export and reload of a ledger as pdl or csv; round trip byte-identical; harness 10/10; mutant red; gaps listed (no row checksum yet).
