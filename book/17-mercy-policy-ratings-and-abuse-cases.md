# Mercy policy: reversal by rating and dispute, automatic forgiveness for high-rated low-balance accounts (owner, 2026-11-22)

Labels: **SAID** (owner's words) · **PRECEDENT** (house doc decided it earlier, cited) · **CHOICE** (my reading, reversible) · **RISK** (a way this can be abused) · **OWNER** (decision for you) · **NOT BUILT** (as of this doc).

## 1. What the owner said (SAID 2026-11-22)
"reversal based on ratings disputes etc, final mercy refunds for high rated, low balance accounts etc very merciful and automatically forgiving"

Read as policy:
1. **Reversal** of a transaction can be triggered by a dispute or a theft report (not by the account owner alone).
2. **Ratings** (REP) decide how much weight a party gets, and how fast they are paid.
3. **Mercy**: a final refund, applied automatically, for an account that is high-rated and has a low balance, so the account is not ruined by a single loss.
4. **Automatic forgiveness**: penalties and debts are forgiven by rule, without a case.

## 2. Precedent that already fits (PRECEDENT, store design section 14, owner decisions 2026-10-27)
| Owner's words | Precedent (decided) | Source |
|---|---|---|
| reversal on dispute | dispute record, rule-decided small amounts, court and owner above the threshold | 14.1 row 1, 14.3 |
| rating | REP ledger, append-only, derived, never edited; decay and appeal are rows | 14.2 |
| "merciful" | decay and appeal rows exist; a clean-REP account may get a shorter window, a brand-new account never | 14.2 |
| "automatic" | small amounts are rule-decided with no person in the loop | 14.1 row 1 |
| high rating means faster payout | REP gates payout wait and posting rate | 14.1 row 3 |

Precedent says mercy is **shorter windows and decay**, not money from nowhere. The owner's "final mercy refund" is new. It is not in precedent.

## 3. My reading of the mercy rule (CHOICE, reversible)
A **MERCY** row, written by rule (not by a person), when all are true:
- the account's REP is at or above `mercy_rep_min` (a RULE row),
- its balance is at or below `mercy_balance_max` (a RULE row),
- the loss references a **dispute or theft case that is upheld**, and the case has a DECISION row (precedent: reversal needs a decision),
- the account has not used mercy in the last `mercy_period_days` (a RULE row).

The refund is capped at `mercy_cap` per account per period. The source is a named **treasury** account that has its own balance row. Mercy never mints coins. That keeps the 21,000 cone supply cap in the spec (sec. 3) true.

## 4. Where the owner's version and the house rules collide (RISK, must be decided)
1. **Mercy from nothing would break the supply cap.** A refund that creates coins is inflation. My reading: funded from a treasury row only. **OWNER:** agree, or name another source.
2. **Rating farming.** An account makes many small honest sales to build REP, then steals. Mitigation: REP from a sale counts only after its payout window closes (the 7-day window, precedent 14.1 row 2). Recent REP counts less than old REP.
3. **Drain-then-mercy.** A colluding account moves its own coins out, drops to a low balance, and claims mercy on a fake dispute. Mitigations: mercy needs a DECISION row (not an accusation alone), the victim and the claimant must be different accounts with different owners, and a fake dispute that is refused lowers REP (precedent 14.2: a thief's REP drops, the victim's does not).
4. **Low balance is cheap to fake.** Anyone can move coins to a second account. Mitigation: balance is measured across the owner's linked accounts (precedent: shared, limited liability, 14.1 row 4).
5. **"Automatically forgiving" removes the check that stops abuse.** Rule-based forgiveness is fine for small sums; for large sums precedent sends the case to the court and the owner (14.1 row 1). **OWNER:** confirm a ceiling above which mercy is never automatic.
6. **Forgiveness of a debt the house is owed** (a fine or a penalty) is not refund money and needs no treasury, but it must still be a row (no ghost data).

## 5. Choices I am making (CHOICE, reversible; values are placeholders)
| RULE row | value | why |
|---|---|---|
| `mercy_rep_min` | 80 (on a 0-100 scale) | high enough that one clean week does not qualify |
| `mercy_balance_max` | 20 cones (20000 millicones) | "low balance" must mean the account cannot absorb the loss |
| `mercy_cap` | 10 cones per period | a limit a treasury can survive |
| `mercy_period_days` | 30 | one mercy per month per account |
| `mercy_requires_decision` | yes | precedent 14.1: a decision row before any money moves |
| `mercy_auto_max` | 50 millicones (the small-amount rule) | above this a person confirms, precedent 14.1 |

All numbers are placeholders until you set them (OWNER). None is calibrated.

## 6. Cases that must FAIL in the harness (before any of this is built)
1. A high-REP account with a low balance and **no DECISION row** gets no mercy.
2. A mercy refund is **drawn from the treasury**, and the total supply is unchanged (sum of balances equal before and after).
3. A second mercy inside `mercy_period_days` is refused.
4. A refund above `mercy_cap` is capped.
5. A claimant whose linked account is the other party in the dispute is refused.
6. A fake dispute that is refused **lowers the claimant's REP** and does not change the victim's.
7. REP from a sale **before its window closes** does not count toward `mercy_rep_min`.
8. A mercy over `mercy_auto_max` is not automatic (it waits for the owner).
9. Mutant: remove the treasury check. Case 2 must go red.

## 7. Status (checked 2026-11-22)
| Item | Status |
|---|---|
| Escrow refund | BUILT (`chain_escrow.c`) |
| REP ledger, decay and appeal rows | defined in store design 14.2, NOT BUILT |
| DISPUTE, DECISION, REVERSE, MERCY rows | NOT BUILT |
| Treasury account | NOT BUILT (no such row found; to be checked before build) |
| Supply cap (21,000) | in spec sec. 3; code check NOT done |
| Mercy rules | CHOICE in this doc, values placeholders |

## 8. Owner decisions needed
1. Mercy source: treasury only (my default), or something else?
2. Ceiling above which mercy is never automatic (my default: 50 millicones auto, a person above that).
3. Should a rating farm be penalised when it is detected, and by how much?
4. Are the placeholder values in section 5 right, or do you want to set them?

## 9. Build order (my recommendation)
Nothing is built yet. Precedent 14 comes first: REP and DECISION rows, the small-amount rule table, then the treasury and MERCY rows with the cases in section 6. Mercy waits until the dispute and decision rows pass their cases.
