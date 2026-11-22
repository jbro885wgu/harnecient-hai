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
