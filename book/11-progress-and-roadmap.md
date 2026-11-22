# Progress and roadmap (copy of the XO nov-22 report, 2026-11-22)

The XO copies are the owner's shareable version. This book page is the authority; the demo page links here.

# 🗺️ hai future roadmap, from 2026-11-22

Order is the plan. Each step ends with a harness case that can fail, a mutant that turns it red, and a push. Nothing here is built unless it is in the progress report as ✅.

## Now (next 2 steps)
1. **Haibot object, the party of 6 and the PC box** (book 07, H1). A haibot is a piece with its own ledger; the roster ledger holds the party (max 6) and the PC box. Refusals: a 7th in the party, a withdrawal from an empty PC. Test: replay gives the same roster.
2. **Elements picked at start** (book 08 section 3). Pick one of the ten Terumon elements at creation; the pick is a row; a second pick is refused.

## Then
3. **Faint, rest and healing as rows** (book 08 section 4). A fainted haibot is out of battle until it rests or spends credits. Use the event pages, not new C.
4. **Credits, shop and the test faucet** (book 08 section 5). Fake and free for now. The faucet is refused unless `faucet_enabled` is set; it ships off.
5. **Hangman battle with spells and ELO per haibot** (book 06 section 4, book 07 sections 5 and 6). Heal, harm and reveal as moves; ELO from match rows. Its first version runs on one machine, with no network.

## Then, the viewers
6. **Progress view** (book 06 section 3): levels, stats and badges, computed from the ledger. Shown in the demo page first, then in the window.
7. **X11 window: typed input and the party view.** Verify the typing path (chat-hai uses it), then the party, the PC and the battle log.

## Later
8. **Tool calls** (milestone M4). The chat picks a tool by weight; a harness says whether it worked.
9. **Linked documents** (L1). A linked file becomes rows at once.
10. **Observed models** (candidate rows, capped weight, a person confirms). Armour proposals by a model follow the same bound: the model names, rules set the numbers, a person reviews.
11. **Armour by research and model** (book 08 section 6).
12. **Pairing and two agents over the network** (book 06 section 6). Needs authentication first. The peer code binds to localhost and has no auth today.
13. **The house event format** for haibot pages (book 09 section 4), so the same pages run in the full app.
14. **Installer** (M7). `curl … | sh -s -- hai`, Linux first.

## Gates before anything is called "done"
- A harness case for every rule, with a negative that is refused.
- A mutant that turns the case red.
- JS parity as a harness case, not by hand (today's gap for events and export).
- A live check: a screenshot opened and described for anything on screen.
- Commit only the paths you changed, then push the branch.

## Not decided (owner)
Element names are chosen (the card). Still open: the credit price list, rest turns, whether shared haibots are copied or transferred, the faint rule (back of the party or out of battle), PC capacity (default 5 boxes of 30), and the model for armour.

---

# 📊 hai progress report, 2026-11-22

Status labels: ✅ BUILT (in the repo, with a harness case and an observed result) · 🧪 PARTLY (built in part, or only checked by hand) · 📝 DESIGN (written, not running) · ⏳ NOT STARTED

Repo: `jbro885wgu/harnecient-hai`, branch `hai` (pushed; head `4915ada` and later). Live demo: https://jbro885wgu.github.io/harnecient-hai/

## 🐾 Where are the pets?
Short answer: **the haibot pets are not built yet.** What exists is the parts they will use:
- ✅ the hai agent (chat, learning, a ledger) that a haibot will be made of
- ✅ the ten Terumon elements (data, validated)
- 🧪 the event pages that a haibot will react with (a sample `rest` and `cast`, run in C and JS)
- ✅ export and import of a ledger (the haibot's save file)
- 📝 the party of 6, the PC box, body and armour, hangman battles, ELO per haibot (design docs 06, 07, 08)
- ⏳ no haibot object in the code yet: no party file, no PC, no battle

So a "pet" today is a chat agent with grades. It has no body, no party, no battles. That is the next big gap.

## 🧱 What is built (✅ or 🧪)
| Area | Status | Evidence (harness case, last run) |
|---|---|---|
| Account (local, salted hash, ledger) | ✅ | `hai_account` 17/17 |
| Chat engine: states, concepts, replies, options | ✅ | `hai_chat` 17/17 |
| Grades 0 to 4, Laplace weights, options by weight | ✅ | `hai_learn` 16/16 (weight mutant turns 4 red) |
| CLI (`hai`), numbered menu | ✅ | piped session verified |
| Window W0 (read-only, on the house renderer) | ✅ | opened on the live desktop; captured by window id |
| Ten Terumon elements (data + validator) | ✅ | `hai_elements` 8/8 |
| Event interpreter, fixed row set | ✅ C, 🧪 JS | `hai_event` 12/12 (missing-rule mutant red); JS parity by hand, 4 runs |
| Export and import (.pdl, .csv) | ✅ C, 🧪 JS | `hai_bundle` 10/10 (quoting mutant red); JS parity by hand |
| Demo page (GitHub Pages) | ✅ chat, grades; 🧪 events, export | headless Chrome renders all panels; clicks not run in a real browser |
| CLI menu core (`hai_core`, M1) | ✅ | 4/4 |

## 🌐 The demo page
- Chat in the browser with the same rules as the CLI. Grades 0 to 4 work and change the option order.
- Events panel runs the sample `rest` and `cast` pages (JS, same as the C op).
- Export `.pdl` / `.csv`, and import into an empty ledger.
- A "coming next" section and a limits list, so visitors know what is not there.
- Not yet checked: pressing the buttons in a real browser (only the parts behind them are tested).

## ⚠️ Known problems and debts (honest list)
1. **Clicks not tested in a browser.** The page's event and export buttons run the same code the tests cover, but I haven't pressed them in a real browser.
2. **JS parity by hand.** The event and bundle parity runs are not harness cases yet.
3. **No row checksum.** An edited grade in an exported file would be accepted on import.
4. **Window typing not built.** W0 is read-only. Typing needs the house renderer's input path, which chat-hai already uses; I haven't verified it for hai.
5. **Full-desktop screenshot came back black.** Capturing the window by id works. The cause is not known.
6. **A chat-hai loop stayed running** after a `--dump-and-exit` run. I found and stopped it. House finding: a process should exit with its owner.
7. **A binary was committed by mistake** (`hai_event`), then removed in a follow-up commit. It stays in the history.
8. **A stray `python3 -c 1` call** ran during the window work. It was a no-op, nothing was added, but AGENTS.md forbids any Python call.

## 🧪 Numbers, for the record (observed on 2026-11-22)
Harness cases: `hai_account` 17/17 · `hai_chat` 17/17 · `hai_core` 4/4 · `hai_learn` 16/16 · `hai_elements` 8/8 · `hai_event` 12/12 · `hai_bundle` 10/10. Total 84 checks, 0 failing. Mutants (each turned red as expected): weights ignored, ledger ignored, passphrase check removed, missing rule defaulted to 0, CSV quoting off.

## 📚 Where to read more
- Repo book: `book/00-INDEX.md` (plan, decisions, design docs 04 to 11)
- Design docs: `book/06-viewer-tools-links-watch-battle-p2p.md`, `book/07-haibot-party-pc-elements.md`, `book/08-events-export-element-faint-credits-armour.md`
- Roadmap: `1.future-roadmap-2026-11-22.md` in this folder (the same plan, with the order)
- Terumon design the haibots come from: `XO/30.rpg-dust-work/oct-28/TERUMON-SHAPESHIFT-AND-EVOLUTION-TREES-DESIGN.md`
- The periodic table card (owner's art): the Terumon periodic table, read 2026-11-22.
