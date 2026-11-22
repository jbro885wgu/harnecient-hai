# Reversal options for the customer-service chain, and the choices taken from house precedent (2026-11-22)

Labels: **FOUND** (checked on disk) · **PRECEDENT** (the owner already decided it in a house doc; cited) · **CHOICE** (my pick where precedent is silent; reversible) · **BUILT / NOT BUILT** (as of this doc).

## 0. Correction to book 15
Book 15 said chain signing was missing from the send path. That was wrong. `chain_send.c` (lines 76-160) verifies an Ed25519 signature over `TX|chain|from|to|amount|nonce` when a nonce and signature are given, and refuses a replayed nonce. **But the signature is optional**: with no nonce and signature the send is accepted (line 150). So signing is BUILT and NOT ENFORCED. The fix is to require it on the live path, not to build it.

## 1. The owner's model, in one line (SAID 2026-11-22)
Customer service: a theft is reported, reviewed, and fixed; the house controls the main chain as a hybrid; accounts are rated; "no one can truly steal".

## 2. The options for reversal (more than escrow)
| # | Mechanism | What it does | Built? | Recommend? |
|---|---|---|---|---|
| A | **Escrow hold** (`LOCK` / `PAYOUT` / `REFUND`) | money waits in escrow; a refund before payout returns it | BUILT (`chain_escrow.c`, signed per store design section 27) | **Yes** for purchases |
| B | **Dispute window before PAYOUT** (7 days, a POLICY row) | a sale is not final until the window closes | Policy row exists (`court-hq/policy.pdl`); no DISPUTE row, not wired | **Yes** |
| C | **Reversal row** (`REVERSE`) for a settled transaction, naming the original tx | undoes a settled payment by appending, never editing | NOT BUILT (store design section 13: "reversal of an already-settled PAYOUT" does not exist) | **Yes**, gated (below) |
| D | **Account freeze** (`STATUS | account | jailed | until | case`) | blocks spending and posting while a case is open | NOT BUILT (store design 14.2 defines it) | **Yes**, it stops a thief draining the account while the case runs |
| E | **House checkpoints** (signed rows that the house uses to say which history is main) | the hybrid main chain | NOT BUILT; spec sec. 4 says longest chain | **Only with a limit** (CHOICE 5) |
| F | **Chargeback from an external chain** | reverse value on real Bitcoin or another chain | impossible to do by us | **No**: the store design section 13 says so explicitly |

## 3. The choices, taken from precedent
Owner's decisions of 2026-10-27 (store design section 14.1, "answered in chat") are the precedent. Each row cites it.

| Question | Choice | Source |
|---|---|---|
| **Who decides a small dispute?** | A deterministic rule table. Amounts at or below **50 millicones-equivalent** (POLICY `small_amount`, default 0 until set) are decided by rule: an install that failed, a payout not yet released, or a duplicate charge is refunded. No model decides a number. | PRECEDENT 14.1 row 1; `court-hq/policy.pdl` `small_amount` |
| **Who decides a large dispute?** | Court case files built from ledger rows, a **describe-only judge** that feeds a rule table, and **owner confirmation** above the threshold. | PRECEDENT 14.3, 14.1 row 1 |
| **Judge bias** | **0**, fixed (`judge_bias` must stay 0; the game's office-buying mechanic is NOT carried over). | PRECEDENT 14.3 and `policy.pdl` |
| **Dispute window** | **7 days** before PAYOUT, a data row, with a short dev override (`dispute_window_dev_days` 0). Brand-new accounts never get a shorter window. | PRECEDENT 14.1 row 2; 14.2 |
| **What REP gates** | **Posting rate on p2p, and payout wait.** Low REP: posts rate-limited, payout waits the full window. High REP: faster payout. Store-repo publishing is NOT gated by REP. | PRECEDENT 14.1 row 3 |
| **Whose REP** | **Hybrid, with limited liability**: an owner shares a fraction (`liability_share_pct` 25) of a pet's or agent's penalty, capped. | PRECEDENT 14.1 row 4; 14.2; `policy.pdl` |
| **REP storage** | Append-only `REP | account | delta | reason | ref | ts` rows, derived, never edited. Decay and appeal are rows. | PRECEDENT 14.2 |
| **Jail** | A STATUS row freezes the wallet: no payout, no send, no posting. Ledger reads still work. | PRECEDENT 14.2 |
| **Bail** | The owner may post bail (`bail_by` = owner), as an escrow lock. | PRECEDENT `court-hq/policy.pdl`, mock case c5 |
| **Representation** | Pets and agents use the my-lawyer style local model for representation, never to decide a number. | PRECEDENT 14.3 |
| **Reversal of signed rows** | Every reversal is a NEW row naming the original, signed by the reviewer. Never an edit. | PRECEDENT, store design section 13 ("write transaction code as if it will be disputed") |

## 4. The choices with no precedent (CHOICE, mine, reversible)
1. **Who may write a `REVERSE` row?** Only the owner account, and only after a DECISION row exists that names the case. Reason: the precedent makes the owner the final confirmer above the threshold; no other role is defined.
2. **Every `REVERSE`, `DECISION` and `STATUS` row is signed by the actor's key.** Reason: store design section 13 says a forged dispute is as easy as a forged payment unless rows are signed. Signing exists (section 4 above).
3. **Checkpoints (option E) are limited to ordering, not money.** A house checkpoint row says which of two peer forks the house follows for display and reversal bookkeeping. It cannot mint, move, or unlock coins. The spec's longest-chain rule still decides between peers for mining. Reason: the spec and the owner's hybrid model both need to hold, and this is the smallest power that lets the house reverse fairly. **Flag for you:** this is the one place where the owner's hybrid idea goes beyond precedent. Reverse it if you want the spec's rule unchanged.
4. **Enforce signing on the live send path** (see section 0). Reason: without it a local process can forge a send. Reject unsigned sends once the reset-day plan is set. Until then, the signed path is the one to test.
5. **A harness case for each row type before any reversal goes live**: a stolen-key purchase is disputed and reversed; a false dispute is refused and the victim's REP does not drop; a jailed account cannot send; a `REVERSE` without a `DECISION` is refused.

## 5. Status (checked 2026-11-22)
| Item | Status |
|---|---|
| Escrow LOCK / PAYOUT / REFUND, signed | BUILT (`chain_escrow.c`) |
| Signed sends (optional) | BUILT, not enforced (section 0) |
| Dispute window policy row | exists as a mock (`court-hq/policy.pdl`) |
| Small-amount rule table | PRECEDENT only; no rule table built |
| DISPUTE, DECISION, REVERSE, STATUS, REP rows | NOT BUILT (store design 14.2 defines them) |
| Court (describe-only judge) | NOT BUILT (`my-lawyer` exists; the platform courts do not) |
| House checkpoints | NOT BUILT, CHOICE 3 |

## 6. Build order (my recommendation, each with a case that can fail)
1. Enforce signed sends (4), with a case that an unsigned send is refused.
2. The dispute window (B), with a case that a settled payout cannot be disputed after 7 days.
3. DISPUTE and DECISION rows, the small-amount rule table (precedent 14.1), with the rule cases (refund, install fail, duplicate).
4. REVERSE row with the owner-only gate (CHOICE 1), with the stolen-key case.
5. STATUS (jail) and REP rows, with the two gates.
6. Checkpoints (CHOICE 3) last, and only if you keep them.

## 7. Sources
- Store design, reversal and disputes: `#.#.calendar-dox/!.HQ-IQ-BOOK/16.game/GAMES-AS-PACKAGES-AND-STORE-DESIGN.md` sections 13, 14 (decisions table 14.1), 14.2, 14.3, 15.2, 27, 28.
- Court mock (placeholders, not decisions): `44.xyz.01.00/&.hq-apps/court-hq/policy.pdl`, `mock_cases.pdl`.
- Escrow code: `44.xyz.01.00/041.pal-chain⛓️/ops/chain_escrow.c`. Send code: `ops/chain_send.c`.
