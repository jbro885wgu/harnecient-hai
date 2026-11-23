# 23. Agents, sessions and personalities (2026-11-23)

Owner, 2026-11-23: "maybe we could also have different pets / agents like the rpg-space has, where we can add different agents with different sessions, corpuses, whatever personalities. That's the end goal. Hook it up and doc it." And: "other hai chat uses the left sidebar for sessions".

Labels: **RAN** = built and checked in this session. **DESIGN** = written, not built. **NOT DONE** = a gap, said plainly.

## 1. The model (RAN)
- **An agent is a folder.** `chat/agents/<id>/data/` holds its own copy of the corpus and voice: `concepts.pdl`, `replies.pdl`, `chat_rules.pdl`, `seeds.pdl`, `states.pdl`, `stopwords.pdl`, `packs/`. `chat/agents/<id>/agent.pdl` is its card (`AGENT | id | persona | created`). The original agent `hai` keeps the old paths (`chat/data`, `window/state/chat.ledger`), so nothing older broke.
- **An agent has sessions.** Each session is its own ledger: `window/state/agents/<id>/chat.ledger` (the first, called `main`) and `window/state/agents/<id>/sessions/<name>.ledger`. A ledger is the agent's whole mind (what it was taught, graded and said).
- **A personality is a data preset** in `chat/personas/<name>.pdl`, applied ONCE at birth: `RULE | key | value` rows replace numbers in the agent's `chat_rules.pdl`, `REPLY | id | text` rows replace its answers in `replies.pdl`, `PACK | name` rows load a starter pack into its first session. After birth the agent's files are plain data to edit. Shipped: `curious` (asks about everything, marks count triple, enthusiastic voice), `terse` (one option, long words only, "Hi."), `scholar` (born knowing the LyfeDesk glossary, formal voice, "my notes say").
- **Two pointers, both preferences** (they only choose what the window shows and writes, never what an agent knows): `window/state/active_agent.txt` and `<agent root>/sessions/active.txt`. A file beside the ledger teaches an agent nothing.

## 2. How to use it (RAN, in the real window; case `cases/hai_agents.pdl`, 33 checks, mutants red)
| You do | What happens |
|---|---|
| `/agent new prof scholar` | a folder `chat/agents/prof/` is made from the base data plus the `scholar` preset; the pack is loaded; `prof` becomes the active agent |
| `/agent hai` or click an agent in the sidebar | switches the active agent; the chat shows ITS session |
| `/agent delete prof` | removes that agent's folder and sessions (never `hai`); the active agent falls back to `hai` |
| `+ New Session` (click) or `/session new` | a new session with an empty transcript that **inherits what you taught** (DEFINE, ALIAS, SAY, FORGET, MARK rows, and the answers you gave to hai's questions); grades and turns are not inherited. If the active session has no turns yet, it is reused (see section 4) |
| click a session / `/session <name>` | switches; backspace on a session asks to delete it; `main` is never deleted from the window |
| sidebar | Agents (the active one highlighted), Sessions of that agent (labelled by the first thing you typed in each, like a chat list) |
| chat colours | you in blue (like the typing box), hai in green, levers and grades muted (`hai.css`: `.chat-you`, `.chat-hai`, `.chat-note`) |

The sidebar follows the shape `chat-hai` already uses (Sessions, "+ New Session", one item per session, `state/sessions/active`): the house had this pattern, so hai-cli reuses it in C (`window/hai_session.c`), not in a shell script.

## 3. Evidence (RAN)
- Case `hai_agents` (33): birth, persona rule and voice applied, the original agent untouched, a duplicate / unknown persona / reserved name / path in a name refused, a failed birth leaves nothing behind, voice and ledger isolation between agents, the scholar born with 9 definitions, a new session inherits, `main` cannot be deleted, the typed commands call the same op, deleting the active agent falls back, and the quick-click case. Mutants that each turn it red: persona rules not applied, persona validated after the copy, session inherits nothing, `main` deletable, `hai_send` ignoring the active agent's data (15 checks fail), empty-session reuse removed, and the old truncation bug.
- All other hai cases unchanged: `hai_teach` 42, `hai_send` 13, `hai_chat` 17, `hai_learn` 16, `hai_confidence` 11, `hai_calibration` 6, `hai_bundle` 10, `hai_account` 17, `hai_core` 4, `hai_tier` 10, `hai_event` 12, `hai_elements` 8.
- The real window, driven by relay injection and photographed: a curious agent greeting in its own voice and asking "Ooh, house!", a scholar answering "my notes say", the original `hai` still asking the plain question, and a new session of the curious agent recalling "house" in the curious voice (44 relay steps pass). Photographs are in the stakeholder deck and tutorial 3.

## 4. Bugs the real window found that the headless tests did not (RAN)
A relay click on "+ New Session" fires the action twice within one second. Both runs picked the same name `session_<epoch>`, and the second run's `inherit()` opened its target with `fopen(dst, "w")`, which truncated the ledger it was also reading from: the new session lost everything it was meant to inherit. Fixes: a session file is never overwritten (a suffix `_2`, `_3` is added), and `NEW` on an active session that has no turns is a no-op. Also: after clicking a sidebar item the composer loses keyboard focus, so a recording script must `tapid composer` before typing (otherwise Enter re-presses the focused item). Also: two scroll lists in one sidebar did not both render, so the agents are plain rows and the sessions are the one scroll list.

## 5. Hooking agents to the house pets (DESIGN, not built)
The house already has pets (`@.apps/pets`, `pet-trainer`, the rpg-space board) that are entities with a uid, a wallet, a curriculum and evolution. The intended link, in order:
1. **Bind by uid.** Add `ENTITY | <uid>` to `chat/agents/<id>/agent.pdl`; an entity's chat window opens the agent bound to it. One authority per fact: the entity owns its stats, the agent owns what it was taught; neither copies the other.
2. **A pet's learning feeds its agent.** Entity "knowledge domains" (see the house's entity-learner plan) become packs: a pet that passes an exam gets a DEFINE row in its agent's ledger through the same `hai_chat_cmd`, so teaching a pet and teaching its chat are one act.
3. **The agent's replies stay fixed text chosen by weights** (the house rule: a model describes, never sets a number); an optional LAN model (the Mac's Ollama via `ai_backend.pdl`) may only phrase a line, never change a weight.
4. **Agents talking to each other** reuse the hai phones and the router (`phone_send_op`, `server_route_op`), see `08-roadmap/design-docs/HAI-ROBOTS-PHONES-SERVER-DESIGN.md`. Blocked, as before, by authentication.
None of this is built. The first step is item 1, one `ENTITY` row and a launcher, with a relay-injection test.

## 6. Not done (said plainly)
- **No entity binding yet** (section 5).
- **Knowledge is per session and per agent**, not shared. A new session inherits a snapshot at the moment you press New; teaching in one session later does not reach its siblings. A shared "knowledge" ledger separate from transcripts is the cleaner design (it would let sessions be pure transcripts); not built because it changes how the engine replays.
- **Lever confirmations ("OK. When you mention house I will say...") use fixed text, not the agent's voice**; only the ask and recall replies are persona-voiced.
- **Inherited rows show as `you: /teach ...` in a new session** though the person did not type them there (their origin is recorded in the row, the window does not distinguish yet).
- **Inflections** (houses, driven) are still not recognised (book 22).
- **The web engine has no agents or teach loop.**
- The personas are three small presets written by me; a person's own wording is the real test. Persona effect is limited to numbers and reply text, not behavior.
- Runtime files: ledgers and sessions have no size cap yet (RUNTIME-FILE-GROWTH-STANDARD debt); the shipped `chat/agents/` is empty and agent folders are gitignored.

## 7. Design-choice test, answered
One authority per fact: the ledger (state) and the agent folder (corpus and voice). Modular: the window calls one op (`hai_session`) and one sender; the helpers are text-included (`hai_active.h`), never a header plus a link. Scalable: the tenth agent is a folder and a persona file, no C. Long term: the entity binding is one row. House shapes: `.pdl` data, a harness that can fail, files not processes, no shell. Cost: about 250 lines of C, no new process.
