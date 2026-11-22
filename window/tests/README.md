# hai-cli window tests (reusable, relay injection)

Same pattern as the other X11-HQ apps (`&.hq-apps/img-hai/tests/`): a case file for the house op `relay_drive`, which types into the REAL window through its input relay, like a person.

| File | What | How to run |
|---|---|---|
| `hai-window-type.pdl` | click the composer by id, type `hi`, Enter, `/grade 4`, `weather`, Enter; checks the LEDGER (`state/chat.ledger`) and the window's own projection (`state/ui.txt`); two shots | needs a FRESH window and an EMPTY `window/state/chat.ledger`: `relay_drive.+x <renderer pid> run window/tests/hai-window-type.pdl <window id>` |
| `../../cases/hai_send.pdl` | the typed-input op alone, headless (13 checks, mutant red 5) | `harness_case_op.+x <abs path of the case>` |

Where the pid and window id come from: pid = `&.hq-apps/hai-cli/state/renderer.pid` (written by the launcher), window id = `relay_drive.+x findwin 784 504`.

Proven 2026-11-22: 16 of 16 on a fresh window; a second run on the SAME window fails line 12 (`turns=1` is gone), which shows the check can fail. The picture was opened and shows the typed rows (`you: hi -> r_greet (83%)`, `graded 4 (r_greet)`).

Not covered: the live case is not a harness case in the house runner (it needs a display and the real window, so it is run by hand with `relay_drive`, like the img-hai tests); the paths inside the case file are absolute (edit if the repo moves); a mid-typing Backspace and a very long line were not tried.
