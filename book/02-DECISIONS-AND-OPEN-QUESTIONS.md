# hai decisions and open questions (rev 3, 2026-11-22)

## Decided (SAID 2026-11-22)
- Dedicated terminal-style desktop app, not a web UI.
- Install by curl, the way LyfeDesk installs today.
- NO language model at all. Human-readable weights only. FSM-powered chat.
- Chat learns, does reinforcement-style weight updates, makes basic tool calls, and the user grades results.
- **Grades:** 0 = bad, 1 = good, and 2 to 5 = better (the owner wrote "1++ = better"; read as 2 to 5 unless he says otherwise). Grade is attached to the reply the bot gave, not to the words typed.
- **Linked documents:** the user can link a document; hai reads it and trains attention on it at once. Design in section 3.
- **Options:** when the bot is not sure, it offers options (not one guess). Each option carries a confidence. The grade is based on the response and on other signals (section 4).
- **Licence:** restrictive, but readable (SAID: "restrictive but open source"). See the licence section; the word "open source" needs one correction.
- House rules unchanged. Docs in HAI-IQ-BOOK.
- Owner (jbro885wgu) has ADMIN on `harnecient-hai` (checked 2026-11-22).

## 1. Found in the house (READ, 2026-11-22) that changes the plan
- **`&.hq-apps/chat-hai/` already exists.** It is a model-driven testground (Ollama/qwen on the Mac, personas as `.pdl`, a `transcript.ledger`, an Xft side-bar renderer `ops/chat_hai_render.c`). Its design says "the app decides who speaks, the model only generates text". hai reuses its shape (personas as `.pdl`, ledger, feed) with the models removed. Do not confuse the two names: `chat-hai` is the testground, `hai` is the product.
- **House window standard is CHTPM** (`XHTPM-WINDOW-BUILD-GUIDE.md`, `chat-hai` notes section 2): named screens (`.chtpm` layouts), a `piece.pdl` menu, one dispatcher op, one render op that writes only `view.txt`. A custom ASCII terminal is the pattern the house already rejected once (see the Haiku handoff section 0).
- **PROPOSAL for question 5 (window):** build hai as a CHTPM screen set (one screen = the chat feed + composer). Reuse the feed and layout ideas from `chat_hai_render.c`, but not its renderer. Reason: one renderer for the house (CENTROID_GOLD_STD), and the nav system the owner wants users to learn is the CHTPM nav. Unverified: that CHTPM can give a true terminal feel (scroll, input line). First check is M1.

## 2. Licence (needs one owner decision)
- "Restrictive" and "open source" cannot both hold in the usual sense. Open source (OSI) licences allow commercial use and redistribution, so they are not restrictive.
- The middle option is **source-available**. The usual one is **PolyForm Noncommercial 1.0.0**: anyone can read, run and modify for non-commercial use; commercial use needs a licence from the owner. Other options: Business Source License (converts to open source after a set time), Fair Source.
- PROPOSAL: PolyForm Noncommercial 1.0.0, with the copyright holder (the owner) able to change the terms later. The README says "source-available", not "open source".
- OWNER DECIDES: confirm PolyForm NC, or pick another.

## 3. Linked documents (design, PROPOSAL)
- A user links a file or URL to hai. hai reads it as text (markdown, txt, pdf-to-text), splits it into sentences, and writes concept and reply rows tagged `SOURCE|<doc id>|<user>|<unix>`.
- Attention is trained at once: the new rows get a starting weight and are ranked immediately by `harness_bank_op find`. No model, no training run.
- Linked content is untrusted input: size cap (RULE row), no code is run from it, no links are fetched automatically, and a doc can be unlinked, which removes its rows (ledger rewrite by verb, not a hand edit).
- A doc cannot change tool weights directly. It adds concepts and replies; tools keep their own weights.
- Harness case: link a doc with a known sentence; ask the question it answers; the right reply ranks first. Unlink it; the reply drops out.

## 4. Options, confidence, and implicit grading (PROPOSAL)
- Each reply is chosen from the top candidates in the bank. If the top score is below a threshold RULE row, the bot shows up to three options with their confidence (score x weight, shown as a number).
- The user's grade (0 / 1 / 2-5) is the main reward. Other signals, all as rows:
  - the user picks one option: that option +1, the others 0 (not punished; they were only offered);
  - the user asks the same thing again within a short time: the last reply -1 (a rephrase is a failed answer);
  - the user closes the chat, or types nothing for a long time: no reward (no signal, so no row).
- Confidence is shown to the user so a low-confidence answer is visibly offered as a guess.
- Harness trust order stays: a harness verdict on a tool call outranks any user signal.

## 5. Install (READ)
- House pipeline: `xyz-installer-dev/tearit-install/install.sh` (`curl … | sh -s -- tearit-hq`).
- PROPOSAL for hai: `… | sh -s -- hai`, app under `$HOME/hai`, launcher `~/.local/bin/hai`. OPEN: the house installer points at `tearitco/*`; the hai URL points at `jbro885wgu/harnecient-hai`. Pick before M7.

## 6. Open questions left
1. Chat topics for MVP (default: hai itself, the nav system, making an account).
2. Daily cap on user-grade reward per profile (default: 10).
3. Confirm the grade reading: 0 bad, 1 good, 2-5 better.
4. Licence: PolyForm NC, or another source-available licence?
5. Window: CHTPM screen set (proposal), or a standalone window?
6. Commit the plan to `#.hai` and push to the repo now, or wait?

## Not yet verified
- CHTPM terminal feel (scroll and input line): not built, not checked.
- The house installer end to end (last recorded run 2026-09-02).

## 7. Grade scale (PROPOSAL, 2026-11-22)
- User-facing grade: 0 bad, 1 good, 2–5 better. Not 0–256: finer grades are false precision and make the learner noisier.
- Stored weight: unchanged, the Laplace ratio in 0–1 from reward and punish counts.
- Grade to counts: a fixed table in RULE rows, for example `RULE | grade_0 | punish 1`, `RULE | grade_1 | reward 1`, `RULE | grade_2..5 | reward 1 + bonus`. Tuned by editing rows, not C.

## 8. Decided 2026-11-22 (SAID, owner)
- **Grade scale: 0 to 4**, stored as a 3-bit field (values 0–7 fit; 5–7 reserved, not offered to users). Meaning: 0 bad, 1 good, 2–4 better. Binary-friendly, so a later 8-level scale needs no format change.
- **Window: reuse the X11-HQ window for MVP**, and make it friendlier than a raw terminal (labelled buttons for the common actions, the nav menu visible, a plain-language help line). Owner: "x11 window is great for now, reuse hai but make it user friendly".
- **Build started:** M0 (repo skeleton) in `#.hai/harnecient-hai/` (clone of `jbro885wgu/harnecient-hai`). Nothing committed or pushed yet.
- **Licence:** still pending the owner's choice of wording (PolyForm NC proposed). The LICENSE file is not written until he confirms.

## 10. M2 built 2026-11-22: instant local account, self-building case
- Op: `ops/hai_account.c` (`create` / `login`). Passphrase: PBKDF2-HMAC-SHA256 via OpenSSL, random 16-byte salt. Ledger row `CREATE|name|salt|hash|iter|unix`, append-only, each row keeps its own iteration count so raising the cost later keeps old accounts valid. Numbers live in `rules/hai_rules.pdl` (name 3-24, pass 8-128, 100000 iterations), never in C; a missing rule exits 9.
- Case: `cases/hai_account.pdl`, run by the house harness. It builds the op from source in a scratch dir, then runs 17 checks, each able to fail: duplicate name, wrong passphrase, unknown account, short name, short passphrase, missing rule, ledger holds the row, ledger keeps its iteration count, and no plaintext passphrase in the ledger.
- Observed: `harness_verdict_op` → `VERDICT|PASS|passed=17|failed=0`.
- Negative control: a copy of the op with the wrong-passphrase check removed turns `login-wrong-passphrase-refused` and `wrong-says-why` red (run output, FAIL rows).
- M1 core test moved into the house harness: `cases/hai_core.pdl`, `VERDICT|PASS|passed=4|failed=0`. Its checks run the 8-check binary built from source.
- Self-building: the case compiles the op and the test from source in its scratch dir, so one harness run is the build plus the test.
- SELF-REPAIR, honest status: NOT built. A verdict names the failing check, and that is the input a repair would need. Auto-applying a code change is refused: house rule (auto-promotion is never built; a person reviews). What exists is the failing-check name and the red verdict.

### Debts (open)
1. Passphrase is on the command line, visible in `ps`, until a prompt verb exists (M2b).
2. The cases name the hai repo by absolute path, because the case op expands only `$T` and `$HOUSE`. Fix: give the case op an env var for an external repo, or move hai into a house path. Owner decision.
3. Case labels: the case op labels every check by the CASE name, so the 17 checks all print `build/...`. Cosmetic; one CASE row per group would fix it.
4. `results/` is gitignored. The verdict file path for an absolute `RESULTS` is broken (`$cdir//abs`); not hit by the real case, which uses a relative path.
5. Not yet: the M3 chat, the window, the installer.

## 11. M3 built 2026-11-22: FSM chat, data-driven, state replayed from the ledger
- Engine: `ops/hai_chat_core.c` (used by the CLI `ops/hai.c` and the op `ops/hai_chat.c`). Data in `chat/data/`: `concepts.pdl` (words to reply, with the option label), `replies.pdl`, `states.pdl` (FSM rows `NEXT | from | concept | to | action`, first match wins), `chat_rules.pdl` (input_max 200, options_max 3).
- Ledger row `TURN|unix|input|concept|reply|state_after|action|typed_name`. State and name are REPLAYED from these rows on every call. Nothing is held in memory.
- Classification: the concept with the most matched word tokens wins (ties by file order). No match goes to the fallback, which offers options instead of a guess. Numbers 1 and 2 are words of the help and bye concepts, so the nav standard works in the chat too.
- Case: `cases/hai_chat.pdl`, 17 checks, `VERDICT|PASS|passed=17|failed=0`. It covers the greet, unknown words getting options, the account asking for a name, the name stored and used, restart replaying the name, number 1, bye (exit 1), a closed chat refusing, the name row in the ledger, and missing setup refusing (exit 2).
- Negative control: a copy of the engine that ignores the ledger (compiles, skips the replay) turns 7 checks red: the state, name, restart, closed-chat and ledger-row checks.
- Piped session: `printf 'hi\naccount\nbo\nweather\n2\n' | ./ops/hai data ledger` gives the expected replies, and a restart keeps the chat closed.

### Honest limits (say these in the UI and the docs)
- Option labels come in file order, not by relevance. They are not yet ranked by weight. That is M4 (tool ranking) and M5 (learning).
- In the name state, any word becomes the name, including "help". Acceptable for now; a guard row is owed.
- Replies are fixed rows. There is no free text, by design.

### Debts
- Absolute repo paths in the case (the same debt as M2).
- The mutation pass is one mutant (ledger ignored). Owed: a second mutant on the FSM rows, so a wrong transition is also caught.

## 12. DECIDED 2026-11-22 (SAID, owner): the window ships, relay driving stays
- Owner: "i do want the x11, even if we have to ship that. ... see development on screen, and use relay driving techniques."
- What it means: hai's window is an X11-HQ app drawn by the house renderer (`khtpm_core_render.c`, 24,825 lines; needs Xlib, Xft and Shape), so the same relay (`entity_menu_history/<pid>.txt`) drives it as it drives every house window.
- Precedent for shipping it: the `tearit-install` payload already ships the renderer for `tearit-hq` (taskbar, login, cursword). hai's payload follows that shape.
- Split (PROPOSAL, recommended): the engine (account, FSM chat, ledger) stays headless and is tested by the harness with no display. The window is a skin over the same engine, driven by the relay.
- Blocker for testing: no Xvfb here, and `DISPLAY=:0` is the owner's live desktop. A window test on the live desktop needs the owner's OK first (house rule for LIVE cases). Owner can install Xvfb (`sudo apt install xvfb`) so window tests run on a scratch display.
- Next (W1): an app copied from `&.hq-apps/_template-hq/` into `hai/window/`, with a manager op that reads the engine. Acceptance: a window opens, the relay types `hi` and gets the reply on screen, a PNG is opened and described, and a negative (a closed window must not count as open).
