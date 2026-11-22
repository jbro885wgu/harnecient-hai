# hai MVP plan (rev 2, 2026-11-22: NO MODEL)

Supersedes rev 1, which assumed a local language model. SAID 2026-11-22: "we wont
use any model. we are only going to use human readable weights. we will have an
FSM powered chat." Rev 1 is kept in git history only.

## 1. What it is (SAID 2026-11-22)
- A dedicated desktop terminal app ("works just like a terminal"), for hai only.
- Purpose: teach users our nav system, then move them to LyfeDesk later.
- Opens from a terminal command and from a launcher. Installs by curl.
- Instant local account (separate from LyfeDesk; can join later).
- An FSM-powered chat that:
  1. chats (replies come from states and rows, not from a model),
  2. learns (weights move from results),
  3. does reinforcement-style updates (reward and punish counts per action),
  4. makes basic tool calls (fixed ops, scored by the bank),
  5. lets the user GRADE each result, and the grade moves the weights.
- Everything a learner changes is a plain `.pdl` row or a weight file a human can read and edit.

## 2. Architecture (READ: it is the house's own, already in 17.ai)
This is the house pattern from `17.ai/HARNESS-AS-GAME-BRAIN.md` and `META-STRATEGY-...-2026-11-22.md`, applied to chat:

| Piece | House part it reuses | Human-readable form |
|---|---|---|
| Chat states (greet, ask-name, idle, answer, offer-tool, confirm) | FSM (decision layer, AI roadmap) | `states.pdl`: `STATE | name | next | on_intent` |
| Intent from words | `harness_bank_op find` ranking: keywords, synonyms, concept slots x weight | `concepts.pdl`: `CONCEPT | open | words...` |
| Reply text | fixed reply rows per state, with slots filled from the ledger | `replies.pdl` |
| Tool call | ops in the house (for MVP: `help`, `list_pages`, `open_page`), each a `--describe` line | `tools.pdl`: `TOOL | name | op | args` |
| Reward and punish | `harness_bank_op update` (Laplace weight `(r+1)/(r+p+2)`) | `bank/*.behavior.pdl` |
| User grade | a new verb: user marks a reply good (+1) or bad (-1) | row in `chat_ledger.txt` (append-only) |
| Harness check | a case that FAILS when a weight is wrong | `cases/hai_*.pdl` |

Ledger is the only authority for account and chat history (house no-ghost-data rule).

## 3. Learning, stated plainly (PROPOSAL, owner to confirm)
- "RL" here is count-based bandit learning: each (state, reply or tool) keeps reward and punish counts; the weight is the Laplace ratio; the bank ranks choices by it. It is not gradient training.
- Reward sources, in order of trust:
  1. harness verdict on a tool call (measured, the strongest),
  2. the user's grade (a human, can be noisy or gamed),
  3. nothing else.
- A user grade alone moves a weight only up to a cap per day (RULE row), so one person cannot make a reply dominant by repetition. (PROPOSAL.)
- Weights are kept per user profile and can be exported and inspected as text.

## 4. Honest limits (state these in the docs and the UI)
- A non-model FSM chat is narrow. It can answer the phrases it has concepts for, and falls back to a clear "I don't know that yet, here is what I can do" reply. It does not write free prose.
- Growth comes from new concept rows and reply rows, added by people or by reviewed batches. A weight alone cannot invent a new answer.
- This is a product decision as much as an engineering one. Confirm it before M3.

## 5. Milestones and acceptance (each can FAIL)
| # | Milestone | Acceptance (must observe) |
|---|---|---|
| M0 | Repo skeleton: README, LICENSE, HAI-IQ-BOOK, install stub | `git ls-remote` shows main; stub prints version |
| M1 | `hai` opens a terminal window | window appears; PNG opened and described; a closed window must not count as open |
| M2 | Instant account | first run creates profile; second reuses it; wrong passphrase refused |
| M3 | FSM chat: greet, ask name, answer 5 known concepts, fallback reply | harness case: each intent maps to its reply; an unknown phrase gets the fallback (negative) |
| M4 | Tool calls: `help`, `list_pages`, `open_page` chosen by bank rank | the right tool ranks first; a wrong weight flips it and the case goes red; each call logs a row |
| M5 | Learning: reward and punish move weights; a grade moves them, capped | after grades, `find` ranking changes as expected; the daily cap refuses the 11th push |
| M6 | Grade UI: good/bad after each reply | a grade row appears in the ledger; a replayed ledger rebuilds the same weights |
| M7 | Installer | fresh user dir: curl install then `hai` works; uninstall removes it |

## 6. Rules carried over (house, unchanged)
- Pal + case `.pdl` + C ops. No Python. No new `.sh` logic (the installer is the one shell file).
- Every rule is an op called by a state or page; numbers in RULE rows; no numbers in C.
- Ledger is the only authority; a hand-edited ledger is refused on replay.
- Heavy jobs through `cpu_gate` and `nice -n 15`, one at a time.
- Harness cases never touch the network.
- Commits on the `hai` branch, pushed only when the owner says.
