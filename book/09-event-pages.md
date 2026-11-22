# E1: event pages for haibot, one interpreter in C and in JS (built 2026-11-22)

Labels: **BUILT** (repo, harness case, observed) · **HOUSE** (the house's own event architecture, read) · **GAP** (known, not done).

## 1. What was built
- `haibot/hai_event.c`: runs one event on a page file. C op, prints `ROW`, `FACT`, `DONE` lines; refuses anything outside the fixed row set, naming the line.
- `docs/event.js`: the same interpreter in JS, for the browser demo. Same rows, same operands, same output.
- `haibot/events/heal_by_rest.pdl`: two sample pages. `rest` heals a fainted haibot once it has rested enough (`$rest_turns_to_heal`); `cast` refuses a spell when MP is short (`$spell_cost`), else casts it and spends MP.
- `haibot/events/rules.pdl`: the numbers, as RULE rows.
- Harness `cases/hai_event.pdl`: 12/12 PASS. The negatives are unknown row kind (refused, line named), unknown IF operator, missing rule, missing fact. A mutant that defaults a missing rule to 0 turns `missing-rule-refused-not-defaulted` red (1 FAIL).

## 2. The fixed row set (this is the contract; anything else is refused)
| Row | Meaning |
|---|---|
| `PAGE \| name` | starts a page |
| `WHEN \| event` | the event that runs this page |
| `IF \| left \| op \| right` | a condition; all IFs of a page must hold. op: `eq ne ge gt le lt` (numbers compare as numbers, else text) |
| `THEN \| APPEND \| field...` | writes one ledger row, fields joined with `\|` |
| `THEN \| SET \| key \| value` | sets a value for the rest of this run (not saved) |
| `THEN \| STOP` | stops the page's remaining THEN rows |

Operands: `$name` is a RULE value, `@name` is a fact (a value SET earlier in the run wins over the loaded facts), anything else is a literal. A bare name is always a literal: there is no guessing.

Exit codes: 0 ok, 2 bad page or row (line named), 3 missing rule or fact.

## 3. The JS parity check (run 2026-11-22)
Four runs on the same files, C op and JS function, compared line for line (ROW, FACT, DONE, and the exit code):
- `facts_rested` with `rest`: identical (5 lines)
- `facts_rested` with `cast`: identical (4 lines)
- `facts_short` with `rest`: identical (2 lines)
- `facts_short` with `cast`: identical (3 lines)

How to repeat: run `haibot/hai_event <page> <rules> <facts> <event>`, then `node -e` with `require("./docs/event.js").runEvent(...)` on the same text, and diff. This parity is NOT yet a harness case (GAP below).

## 4. What is NOT the house event architecture yet (GAP, honest)
- The house's real event pages are `events/<name>/event_pkg/pages/page_1/{condition.pdl, event.ir.pdl}`, compiled to pal, with a wrapper per command (HOUSE, `EVENT-ARCHITECTURE-ONBOARDING-2026-11-05.md`). This interpreter is a **small subset for hai**, not that format. Making hai pages load the house format is a later step, and it must keep this row set as the subset.
- JS parity is run by hand (section 3), not by a harness case. Owed: a case that runs both, or a shared fixture file both sides must match.
- The sample pages are not yet wired to chat, the window, or a grade. They run only when the caller names the event.
- The interpreter does not write the ledger itself. The caller takes the `ROW` lines and appends them through the haibot's own verbs (the rule the ledger doc sets: the ledger is the only authority).
- Pages have no loops, no calls to ops, and no time. That is deliberate for E1.

## 5. Next (E2 in book 08)
Export and reload (`.pdl`, `.csv`), round trip C to JS to C. The page files above are already `.pdl` rows, so they export as they are.
