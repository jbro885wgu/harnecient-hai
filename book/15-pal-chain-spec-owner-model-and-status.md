# PAL-CHAIN: the spec is recovered, the owner's model, and what is built (2026-11-22)

Labels: **FOUND** (checked on disk) · **SAID** (owner's words, 2026-11-22; a target, not built) · **READ** (code or doc) · **GAP** (not built) · **OWNER** (a decision for you).

## 1. The spec is not lost (FOUND)
- `PAL-CHAIN-STANDARD.txt` is on disk in five identical copies, under the older ZEST trees on this machine (not in the current house tree or in git history). All five: 22,025 bytes, md5 `b76a9845`, dated 2026-07-19. One copy: `…/xdb/ZEST-10.18+12REPO/…/2.muchi-verse/#.dox/PAL-CHAIN-STANDARD.txt`.
- It is 367 lines, sections 0 to 7: grounding, wallet (wallet_id + password), transactions and blocks (SHA-256), mining (proof of work, 21,000 cone cap, halving), network and consensus (v1 scope), chtpm UI, which pieces are pal, and named gaps.
- **OWNER:** the house still cites a file it does not have. Put a copy in the house tree (for example beside `041.pal-chain⛓️/`), or say the ZEST copy is the authority. I did not copy it into the house repo.

## 2. What the spec itself says (READ, sections 1 to 7)
- Wallet: `wallet_id` + password, SHA-256, **no salt** (its own gap, sec. 7). Reason given by the owner in 2026-07: "sha256 or even weaker because we will refund users if their coins are stolen".
- Transactions: no signatures in v1 (sec. 7: "a malicious LOCAL process could still forge a TX").
- Consensus: longest chain, fork resolution is a gap (sec. 7). Mining is peer-to-peer over `palnet_peer`, full mesh.
- Difficulty: one fixed value, no retargeting (sec. 7).
- Supply: 21,000 cones, halving reward (sec. 3).

## 3. The owner's model (SAID, 2026-11-22; NOT BUILT)
Owner's words: "we are a customer service blockchain. so if someone's stolen from they can report and get it fixed. we control main chain as hybrid … no one can truly steal cuz accounts are 'rated' etc."

Read as a target model:
1. **Customer service first.** A theft is reported, the house reviews it, and the coins are returned. This matches the spec's "we will refund users if their coins are stolen".
2. **Hybrid main chain.** The house controls the main chain. Peers may mine and relay (spec sec. 4), but the house decides which history is main. This **conflicts** with the spec's longest-chain consensus (sec. 4) and needs a rule that says which one wins.
3. **Rated accounts.** Each account carries a rating. The owner's words do not say what the rating measures or who sets it. It must be defined before it is built.
4. **"No one can truly steal."** This holds only if the house is trusted to reverse fairly. That is a policy claim, not a cryptographic one. The doc should say so, so no one reads it as a guarantee.

## 4. What is built against the model (READ, checked 2026-11-22)
| Piece | Status | Evidence |
|---|---|---|
| Reversal | **Partial.** `chain_escrow.c` has `REFUND` (by a named agent, to a payee, capped by the lock). Escrow refund is the only reversal path. | `ops/chain_escrow.c` lines 1-27 |
| Report a theft | **NOT BUILT** | no report op or row in `ops/` or `pal/` |
| Review and decision | **NOT BUILT** | no review row type |
| Account rating | **NOT BUILT** for wallets. (TSC_ELO exists for TSOTS, a different system.) | grep of `ops/` and `pal/` for rating: none |
| House control of main chain | **NOT BUILT** (spec says longest-chain) | spec sec. 4 and 7 |
| Dispute window | **Policy row exists**, not wired to pal-chain | `@.apps/pre-design/`, `chain:cones-test` policy (store design) |

## 5. Gaps against the owner's model (GAP)
1. **Report and decision rows.** A `REPORT` row (who, which tx, why) and a `DECISION` row (who decided, outcome). Both append-only, both on the ledger (no ghost data).
2. **A reversal that is a new row,** never an edit. The reverse of a tx is a `REVERSE` row that names the original tx. Balances are then replayed from the rows.
3. **Who may reverse.** A named role (house operator) with each use recorded. Without this, "customer service" is a backdoor.
4. **A rule for the main chain.** For example: the house's signed checkpoint wins over a peer fork, and every checkpoint is a row. This must be decided by you (OWNER).
5. **Rating defined.** What raises or lowers it, who sets it, and whether it affects spending limits or only reversal priority. OWNER.
6. **Signing (sec. 7 gap).** Still needed. A reversal that is not tied to a signed tx is weaker. Send-path validation (the sender id is unchecked, `chain_send.c` line 161) is a **code bug** and is not deferred.

## 6. Deferred on purpose (OWNER decision, 2026-11-22)
- **Password hashes and the public-history exposure are NOT flagged** while this is dev-only. The owner will reset the wallets on an official day. Track them on a **reset-day checklist**, not as an open risk. This covers: unsalted SHA-256, the two wallet files in public history (`origin/claude`, `origin/grok`, `origin/main`), and changing the `house` and `jb` passwords.
- Not deferred: the sender-id shell injection (a code bug), the missing send signing, and the missing report/decision/reversal rows.

## 7. Next steps (my recommendation)
1. OWNER: place or confirm the spec's home (section 1).
2. OWNER: decide the main-chain rule (3.2 and gap 4) and what the rating measures (gap 5).
3. Build the send-path fix (sender id validated, argv not shell) with a case that can fail. No hash work.
4. Write the `REPORT`, `DECISION`, and `REVERSE` row shapes as a design, then one harness case each.
5. Hash work moves to the reset-day checklist.
