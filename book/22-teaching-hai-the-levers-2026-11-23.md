# 22. Teaching hai: the levers (2026-11-23)

Owner, 2026-11-23: "there should ideally be different ways of teaching different levers and nobs to pull ... that's the point of this whole thing. Get crazy with it. Let's bootstrap it and make it easy for new users to do the same."

Labels: **RAN** = built and checked in this session. **NOT DONE** = a gap, said plainly. Everything a lever does is a row in the chat ledger (`window/state/chat.ledger`); state is rebuilt by replay, nothing is held in memory, and a file beside the ledger teaches it nothing.

## 1. The levers (RAN; case `cases/hai_teach.pdl`, 42 checks, 8 mutants turn it red)
| Lever | You type | What it does | Ledger row |
|---|---|---|---|
| **Ask** (automatic) | any sentence with a word hai does not know | hai stops and asks "What does it mean to you?"; your next line is the meaning (`skip` passes) | `TURN ... ask_word`, then `TURN ... store_meaning` |
| **Teach** | `/teach word = meaning` | teach a meaning without waiting to be asked | `DEFINE|ts|word|meaning|you` |
| **Alias** | `/alias word = concept` | a new word triggers a behavior hai already has (`/alias yo = greet`); concepts: greet, help, account, nav, bye | `ALIAS|ts|word|concept|you` |
| **Say** | `/say word = text` | hai answers with YOUR sentence when the word comes up | `SAY|ts|word|text|you` |
| **Mark** | `/mark word` (or `/mark word normal`) | a marked word is asked about first and counts double when recognised (confidence is capped at 100) | `MARK|ts|word|high` |
| **Forget** | `/forget word` | removes the meaning, the answer, the alias and the mark | `FORGET|ts|word` |
| **Pack** | `/pack lyfedesk` | loads a starter pack (a text file of DEFINE / ALIAS / SAY rows) into YOUR ledger in one command | `PACK|ts|name`, then each row with origin `pack:name` |
| **Grade** | `/grade 0..4` | the existing lever: moves the weight of the last reply's concept (a taught word has its own concept `taught:<word>`) | `GRADE|...` |

Every turn and every lever also writes `SAID|ts|turn|text`: what hai actually said (a recorded fact), so the window shows the transcript without re-deriving it.

## 2. How confidence works for taught words (RAN, worked numbers in the case)
`confidence = round(100 x taught-word hits x (2 if marked) / content words x weight of taught:<word>)`, capped at 100. Content words are the words not in `chat/data/stopwords.pdl`. "do u like the house" has one content word (house), the weight starts at 0.5, so **50%**; after `/grade 4` the weight is 5/6, so **83%**; after `/mark house` the hit counts double, so **100%**. The built-in concepts keep their old formula (matched words / all words x weight), so "hi" is still 83%.

## 3. Make your own pack (RAN)
A pack is a text file `chat/data/packs/<name>.pdl`:
```
DEFINE | word | what it means in your own words
ALIAS  | word | greet        (a built-in concept id)
SAY    | word | the exact answer you want
```
`/pack <name>` appends one row per line to your ledger with origin `pack:<name>`, so you can still grade, mark and forget them. The shipped pack `lyfedesk` has 9 definitions (house, driving, confidence, harness, ledger, hai, harnecient, entity, attention) and 4 aliases (yo, howdy, options, commands). A pack is data: add one, never edit C.

## 4. Tutorials (RAN: recorded in the real window by relay injection)
Two narrated videos (English voice, Japanese subtitles, narrator face), in the owner's XO folder `%.HARNECIENT/hai-user-dox/tutorials/` and `31.Nov/nov-22/14.*`, `15.*`: Tutorial 1 "teach hai your words" (ask, explain, recall, grade, mark) and Tutorial 2 "more levers and starter packs" (alias, say, forget, pack). A stakeholder report with the before and after is `13.*`. House text: `16.game/presentations/HAI-RESULTS-VS-DESIRE-2026-11-23.md`.

## 5. What is NOT done (said plainly)
- **Inflections are not recognised:** a taught word does not cover its plural or other forms (measured: 1 of 8 inflected sentences recognised; "houses", "driven", "harnesses" all get asked about again). A candidate fix, not built: when an unknown word shares a stem with a taught word, ask "is this a form of <word>?" and store an alias.
- **The web engine (`docs/engine.js`) has no teach loop**, so C and JS are no longer in parity (the case `hai_chat` parity of 29 lines still passes for the old behaviour, but the new levers are C only).
- **Teach logic is in C, not in event pages / NEXT rows** (house rule D92 wants rules as data). Stopgap, label: the ask / store / recognise steps in `hai_chat_core.c`. Removal condition: when the FSM `NEXT` table can carry the `ask_word` and `store_meaning` actions as rows.
- **Low-confidence replies are still asserted, not asked:** "what should i call u ?" is answered with the help list on the single word "what". Next lever: a reply chosen on one weak word should be phrased as a question.
- **`/act word = action` (tool calls) is not built.** The plan: map a word to an allow-listed action id from a data file, and PROPOSE the command (never run it from the chat). This is the link to the work router (20 routes) that saves an outside agent's tokens.
- **Recognition is exact whole-word match** (the same lexical wall as the router, lab H8); the sentences in lab H12 were written by me, not by a user.
- The window row buffer shows the last 14 rows; there is no scrolling by key yet.
- The photographs show a 784x504 window; long meanings wrap.

## 6. Design-choice test (house rule), answered
One authority per fact: yes, the ledger. Modular: levers are rows + one core function (`hai_chat_cmd`), the window only calls it. Scalable: the 10th lever is a new row type and a verb. Long term: rows can move to NEXT / event pages. House shapes: `.pdl` data (packs, stopwords, rules), a harness that can fail (`hai_teach`), a capped runtime file (the ledger has the daily grade cap; NOT DONE: a size cap on the ledger itself, listed under RUNTIME-FILE-GROWTH-STANDARD debt for the hai window ledger). Cost: a few hundred lines of C, no new process.
