# haibot: shared events, export and reload, element choice, faint and healing, credits, armour by research (design, 2026-11-22)

Labels: **BUILT** · **HOUSE** (in the house, checked by reading) · **DESIGN** · **OWNER** (said 2026-11-22, quoted) · **OPEN** (not decided).

## 1. Events in C and in JS, one set of pages (OWNER: "it should use events as well even in js")
- The house rule: what happens is an **event page** (`events/<name>/event_pkg/pages/page_1/{condition.pdl, event.ir.pdl}`), and the work is done by an op (HOUSE, `02-architecture/EVENT-ARCHITECTURE-ONBOARDING-2026-11-05.md`, layer 2).
- **Design**: the haibot's rules (spell use, damage, heal, faint, buy) are event pages. The C engine runs them through the ops. The JS page runs **the same pages** through a small interpreter for the same subset of rows. The C result and the JS result must match, as the chat parity check does now (M5).
- **Where the JS interpreter stops**: any row the JS does not know is refused loudly, never guessed. The parity check lists the row kinds both sides support.
- Ops keep their numbers in RULE rows, so the page and both engines read the same numbers.

## 2. Export and reload: CSV or PDL (OWNER: "save as csv or .pdl, then reload in c after they download")
- A haibot, its ledger, its events and its spells can be **exported** as one of:
  - `.pdl` (rows, the house format; the reference), or
  - `.csv` (a flat table for spreadsheets; one section per row kind, with a `kind` column).
- The export is a **projection of the ledger**: it contains rows, not state. Import = append those rows to a new ledger (through the same verbs), so a reloaded haibot replays to the same state.
- **Round trip test**: export in JS, download, reload in C, export again in C, and the two files match (timestamps masked). Also the other way round.
- A file that fails the row checks is refused and names the bad row. An imported file cannot set a stat directly; it can only add rows, which the ledger then replays (house rule: no ghost data).
- OPEN: whether a shared file may carry another person's haibot (owner's decision; default: a copy, never a transfer).

## 3. Element chosen at start, from ten (OWNER: "let them pick element at start like pokemon ... from the 10; later is fine")
- Ten elements, as data (`elements.pdl`). The player picks **one** at creation. Creation writes one `ELEMENT|pick|pid|element|unix` row.
- The choice sets the first spell line (it unlocks first). The other nine are learned by grades in their words, as in the spell design (book 07 section 5).
- **Ten names**: not yet chosen. The first version can use placeholders (`el_01` … `el_10`) and the names come later, as data.
- Build: later (owner: "later is fine"). Until then, creation picks element 1 by default, and that is labelled as a default in the viewer.

## 4. Faint and healing (OWNER: "faint, out of battle. not battleable till healed. resting heals or ... buy / use credits")
- **Faint** at 0 HP. A fainted haibot is **out of battle**: it cannot be picked, its ELO is kept, and it writes one `FAINT|pid|match|unix` row.
- **Unbattleable** until healed. Two ways, both as rows:
  1. **Rest**: `REST|pid|turns` rows. After a RULE number of rest turns (`rest_turns_to_heal`) it is healed (`HEAL|pid|by=rest|unix`).
  2. **Buy or use credits** (section 5): a `HEAL|pid|by=credits|cost|unix` row, with the price in a RULE row.
- A healed haibot returns with a RULE percentage of HP (`heal_percent`), not full, unless the price says full.

## 5. Credits: fake and free for now, with a top-off faucet for testing (OWNER: "lets give fake free credits for now ... top off faucet 4 fun testing")
- **Credits** are a **fake currency**: a ledger of `CREDIT|add|user|amount|reason|unix` and `CREDIT|spend|user|amount|reason|unix` rows. Balance is replayed, never stored.
- **Fake and free for now** means: no payment, no real money, no chain. A short notice on the page and in the viewer says so. The house's chain and coin rules are **not** used here (book 06 and the pet chain docs are separate).
- **Top-off faucet** (testing only): a verb `faucet` that adds a fixed RULE amount (`faucet_amount`) and is **refused** unless `faucet_enabled = 1` in the rules, which ships as `0` and is set to `1` only in a test house. It is labelled "test faucet" in the viewer. Owner's words: "just 4 fun testing 4 now".
- Shop: `SHOP | item | price | effect` rows. Buying writes `CREDIT|spend` and an `ITEM|buy` row.

## 6. Armour: researched, then crafted with an AI model (OWNER: "armor = researched crafted using ai model, duh, thats the point")
- **Research first**: an armour recipe is a research item. Research is a ledger of `RESEARCH|pid|topic|grade|unix` rows (grades in the topic's words, the same learning path as chat). A recipe unlocks when its research grade sum passes a RULE threshold.
- **Crafting with an AI model**: a model proposes the armour design. The house's attrition rule still applies to it (HOUSE, `17.ai/META-STRATEGY-...-2026-11-22.md`):
  1. the model **describes** the armour in one fixed line: `ARMOR_PROPOSAL | name | slot | element_resist=<element>:<tenths> | body_fit=<type>`;
  2. the **numbers** come from RULE rows, never from the model (the model names things, the rules set how strong they are);
  3. a harness case checks the proposal (valid slot, known element, resist within the RULE range);
  4. a **person reviews** it and only then a `ARMOR|add|id|...` row is written.
- **Note on "no model"**: the owner's earlier rule ("we aren't using a model") was for the hai chat (the hai-model). Armour crafting is an explicit, owner-chosen use of a model, bounded as above. OPEN: whether the same bound applies to the chat later (the default is no).
- **Which model**: OPEN. The house has local Gemma on the owner's Mac (`17.ai/ENTITY-LLM-ENDPOINTS-...`). The owner's free-model rule applies (no paid calls without a named job).

## 7. Build order (proposal, each step with a test that can fail)
| # | Piece | Test that can fail |
|---|---|---|
| E1 | event page interpreter (JS) for a small row set; same row set in C | the same page gives the same result in C and JS (parity) |
| E2 | export and reload (pdl and csv), round trip | export, reload in C, export again: files match; a bad row is refused and named |
| E3 | element pick (default element 1 until names exist) | a second pick is refused; the pick sets the first spell |
| E4 | faint, rest, heal | fainted cannot be picked; rest below the threshold does not heal |
| E5 | credits ledger, shop, test faucet | faucet refused with `faucet_enabled=0`; a spend over balance refused |
| E6 | research and armour proposals (fixed lines, rules set numbers, human review) | a proposal with an unknown element refused; a number in the proposal is ignored (rules win) |

## 8. Decisions (OWNER answered some; the rest OPEN)
- Element names (OPEN: the owner will pick later).
- Credit price list and rest turns (OPEN: placeholder RULE values until tuned).
- Shared haibot files: copy only (default) or transfer (OPEN).
- The model for armour (OPEN: local Gemma on the Mac by default).
- Whether the chat-side hai-model may use a model later (default: no).
