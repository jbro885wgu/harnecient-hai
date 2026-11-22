# M5: learning from grades, and the GitHub Pages demo (2026-11-22)

Labels: **BUILT** = in the repo, with a harness case and an observed result. **DESIGN** = not built.

## Built (C is the authority; the browser copies it)
- **Grades 0 to 4** (`hai_chat grade`, or type `grade N` in the CLI). Each grade is a `GRADE` row in the ledger. Reward and punish come from RULE rows (`grade_<g>_reward`, `grade_<g>_punish`) and are written into the row, so the ledger alone decides the weights. BUILT.
- **Weights**: Laplace `(reward+1)/(reward+punish+2)` per concept and per reply; 0.5 with no grades. BUILT.
- **Weights act in two places only**: a tie in word matching goes to the higher weight; the fallback options are listed highest weight first. Replies are never rewritten. BUILT.
- **Daily cap**: `grade_daily_cap` (10) per ledger per day. The 11th grade is refused. BUILT.
- **One grade per reply**: a second grade on the same reply is refused. BUILT.
- Harness `cases/hai_learn.pdl`: 16 checks, all PASS. Mutant that ignores weights: 4 checks go red. Hand-computed values (Laplace) are in the case comments.
- Parity: the same 20 commands through the C engine and the JS engine give identical replies, exit codes, stats and ledger rows (timestamps masked): 43 output lines, 18 ledger rows, 6 grades.
- Other cases still pass after the change: `hai_chat` 17/17, `hai_account` 17/17, `hai_core` 4/4.

## Demo page (`docs/`, GitHub Pages)
- `docs/index.html` + `docs/engine.js` (a JavaScript port of the C engine) + `docs/data/` (copies of `chat/data/`).
- Works: chat, the menu, the state line, grade buttons 0 to 4 after the last reply, "start over" (clears the browser's own ledger).
- Limits (also on the page): no window typing, no linked documents, no observed models, no tool calls, no installer yet. Learning here is only grade counts; replies are never rewritten.
- Checked: parity above; headless Chrome renders the greeting, state line and grade buttons. Clicking a grade button in a real browser was NOT run (the page's click path is checked through the engine calls only).

## Design (not built)
- Typed input in the window; linked documents; observed models (candidate rows); tool calls (M4); the installer (M7); learning from RL scores from checks.
