# 24. How hai answers: the transparent pipeline (2026-11-23)

Owner, 2026-11-23: "not a black box, how does it respond. Does it have a bank, is that showable, viewable? Do we have a studio or something to view that kind of stuff?" And: "can use those to dump when creating tutorial presentations, would be very high value."

Labels: **RAN** = built and checked in this session. **DESIGN** = written, not built. **NOT DONE** = a gap, said plainly. Every number below was produced by the code in this repo (`hai_chat why`, `hai_chat bank`) on 2026-11-23.

## 1. What happens to one line you type (RAN, `ops/hai_chat_core.c`, function `hai_chat_turn`)
1. **Split into words:** lower-case letters and digits (`hey , how are u ?` becomes hey, how, are, u: **4** words).
2. **Look for a meaning.** Each built-in concept (`chat/data/concepts.pdl`) counts the words of yours that are its keywords; words you aliased onto it (`ALIAS` rows) count too. The concept with the most hits wins; a tie goes to the higher trust, then to file order.
3. **Taught words.** If a word you taught (`DEFINE`, or an answer you gave when hai asked) is in the line and its hits are at least the best built-in hits, the taught word answers instead (so "so what about the house" is about house, not about "what").
4. **Unknown word.** With no match at all, hai looks for a *content word* (not in `stopwords.pdl`, at least `teach_min_len` letters, not a keyword, alias or taught word; a marked word first, else the longest). It asks what that word means and waits (`await_meaning`). Your next line is stored whole as the meaning.
5. **Confidence.** Built-in: `round(100 x matched words / all your words x trust)`. Taught: `round(100 x hits x (2 or more if marked) / content words x trust)`, capped at 100. **Trust** of a concept (or of `taught:<word>`) is `(reward+1) / (reward+punish+2)`: seeds and `/grade` move it; with no grades it is 0.50 (or the seed, 0.83 for greet).
6. **The state machine** (`chat/data/states.pdl`, rows `NEXT | from | concept | to | action`) decides the next state: `start`, `idle`, `await_name`, `done` come from those rows; `await_meaning` (the ask and the answer) is still set in C, a D92 stopgap listed in book 22 section 5.
7. **The sentence.** A reply id is looked up in the agent's own `replies.pdl` and `{word}`, `{meaning}`, `{name}`, `{say}` are filled. Then **three rows are appended to the ledger:** `TURN` (what you typed, the concept, the reply id, the new state, the action, the typed word, the confidence), `SAID` (the sentence hai said) and `WHY` (a plain-language reason with the numbers). The window and every dump only READ those rows.

## 2. The WHY row (RAN, case `hai_teach` section "transparency")
Examples, produced by `hai_chat why`:
- `matched "hey" as greet: 1 of 4 words x trust 0.83 = 21%`
- `"house" is a content word that is not a keyword, an alias or a taught word, so I ask what it means`
- `I asked what "house" means, so I stored your whole answer as its meaning (certain: you told me)`
- `you taught me "house": 1 hit of 1 content words x trust 0.50 = 50%`
- `no word matched any meaning I know and there is no new content word to ask about, so I offer my top options`
In the window it is the gray `why:` line under each answer (the bottom-bar `why` button hides it).

## 3. The bank (RAN): what it is and how to see it
- **The bank is data plus rows:** the agent's files (`concepts.pdl`, `replies.pdl`, `stopwords.pdl`, `chat_rules.pdl`, `seeds.pdl`, `packs/`; the base is `chat/data/`, another agent has its own copy in `chat/agents/<id>/data/`) and what its ledger has learned (taught words, aliases, answers, marks, grades).
- **In the window:** the bottom-bar `bank` button (or typing `/bank`; `/chat` goes back) shows the Bank of the active agent: built-in meanings in green with their keywords and trust, taught words in gold with their meaning, `[marked]` and trust, aliases and answers. It is the ledger REPLAYED, so it is exactly what the next reply will use.
- **As text:** `hai_chat bank <data dir> <ledger>` prints `CONCEPT|id|keywords|reply|trust`, `TAUGHT|word|meaning|marked|trust`, `ALIAS|word|concept`, `SAY|word|text`; `hai_chat why <ledger> <turn|last>` prints what you said and why hai answered as it did; `hai_chat stats` prints reward, punish and weight per concept and reply; `hai_chat calib` prints calibration against your grades.

## 4. Is there a studio for this? (checked 2026-11-23)
- **Data Studio (`&.hq-apps/data-hq`)** is generic and good at tables: `data_op sniff` reads `chat/data/concepts.pdl` as a 5 by 5 table. It **cannot read hai's ledger**: the ledger mixes row kinds (`TURN`, `SAID`, `WHY`, `DEFINE`...) and is neither a CSV, a headed `.pdl` table nor an `EVENT` ledger (`data_op: not a table`). A hai **exporter** is the right hook: `GAME | export | <program>` in an app's `game.pdl`, writing `turns.csv` (turn, input, concept, reply, confidence, why), `bank.csv` and `weights.csv`, registered in `datasets.pdl` as **private** (a chat is private; the registry fails closed). **DESIGN, not built.**
- **`concept-bank-hq`** is the read-only viewer of the HOUSE concept bank (`&.widgits/concept-bank`), not hai's.
- **What exists for hai today:** the window's Bank and why views, the `hai_chat` dumps, and the web demo `docs/index.html` (an older engine copy without the teach loop).
So the honest answer: there is no separate studio for hai yet; the Bank view is the first one, and Data Studio is the natural second once an exporter exists.

## 5. Dumping real data into a tutorial deck (RAN: how the five 2026-11-23 videos were made)
1. **Get the real data** from the code under test: `hai_chat why <ledger> <n>`, `hai_chat bank <data> <ledger>`, the raw ledger rows of one turn, the first lines of `concepts.pdl` and `replies.pdl`. Nothing is typed by hand.
2. **Photograph the real window** by relay injection (`relay_drive`, `dump_frame_png_op`): `shot` after every `expect` on the window's own `state/ui.txt`.
3. **Turn text dumps into slides** with ffmpeg drawtext: a 1280x640 background, DejaVu Sans Mono, `expansion=none` so `%` prints literally, lines wrapped at spaces. A card header says "(real output)" and names the command.
4. **One manifest row per slide** (`snapshots/NN-name.png | seconds | Japanese subtitle ## English voice`) and `make_presentation_video.py <deck> --theme harnecient --font /usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc --narrator <Actor1.png>` (niced, through `cpu_gate`). **Without `--font` the Japanese strip renders as empty boxes** (found by opening a frame of the first render, which had to be discarded).
5. **Verify before delivering:** `ffprobe` (video and audio streams), `volumedetect` (not silent), and OPEN a frame with Japanese text.
The decks: stakeholder report (results versus desire), tutorial 1 (teach it your words), tutorial 2 (more levers and starter packs), tutorial 3 (agents, sessions, personalities), and this explainer ("how hai answers"). Home: the owner's XO folder `%.HARNECIENT/hai-user-dox/hai-videos-2026-11-23/` and `31.Nov/nov-23/5.hai-videos-2026-11-23`.

## 6. Window and test traps found today (RAN; each cost time)
**Layout (khtpm renderer):**
- The renderer accepts ONE `<sidebar>`. A second one never rendered. For three columns use a **flex page** (`class` with `display:flex; flex-direction:row`, the `canvas-craft` pattern): the sidebar plus N panels sized by `width` and `flex-grow`.
- **The `<sidebar>` element must never be removed** (`show=` false on it blanked the whole window: the layout trigger needs a sidebar and a panel). Minimize it by narrowing it (a class from data: 200px open, 40px with a `[+]` restore).
- **Chrome (2026-11-23, owner: make it look like rpg-edit/Teario):** the window now copies rpg-space: a `tabbar` (`tab rp-tb`, tabs Sessions/Agents/Bank/Why, `hai-on`/`hai-off` classes), panel titles `ov-title` with an `ov-min` `_` button, `rp-menu-row` buttons, and a `pchq-footer` with `bars` and one `pchq-ft` button per agent. The tabbar must sit INSIDE the flex page before the sidebar, with `width: 0px`, or it reserves an empty column (`100%` was read as 100px).
- rpg-edit and rpg-space get their side bars from **overlay rows on a `<canvas>`** (`canvas-overlay-left/right` with `ov-pin`, a `_` minimize, footer toggles). Overlays need that canvas, and the canvas takes the middle, hiding text. For a text window use the flex page instead.
- A `repeat` variable prefix is its `bind` name (`bind="bk"` reads `bk_0_text`, not `bank_0_text`).
- The shared footer class clips labels to a letter: give the on/off classes an explicit width.
**Driving (`relay_drive`):**
- `tapid` mis-resolves elements in the flex page's panels (the composer, the right column): it clicked x=295 for an item drawn at x=680. Use `tap x y` (window coordinates) there; sidebar items and the footer resolve fine.
- The first click after a sidebar action (or on an unfocused window) only focuses: tap the composer twice before typing, or the typed line goes nowhere and Enter re-presses the focused item.
- A relay `tapid` on a sidebar item fired its action **twice per click, intermittently** (a toggle went 1, 0, 1 in a 50 ms watch of `ui_prefs.txt`; seen with `NEW` too). I did NOT test a real mouse click. `hai_session` now ignores an identical request repeated within 600 ms when the renderer calls it (argc 4+; cases are never debounced). No case covers the debounce. `hai_session NEW` used the same `session_<epoch>` name and truncated the ledger it was inheriting from: sessions are never overwritten now, and `NEW` on an empty session is a no-op.
**Harness (`harness_case_op`):**
- **`EXPECT_FILE` takes FOUR fields (path | exact text | label); a three-field row is silently ignored** (no PASS, no FAIL). Existence is `EXPECT_EXISTS`. Six of my checks were no-ops until I compared the number of check rows with the number of PASS lines. Do that comparison on every new case.
- A bare `|` in an `EXPECT_OUT` / `EXPECT_HAS` text must be written `\p`.
- A negative check on a wrong path passes vacuously (`EXPECT_NOFILE` of a path that is never written): always break the code once and see the check go red.
**Shell:** `pkill -f <pattern>` kills your own shell when the pattern is in your command line; `gcc ... | head` can stop the compile before it links (the old binary then runs).

## 7. Not done (said plainly)
- No Data Studio exporter for hai (section 4); no hai-specific chart (trust over time, confidence calibration) yet.
- The Bank shows the active agent only; there is no side-by-side comparison of two agents.
- Inherited rows in a new session still show as `you: /teach ...` (their origin is in the row, the window does not distinguish it).
- Lever confirmations are fixed text, not in the agent's voice.
- (Closed later the same day, see section 8: the web demo was ported and a case keeps C and JS identical.)
- The Japanese subtitles and narration lines are mine and have not been reviewed by a native speaker.

## 8. Later the same day (2026-11-23): web port, installer, dashboard (RAN unless marked)
**The web demo now has the teach loop.** `docs/engine.js` was rewritten as a faithful port of `ops/hai_chat_core.c`: the replay (DEFINE, ALIAS, SAY, FORGET, MARK, PACK, ask and answer), the ask-about-an-unknown-word branch, taught words, marks, the WHY and SAID rows, the levers, `bank`, `why`, `viz`. It is also a command line with the same verbs as `hai_chat` (`node docs/engine.js turn <data> <ledger> <input>`, `HAI_NOW` fixes the clock). **Case `hai_web_parity` (449 rows)** runs the C and the JS engine on the same 32-step conversation and on the dumps (why for 21 turns, bank, viz, stats, calib, state) and requires the identical literal text and exit code from both; mutants in either engine turn it red (taught-confidence, WHY text, forget order, viz counts, the C min-length rule). **Trap found by the parity run:** JavaScript `toFixed` rounds an exact tie up (0.125 gives 0.13), C `printf` rounds it to even (0.12). `fmt()` in the engine follows C, checked against the system printf in the same case.
**The page** (`docs/index.html`): the feed is drawn from the ledger alone (TURN, SAID, WHY, GRADE rows; the old second `localStorage` array of reply text is gone), the levers and the starter pack are typed or one button, and there is a dashboard (below). First-visit and pre-filled-ledger paths were loaded in headless Chrome and the DOM and screenshots read. Hover tooltips and the table switch were NOT exercised by a script (only the code path that draws them ran).
**The installer** (`install.sh`): builds `hai` and `hai_chat` from source into `~/.local/share/hai`, writes `~/.local/bin/hai` (plus `hai bank`, `hai why`, `hai where`), keeps the user ledger in `~/.local/share/hai-chat` (an update never touches it), `sh install.sh uninstall`. `HAI_SRC=<checkout>` installs from a local folder (used by the case). **Case `hai_install` (23 rows)**: install, the command works, an update keeps what the user taught, refusals, uninstall; the mutant that wipes the ledger on update turns it red. `ops/hai.c` (the command-line chat) now has the same levers as the window (`/teach /alias /say /forget /pack /mark /grade /bank /why`) and prints the WHY line under every reply. **Not run:** the real download path was run once by hand into a scratch HOME and it works mechanically, but the public `hai` branch on GitHub is 13+ commits behind these changes, so **a public install today gives the OLD chat (no /teach) until the owner pushes the branch** (pushing is the owner decision).
**The dashboard.** One data verb, `hai_chat viz <data> <ledger>` (C) and `node docs/engine.js viz ...` (JS), identical output: `CONF|turn|confidence|concept`, `KNOW|turn|taught|aliases|says|marked`, `TRUST|id|weight|reward|punish`, `CAL|lo|hi|n|good|rate|mean_conf`. **Window** (tab Viz, `/viz`): text-bar charts (the house precedent is csv-lab-hq: the khtpm window has no painted charts), blue and orange marks, ink labels. **Web:** four SVG charts (confidence per turn, knowledge over time, trust per meaning, calibration) with a legend on the two-series chart, 24 px hover targets, per-mark tooltips (textContent only), a table view, a switch to tables. Colors follow the dataviz skill: dark-surface blue #3987e5 and orange #d95926 validated with its script (CVD dE 26.8, contrast over 3:1, all checks pass). `replay()` in C was split into `replay_line()` to read the state after each turn (all cases stayed green).
**Window traps found today:** `scrolllist` opens scrolled to the end unless it has `class="from-top"` (the Viz list needs it); khtpm text rows wrap at about 52 characters in the centre column, so a 20-cell bar wrapped (12 cells fit); a click on a sidebar item can fire twice (debounced in `hai_session`, see section 6).
**Not done:** no pet binding; no Data Studio exporter (the viz lines are the natural source for one); tooltips and table switch untested by script; the Japanese lines in the videos are not reviewed by a native speaker; the desktop Viz view has no calibration chart legend beyond the two labelled rows.
