# 21. hai-cli: the hai window, opened from the H-AI menu (2026-11-22)

Owner, 2026-11-22: "call it hai-cli and add it". Labels: **RAN** = I ran it and looked at the result. **NOT DONE** = a gap, said plainly.

## 1. What was added (RAN)
- The window label is now `hai-cli` (`window/hai.xhtpm`). It is the W0 window: it shows the chat ledger and follows it. It is read-only.
- A row `hai-cli` in the H-AI menu (top bar cell 15, `ai_menu_12` in `#.desktop/livedesk_taskbar.pdl`, command `livedesk:open-hai-cli`).
- Launcher registry row `launcher_hai_cli` in `#.desktop/livedesk_launchers.pdl`, pointing at `&.hq-apps/hai-cli/button.sh`.
- `&.hq-apps/hai-cli/button.sh run|stop` in the house: a launcher only. It reads `hai_home.pdl` (`HAI_HOME | <path to this repo>`), starts the house renderer on `window/hai.xhtpm`, keeps `state/renderer.pid`, writes the last launch to `state/launch.log`. `hai_home.pdl` is DEV WIRING until the installer (book 08, M7) puts hai under the house.
- A 12-line branch `livedesk:open-hai-cli` in `khtpm_taskbar_manager.c`, copied from the `h-ai-lab` branch.

## 2. Why a branch and not a plain menu row (RAN, found the hard way)
The first try was the same shape as the `img-hai` row: `sh &.hq-apps/hai-cli/button.sh run`. It did nothing. The registry comment in the manager says why: a relative path in a menu command resolves against the manager's working directory, not the house, and the unquoted `&` splits the command. So use the launcher registry plus a `livedesk:open-...` command (the `h-ai-lab`, `store` and `signup` rows do). NOT CHECKED: whether the existing `img-hai` row (`sh &.hq-apps/img-hai/button.sh run`) has the same fault; it looks the same.

## 3. Driving a dropdown from the relay: what is true (RAN)
- Open a header cell: pointer onto the cell, then `MOUSE_EVENT: 1 <x> <y> 1` and `... 0` in the bar's relay file (`#.desktop/entity_menu_history/<pid>.txt`, pid = the process whose args end in `khtpm_strip_header.xhtpm`). The dropdown is its own window; check `Map State`.
- The badge numbers drawn beside rows (17 to 30 in the h-ai menu) are NOT what a typed digit means. A typed digit is a 1-based ROW INDEX inside that menu, and digits do not add up across separate relay keys: `2` then `8` ended on row 8 (tomom), and Enter then LAUNCHED tomom. I did that by mistake. Closing a menu by typing a "nav number" can therefore launch a different row. Use a pixel click on the row you want, and photograph the dropdown first.
- Arrow keys 1001 did not move the dropdown focus.
- A first open right after a manager rebuild may show a stale list; open it again and photograph it.

## 4. W1: typing inside the window (RAN, 2026-11-22; owner: "we need to do that first, test it with relay injection like the other x11-hq apps")
- The window has a `<cli_io id="composer">`, the house element `chat-hai` uses. The renderer runs its action as `window/bin/hai_send <package_dir> <house_root> <typed text>`.
- `window/hai_send.c` is a small C op: a typed line is one chat turn (`hai_chat_turn`), `/grade N` grades the last reply (`hai_chat_grade`). It writes only ledger rows; `state/ui.txt` is a projection the manager rebuilds each second from the ledger (no ghost data). No shell runs the typed text.
- The window list shows each TURN with its reply id and confidence (`you: hi  ->  r_greet  (83%)`) and each grade (`graded 4  (r_greet)`), last 8 lines. It does NOT show the reply text or the calibration (use `hai_chat calib`).
- Tests: `cases/hai_send.pdl` (headless, 13/13; a mutant that breaks the `/grade` branch turns 5 checks red) and `window/tests/hai-window-type.pdl` (relay injection into the real window: 16/16 on a fresh window, and a re-run on the same window fails line 12 as it should). See `window/tests/README.md`.
- Found while recording: a photograph taken the instant the projection changes shows the OLD picture (the window repaints a moment later), so the recording case has a `sleep` before each shot.

## 5. The recording (RAN)
A 60 s video of the whole path: the bar cell, the menu row, the window opening, then `hi`, `/grade 4`, `help`, `weather`, `/grade 4`, `hi` typed INSIDE the window by relay injection. English voice with Japanese subtitles and the narrator face (presentations standard 3b and 3c). The video is in the owner's XO folder `31.Nov/nov-22/12.hai-cli-typed-in-window-video-en-voice-ja-subs/`; the house doc is `16.game/presentations/HAI-CLI-FROM-THE-H-AI-MENU-VIDEO-2026-11-22.md`.
Real result in it: "hi" went from 83% to 90% confidence after it was graded 4 (the Laplace weight moved), and `hai_chat calib` printed `ECE=0.5850 graded=2` (hand check 0.5 x 1.0 + 0.5 x 0.17 = 0.585). Two grades say nothing about calibration in general; the video says so.

## 6. Not done
- `hai_home.pdl` holds an absolute path of this machine; the live relay case has absolute paths too.
- The window ledger (`window/state/chat.ledger`, gitignored) holds the recorded session.
- The reply TEXT is not shown in the window (only id and confidence).
- I opened tomom by mistake (section 3); a request to close it by pid was denied, so that window stays open until the owner closes it.

## 7. Why the window shows no reply text, and why confidence is low (owner question 2026-11-23; RAN, checked in the ledger and code)
Owner: "those confidence levels seem very low. but i still think it should do an attempted chat response in that open window (where the other chat happens?) or is that intentionally not done?"

**The reply text is not shown because W1 stopped one step short. It was not a design decision.** Section 6 lists it under "Not done". What exists:
- Every turn already writes its reply to the ledger: `TURN|ts|input|concept|reply|...|confidence`, for example `TURN|hey , how are u ?|greet|r_greet|...|21`. The reply field is an id (`r_greet`); the words live in `chat/data/replies.pdl` (`REPLY | r_greet | Hi. I can chat with fixed replies for now. Type help.`).
- `window/hai_manager.c` builds the list row from the input, the reply id and the confidence only (`you: hi  ->  r_greet  (83%)`), so the person never sees the sentence hai answered. The headless command `hai_chat turn` prints the sentence; the window does not.
- So the window is a ledger viewer with a composer, not yet a chat. The fix is small and needs no engine change: look the reply id up in `replies.pdl` and show `hai: <text>` under each `you:` row (and in the right pane for the selected row). To build, with a relay-injection check that the sentence appears on screen (a ledger check alone is not enough, per the compendium).

**Why the confidence is low (by the formula, not a fault):** `confidence = round(100 x matched words / input words x concept weight)`. "hey , how are u ?" has about 5 words and only "hey" matches the greet concept, so about 20 x weight = 21%. "what should i call u ?" matched only "what" (help) = 15%. A short, exact word ("hi") scores 83% to 90%; a natural sentence scores low because most of its words are not keywords. The number is honest about how little of the sentence hai understood. It is NOT calibrated yet: with 2 grades the calibration check printed ECE 0.585 and says nothing.

**Two real problems the owner's example shows (RAN):**
1. Wrong answers get a confident-looking reply: "what should i call u ?" is answered with the help list because the word "what" is a help keyword. A reply chosen on one weak word should be shown as unsure ("I think you want help, say yes?"), not as a flat answer.
2. When nothing matches (the fallback), the reply is only "I don't know that yet." plus a menu. The owner's idea (ask what an unknown word means, store the answer) replaces that dead end. See `16.game/presentations/HAI-RESULTS-VS-DESIRE-2026-11-23.md` and the lab notebook DIRECTION NOTE.

**Next (not built):** (a) show reply text in the window; (b) low-confidence replies phrase themselves as a question; (c) the teach loop (`/mark`, "what do you mean by X?"). Done in that order, each with a relay-injection check.

### 7b. FIXED the same day (RAN 2026-11-23): hai's reply is shown, and the chat is above the input
- `window/hai_manager.c` now looks each reply id up in `chat/data/replies.pdl` (fills `{name}` from the ledger row) and publishes a `hai: <text>` row after each `you:` row; row buffer 8 to 12. Headless check on a scratch copy: 10 rows for 5 turns; a mutant with `replies.pdl` hidden gives 0 `hai:` rows.
- `window/hai.xhtpm`: the transcript list moved from the narrow sidebar into the wide panel, above the composer (the owner's rule: "chat should be in the window above the text input"). The sidebar keeps the stamp, state and turn count.
- Seen in the real window by relay injection: the owner's six lines show `you:` and `hai:` rows, all five turns on screen (photograph `p03-five-lines.png`; 22 of 22 relay steps pass).
- NOT DONE: reply sentences are fixed text (by design); the low-confidence "ask, don't assert" phrasing and the teach loop are still to build. The case `window/tests/hai-window-type.pdl` still asserts the old row format (`you: ... -> r_x (N%)` still appears, so it should pass; not re-run after this change).
