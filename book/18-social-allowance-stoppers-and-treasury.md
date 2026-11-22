# Social allowance, stoppers, and the self-sustaining treasury (owner, 2026-11-22)

Labels: **SAID** (owner's words) · **FOUND** (checked in code or docs) · **PRECEDENT** (a decided house rule) · **CHOICE** (my reading, reversible) · **GAP** (designed or missing, not built) · **OWNER** (your decision).

## 1. What the owner said (SAID 2026-11-22)
"the exploit is kind of okay, we also have faucets etc. it's like social allowance. we have stoppers for that, it will lower their cooldown for reports etc, so w/e they game the system every week. the rest of the chain's taxes, tributes etc fund this and it keeps the economy self sustaining."

Read as policy:
1. Gaming the allowance is **accepted as a cost**, the way a welfare system accepts some fraud.
2. The allowance is paid out **every week** to accounts that qualify.
3. **Stoppers** throttle the abuse: when an account games the system, its cooldowns for reports and claims get longer.
4. **Taxes and tributes** on the rest of the chain fund the allowance, so the economy stays self-sustaining.

## 2. What exists (FOUND)
| Piece | State | Evidence |
|---|---|---|
| Faucet that **mints** (`FAUCET` mint line) | BUILT, but **test chains only** by design: refused on the real cones chain (no `chain.pdl`, or kind = cones). Inflation "by design on test chains". | `041.pal-chain⛓️/ops/chain_faucet.c` header |
| Faucet stoppers | BUILT: `cooldown_seconds` per wallet, `daily_cap_millicones_per_wallet` per UTC day, both from append-only ledger rows | same file, lines 6-11 |
| Master treasury receives tax and fee | **GAP**, designed only | `CHAIN-MASTER-BANK-FED-AND-CONTRACTS-DESIGN-2026-10-28.md` section 3 |
| Master tax, transaction fee, bonds, royalties, annuities | **GAP**, designed only; the design doc says "not found anywhere" in the docs before it | same doc, line 23 |
| The word "tribute" as a defined term | **GAP**: the design doc marks it OPEN | same doc, section 4 |
| Weekly allowance | **GAP**: no row, op, or schedule found | not in code |
| Report cooldown (stopper for reports) | **GAP**: the store design defines dispute windows and REP, not a report cooldown | `GAMES-AS-PACKAGES-AND-STORE-DESIGN.md` 14 |

## 3. Two very different things the owner's words could mean
1. **A faucet** (mints). That is inflation. It is OK on a test chain (the faucet's own design says so) and **forbidden on the real cones chain** if the 21,000 cap (spec sec. 3) must hold.
2. **A treasury payout** (recirculates). Money comes from taxes and tributes already collected, so total supply does not change.

**CHOICE 1 (mine):** the real-money social allowance is a **treasury payout only**. The faucet stays a test-chain tool. This keeps the 21,000 cap true and matches the owner's "self-sustaining" wording, which only works if the money already exists.

## 4. The self-sustaining rule (CHOICE 2, an invariant the harness can check)
Per period (one week):
- `allowance_paid <= tax_in + tribute_in`, and
- the treasury **never goes below its floor** (`treasury_floor`, a RULE row).

If the treasury reaches the floor, the allowance **pauses automatically** until it recovers. That is the big stopper, and it protects the economy from gaming, whatever the gaming cost is. Every pause and every payout is a row.

## 5. Stoppers (what the owner called "stoppers") (CHOICE 3, using the faucet's precedent)
Each is a RULE row, the same shape the faucet already uses:
| Stopper | Row | Precedent |
|---|---|---|
| Cooldown per account between allowance claims | `allowance_cooldown_days` | faucet `cooldown_seconds` |
| Cap per account per period | `allowance_cap_per_period` | faucet `daily_cap_millicones_per_wallet` |
| Longer report cooldown for flagged accounts | `report_cooldown_days` raised by a STATUS row | store design 14.2 (STATUS rows, jail) |
| Treasury floor pause | `treasury_floor` | new (this doc), same pattern as the cap |

## 6. Accepted cost (SAID: "w/e they game the system every week")
This is a **budgeted loss**, not a bug. The owner accepts it. Two things still must hold:
1. The treasury floor (section 4) limits the total gaming loss. Gaming can drain the allowance budget but cannot drain the treasury below the floor.
2. A **COST row** each period records the allowance paid, the number of accounts that hit a stopper, and the treasury balance. That makes the cost visible, so the owner can see it, instead of hiding it in balances.

## 7. Cases that must FAIL (before any of this is built)
1. The real cones chain refuses a faucet mint (the existing refusal is kept).
2. An allowance payout reduces the treasury by exactly the paid amount, and total supply is unchanged.
3. A payout that would take the treasury below `treasury_floor` is refused, and the allowance is paused.
4. Claims inside `allowance_cooldown_days` are refused.
5. A flagged account's report cooldown is longer than a clean account's.
6. A week with `allowance_paid > tax_in + tribute_in` writes a COST row that flags the imbalance.
7. Mutant: remove the floor check. Case 3 must go red.

## 8. Owner decisions
1. Allowance size per week, and whether it is the same for every account.
2. The floor (`treasury_floor`) value.
3. Which tax and tribute rates feed the treasury (the design doc's "master tax" rate, not yet set).
4. Whether a flagged account loses its allowance entirely, or only waits longer.

## 9. Status (checked 2026-11-22)
Nothing in sections 4 to 7 is built. The faucet's stoppers and its test-only refusal are built. The treasury, tax, tribute, and allowance are design only. The next real step is the master treasury row, with its floor and the COST row, before any allowance is paid.
