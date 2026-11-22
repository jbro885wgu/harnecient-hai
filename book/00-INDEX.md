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
