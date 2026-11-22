# Faucet correction, and halving the 21,000 cap (2026-11-22)

Labels: **FOUND** (checked in code) · **SAID** (owner) · **CHOICE** (my reading, reversible) · **RISK** · **OWNER** (decision).

## 1. Units (FOUND)
- 1 cone = 1,000 millicones.
- `TOTAL_SUPPLY_MILLICONES` = 21,000,000 = **21,000 cones**. (`chain_miner.c` line 79)
- `INITIAL_REWARD_MILLICONES` = 10,500 = **10.5 cones per block**; `HALVING_PERIOD_BLOCKS` = 1,000. (lines 77-78)
- The cap is exactly the sum of that schedule: 10,500 × 1,000 × 2 = 21,000,000. So the cap is not a separate number; it follows from reward and halving period.

## 2. The faucet (correction to book 18, FOUND)
Owner: "the faucet doesn't mint. but it mines." Checked against `chain_miner.c` (lines 260-263):
- A `FAUCET` line in a block **adds the amount to the wallet's balance** (`*wallet_bal(tf[1]) += amt`). Nothing is subtracted from anywhere.
- The supply counter `total_minted` (line 371) sums **only block rewards** (`reward_for_block`). A faucet credit is not added to it.
- So faucet coins sit **outside the 21,000 cap**. Whether you call that minting or mining, the balance rises and the cap does not see it.
- It is only allowed where `chain.pdl` enables the faucet, which the real cones chain does not (chain_faucet.c header). So the real chain is not affected today. Test chains are.
- Book 18 said "faucet mints". Correct it to: **credits balances outside the reward schedule; not counted against the cap; enabled only on chains that declare it.**

## 3. "Corporate miners with easier difficulty" (FOUND, not built)
- Difficulty is **one value per chain**: `MINING difficulty_hex_zeros` in `chain.pdl`, or the env var. (`difficulty_hex_zeros()`, line 100)
- There is **no per-miner difficulty** in the code. A hybrid where corporate miners mine easier, and the faucet uses that path, is a design, not code. The design doc grep found no match, so the source is the owner's words only.
- The owner says this is known upfront and is "the cost of doing business on our chain". That is a **disclosure** rule, which can be built as a row (a `DISCLOSURE` row the chain shows before a user joins).

## 4. Halving the 21,000 cap (SAID: "the 21k will halve")
Two readings, both a change to `chain.pdl` rows:
| Reading | What changes | Cap after | Note |
|---|---|---|---|
| A. The cap halves | `total_supply_millicones` 21,000,000 → 10,500,000 | 10,500 cones | the reward schedule must also be changed, or minting stops at the new cap early |
| B. The reward halves from now | `initial_millicones` 10,500 → 5,250 | 10,500 cones (because the sum halves) | cap and reward stay consistent |
| C. The halving period halves | `halving_blocks` 1,000 → 500 | 10,500 cones | same total, faster emission |

**CHOICE (mine):** reading **B** (halve the initial reward, keep the cap a consequence of it). Reason: the cap is then derived from one schedule, not two numbers that can disagree. It also matches the spec's "halving reward" (sec. 3).

**RISK, the important one:** the miner checks `reward_for_block(idx)` against each block's index, so changing the schedule **changes the validity of blocks already mined**. A new schedule applied to the whole chain is a hard fork. Two safe options:
1. **Epoch row:** a `REWARD_EPOCH | from_block=N | initial_millicones=5250` row. Blocks before N keep the old reward; blocks at or after N use the new one. The total stays exact: already-minted coins are not touched.
2. **Only at a fresh chain** (a new genesis). Simple, but it resets balances.

**CHOICE:** option 1, the epoch row, applied at a block height you choose. It keeps the existing history valid, and it is a row, so the change is visible.

## 5. Checks before any change (must FAIL if wrong)
1. The new cap equals the sum of the schedule (no separate number that can disagree).
2. A block before the epoch row uses the old reward; a block at or after it uses the new one.
3. Total minted after the change never exceeds the new cap; mining stops at the cap (existing `supply cap reached` path).
4. A faucet credit does not raise `total_minted` (documents the outside-the-cap fact).
5. Mutant: apply the new reward to all blocks, not from N. Case 2 must go red.

## 6. Owner decisions
1. Which reading: A (cap halves, schedule follows), B (reward halves, my default), or C (period halves).
2. The activation block N, or a fresh chain.
3. Whether the faucet's uncapped credits should count against the cap on test chains, or stay outside it (my default: outside, labelled, test chains only).
4. The "corporate miners mine easier" rule: as a `DISCLOSURE` row shown before joining (my default), or left as a verbal understanding.

## 7. Status
Nothing changed in code. The cap and schedule are as above (FOUND). Book 18 is corrected by section 2 here.
