# 25. Embodied agents: a world, sessions as books, permissions (DESIGN, 2026-11-23)

**Everything here is DESIGN. Nothing in this chapter is built.** Labels: **BUILT** = exists and was run. **DESIGN** = written here, not built. **OPEN** = needs the owner. The BUILT parts it stands on are listed in section 1.

Owner's words (2026-11-23), kept because the build must match them:
> "we want hai-cli to be an actual embodied agent doing chat. creating a new agent (haibot) creates a new embodied agent on the map ... the agent can do tool calls on the map, create / farm etc. and the user can see the agent learning in the demo and in the install."
> "sessions hold many agents, we can switch agents, but agents aren't on another page unless they teleport." "real folders, but on the JS it's just a data file + visual ... they should be able to download the file and reopen their session."
> "agents will have settings and we can make it so an agent can or can't do certain things, hard biases, and hand-tuned biases ... more permissions as it learns."
> "head is like viz: it would open the board for hai-cli attached ... later hai-pal, or connect hai-cli to an existing 2D/3D board by pairing it so it can control the entities / board / books / pages."
> "hai-cli itself can be chatted with without being an agent ... a general default agent, an entity but not embodied, that keeps its own global application-level memory ... any entity in the first slot is treated as this main agent, like the first pokemon in a party."
> Later goals: agents edit blueprintz, create and edit events, make whole games, or be NPCs inside a game.

## 1. What exists to stand on (BUILT, checked 2026-11-23)
- The chat engine in C (`ops/hai_chat_core.c`) and JS (`docs/engine.js`), kept identical by case `hai_web_parity`; every fact is a ledger row, replayed on demand.
- Agents as folders (own corpus, persona, ledgers), sessions as ledgers inside an agent (`window/hai_session.c`), a window with a tab bar, Sessions and Agents columns, a Viz tab with a per-item drill-down.
- A ledger export and import for the web demo (`docs/bundle.js`); an installer (`install.sh`).
- The house: a shared board engine (rpg-space, Teario), books and pages (rpg-edit, blueprintz), an op registry. **Not yet connected to hai.**
- The haibot design (`book/07`): party of 6, a body, elements. **Never built.**

## 2. The model (DESIGN)
| Thing | What it is |
|---|---|
| **Session** | the whole saved world: one folder on the desktop, one downloadable file on the web. It holds many agents. |
| **Book** | the session's structure. For the prototype: one book, `hai-cli`, inside the session. |
| **Page** | one map in the book (a room, a field). A page is what `session` means in the window today: the old per-agent sessions go away. |
| **Agent (pal)** | an entity in the session with a ledger. It is on exactly one page at a time. It changes page only by a `teleport` verb. You switch which agent you talk to. |
| **hai-cli (main agent)** | the entity in slot 1 of the party. It is NOT embodied (no body on a map). It can be chatted with, can be renamed, and keeps the global memory: the list of all sub-agents, and rows about what it watched you do with them. Any entity put in slot 1 becomes the main agent. |
| **Party order** | slot 1 is main. Reordering is HELD (section 8); until then a plain command sets the main agent. |

## 3. Actions are verbs, and verbs are ledger rows (DESIGN)
An agent action is one row, written through one op per verb (house rule: every rule is an op, never hard-coded in an engine): `SPAWN`, `MOVE x y`, `MKDIR name`, `MKDOC name text`, `READ name`, `WRITE name text`, `TELEPORT page x y`, later `FIGHT`, `FARM`, `BLUEPRINTZ_EDIT`, `EVENT_NEW`. The world is what you get by replaying the rows (no ghost data: no state file that the ledger does not explain). The desktop applies a row to real folders; the web applies the same row to a data object and draws it. **The two must agree: the same parity case idea as `hai_web_parity` (same rows in, same world and same text out).**
"Tool call" in this design means exactly this: an agent chooses a verb and its arguments, a permission check allows or refuses it, a row is written. The agent learns the phrase that triggers a verb through the same teach loop as a word (ask, teach, alias).

## 4. Permissions and biases (DESIGN)
Each agent has a settings file (data, not code):
- **Hard rules** (`DENY verb` / `ALLOW verb`): an agent without `ALLOW MKDOC` cannot, whatever it has learned. Checked before a row is written; a refusal is itself a row (`REFUSED verb reason`) so it can be audited and shown.
- **Tunable biases** (`BIAS verb weight`): how readily an agent picks a verb when several fit; hand-set by the user.
- **Permission growth:** a rule row can say "ALLOW verb once trust of verb is at least 0.80" (trust is the same weight the grades already move). So an agent earns permissions by being graded good, and the user can see why it was or was not allowed (the WHY line).
- Default for a new agent: the smallest set (`MOVE`, `READ`). Everything else is off until granted.

## 5. Real folders and the sandbox (DESIGN, the safety-critical part)
On the desktop an agent's folders and documents are real directories and files **inside one sandbox root per session**. Rules, each with a case that can fail:
1. Every path an agent gives is resolved against the sandbox root; `..`, absolute paths, symlinks that leave the root, and names with `/` or NUL are refused (a refusal row).
2. The sandbox root is never a user folder (`xyzfs/users` and anything outside the session folder are refused by name).
3. Size caps: bytes per document, documents per session, depth (a runtime cap, registered in the runtime-file standard).
4. The escape attempts are the first cases written, before any verb works.
On the web there are no real folders: the session file holds the same rows and the document texts, nothing else, and the page draws them. **Download = the session file; reopening it = replaying it.**

## 6. Windows (DESIGN)
- **Prototype:** a second X11-HQ window attached under or over hai-cli, opened by a new top-bar tab `head` (like `Viz`, a toggle). It shows the agents of the current page on the shared board (2D first, 3D later, reusing the board engine) and has a chat line to the agent you picked. Standalone `hai-cli` (from under H-AI) works without it: a CLI-only user keeps the current chat.
- **Later:** an rpg-edit book `cli-pal` (event based): board in the centre instead of the chat, chat on the right, pages on the left. The same session, opened a second way.
- **Later:** pairing hai-cli with an existing 2D or 3D board so it can control entities, books and pages there (the same verbs, a different world behind them, same permission checks).
- **Proof for both entry points (owner: "doc, test and prove both"):** one session file must open in the standalone window and in the rpg-edit book and give the same world; a case replays one ledger through both and compares.

## 7. The main agent (DESIGN)
Slot 1 of the party is the main agent. It has no body. Its memory is rows too: `MAIN|ts|agent|event` (agent created, renamed, switched to, a grade given to it, a verb it was refused). It can be asked "what agents do I have" and "what did I do with prof" and answer from those rows. It can watch the user interact with sub-agents (rows, not guesses). Renaming it renames an entity; putting another entity in slot 1 swaps which one keeps the global memory (the memory rows follow the slot, not the name: OPEN, decide when building).

## 8. Reordering the party (HELD, decision recorded)
Owner's idea: type `^` or `v` on a row to move it; the `>` marker changes. **Conflict:** `^` already means the "In" input mode in the shared nav (badges like `[^]10.`). **Options:** (a) rename the In marker (to `#`): touches every window's badge, the shared renderer, many cases and docs for a cosmetic gain; (b) a separate move mode: `m` on the focused row switches its marker to `↕`, then `^` and `v` (or arrow keys) move it, Enter or Esc leaves; `^` keeps its one meaning. **Recommendation: (b)**, because one key keeps one meaning in each mode and it works for any list. **Held**: not built now; the main agent is "slot 1" and a command sets it. A design-choice-log entry is OWED in the house `DESIGN-CHOICE-TEST-AND-DECISION-LOG.md` (that file has other agents' uncommitted changes; add the row when it is clean).

## 9. Build order (every step has a case that can fail; nothing is claimed before it runs)
1. **Sandbox first:** path resolution and escape refusals, with the attack cases written first (C op, harness case).
2. **World verbs** (`SPAWN MOVE MKDIR MKDOC READ WRITE TELEPORT`) in C and JS, same rows, same text; a parity case; permissions (hard rules, then biases).
3. **Session file:** one session, many agents, pages; export and import on the web; the desktop folder and the web file replay to the same world (parity case).
4. **Main agent** and the global memory rows; the `/agent main <id>` command.
5. **The `head` window:** the attached board (2D), agents on a page, a chat line to one agent; photographed and driven like the other windows.
6. **Commanded actions learned through the teach loop** ("make a folder called notes" becomes the verb, the phrase is learned and graded).
7. **The rpg-edit book `cli-pal`**, and the proof that both entry points open the same session.
8. **Then:** blueprintz tool verbs, event verbs, NPC use, pairing with an existing board, self-directed goals (agents acting without a command).
Reordering (section 8) is added after step 5.

## 10. Open and not decided
- **OPEN:** whether the main agent's memory follows the slot or the entity (section 7).
- **OPEN:** whether permissions can ever be granted by the agent itself (default: never; only the user or a trust rule the user wrote).
- **NOT BUILT:** all of this. **NOT CHECKED:** how the board engine takes entities from a ledger (needs a read of the board input before step 5).
- **RISK:** real folders. Treated as the first thing to build and test (step 1); no verb that writes to disk ships before the escape cases pass.

## 11. Opening it in rpg-edit (pointer, 2026-11-23)
The research and the recipe for opening this as a BOOK in rpg-edit (book = session, page = map, agent = pal row, rules as ops behind event pages, Play runs the driver) are in the house doc `16.game/OPEN-A-PROJECT-AS-A-BOOK-IN-RPG-EDIT-EVENT-BASED-PAGES-AS-SESSIONS-2026-11-23.md`. Built so far on that road: `ops/hai_world.c` (the world rules, case `hai_world`, 43 checks). Not built: the book, event pages, the driver, permissions, the sandbox.

## 12. Better charts when hai runs inside the rpg-edit wrapper (owner 2026-11-23; DESIGN)
Inside the rpg-edit book there is a canvas (the board in the centre), so the Viz tab can draw real charts into a canvas frame instead of the text bars the plain window uses today (the standalone window cannot draw graphics; the web page already draws SVG charts). The data does not change: the same `hai_chat viz` and `detail` lines (CONF, KNOW, TRUST, CAL, ITEM, HIST ...) feed whichever drawer is present, a text-bar drawer, an SVG drawer, a canvas drawer; one data source, three projections (the house 2D/3D-parity rule). For the canvas drawer the frame format is the one `hai_world` already writes (RGBA plus a receipt); a chart op would write the same kind of frame. A chart view would replace the board while the Viz tab is open (the owner's earlier ask: a view instead of chat, a click-through to the word). **NOT BUILT.** Check when built: the canvas chart and the SVG chart must show the same numbers for the same ledger (a parity case on the data lines, plus a frame opened by eye).
