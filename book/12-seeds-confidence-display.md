# Seeds, confidence, and the display fix (built 2026-11-22)

Labels: **BUILT** (repo, harness case, observed) · **HAND-SET** (a value a person chose and can edit) · **GAP** (known, not done).

## 1. Seeds: hand-set starting counts (HAND-SET)
- File: `chat/data/seeds.pdl`, rows `SEED | concept | reward | punish`. Starting values: greet 4/0, help 2/0, account 1/0, nav 1/0, bye 1/0. So greet starts at 5/6 = 0.83, help at 0.75, the rest at 0.67.
- Seeds are counts like grades: applied before the ledger, shown in `stats`, and moved by grades.
- A seed for a concept that does not exist is **refused** (exit 2, "seed refers to an unknown concept"), never ignored.
- No seeds file means no seeds. The learning case (`hai_learn`) runs on a seedless copy, so its exact numbers still hold.
- Built in C (`ops/hai_chat_core.c`, `apply_seeds`) and JS (`docs/engine.js`, `parseData`). Parity: 29 output lines identical, with seeds and confidence on.

## 2. Confidence per reply (BUILT)
- For each turn: `confidence = round(100 × matched words ÷ input words × weight of the chosen concept)`. No match gives 0%.
- The reply shows it: `... (confidence 83%)`. The turn row keeps it as a ninth field.
- Worked numbers (in `cases/hai_confidence.pdl`, checked by hand): "hi" 83%, "hello there" 42%, "help" 75%, "weather" 0%.
- The grade still judges the reply. Calibration (does 80% get good grades 80% of the time?) is the next measure and is NOT built (GAP).
- Confidence is about **routing** (which concept was chosen). Replies are fixed, so it says nothing about how well a sentence reads.
- Mutant (weight ignored): the case goes red (4 FAIL).

## 3. Display fix (BUILT)
- The demo page used to line up reply texts against **all** ledger rows. A grade row is a row too, so every reply after a grade showed the wrong text, or a bare reply id such as `r_fallback`. The page now keeps texts per turn only, and shows each grade as its own line ("graded 1 (reply r_greet)").
- Rows imported from a file have no stored text, so they show "(reply X, text not stored)". That is honest, not a bug.
- The demo's greeting now shows the seeded 83% (it had shown 50% when the seeds were not loaded).

## 4. Bundle and window
- `hai_bundle` and `docs/bundle.js` accept TURN rows with 8 or 9 fields (the 9th is the confidence). Older 8-field rows still import.

## 5. Harness status (observed 2026-11-22)
| Case | Result |
|---|---|
| hai_chat | 17/17 |
| hai_learn (seedless copy) | 16/16 |
| hai_confidence (new) | 11/11; weight mutant 4 FAIL |
| hai_account | 17/17 |
| hai_core | 4/4 |
| hai_bundle | 10/10 |
| hai_event | 12/12 |
| hai_elements | 8/8 |

## 6. GAP
- Calibration of confidence against grades is not built.
- The seeds are hand-set by design; there is no way yet to propose a seed from data (that would be a candidate row for a person to approve, as in the attrition model).
- JS parity for the seed and confidence path is run by hand, not as a harness case.
