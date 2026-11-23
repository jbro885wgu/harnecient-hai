# hai (harnecient-hai)

A small, private, source-available chat terminal. It opens from a `hai` command, makes a local account instantly, and chats through a finite state machine with human-readable weights. No language model. You can grade replies (0 to 4) and link documents that the bot learns from at once.

Status: **runs, and learns what you teach it.** It asks about words it does not know, remembers your answers, shows why it answered (the WHY line), and has a dashboard. See `book/` (HAI-IQ-BOOK), start with `book/24-how-hai-answers-the-transparent-pipeline-2026-11-23.md`.

- Try it in a browser (no install): https://jbro885wgu.github.io/harnecient-hai/
- Install (Linux or macOS, needs a C compiler, no sudo): `curl -fsSL https://raw.githubusercontent.com/jbro885wgu/harnecient-hai/hai/install.sh | sh -s -- hai`, then type `hai`. Your chat ledger stays in `~/.local/share/hai-chat`; `sh install.sh uninstall` removes the program.
- Teach it: `/teach word = meaning`, `/alias word = concept`, `/say word = text`, `/mark word`, `/pack lyfedesk`, `/grade 0` to `/grade 4`; see `/bank` (what it knows) and `/why` (why it answered).
- Licence: pending the owner's choice (source-available, read and run for non-commercial use). See `LICENSE` once added.
- Docs: `book/00-INDEX.md`.
