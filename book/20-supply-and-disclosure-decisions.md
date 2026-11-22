# Supply, faucet and disclosure: decided (2026-11-22, by the agent, from house precedent)

Labels: **FOUND** (checked) · **PRECEDENT** (a decided house rule, cited) · **DECIDED** (my call; reversible by a row, not an edit).

## 0. What is on the live chain (FOUND)
- `041.pal-chain⛓️/data/blockchain.txt` has 3 rows: the chain is at its first few blocks.
- No `chain.pdl` exists, so the faucet is off and the schedule is the built-in constants (`chain_miner.c` lines 77-79).
- Minted so far is about 10.5 cones per block: a few tens of cones at most (not counted precisely here).

## 1. Halving the cap: reading B, applied now (DECIDED)
- **Choice:** the initial reward halves, and the 21,000-cone cap becomes **10,500 cones** (10,500,000 millicones). The cap stays derived from one schedule.
- **Activation:** a `REWARD_EPOCH` row at the current head. Blocks before it keep their reward. Blocks from it use the new reward.
- **Formula, read from the ledger (not typed):** the new per-block reward `R` from block `N` is `R = (cap − minted_before_N) ÷ (2 × halving_blocks)`. So the total is exactly the cap.
- **Why now:** the chain is a few blocks old and is dev-only. The owner's own rule (hash and password items are deferred to an official reset day) says dev state is cheap to change. Changing it after people hold coins costs far more. Precedent: the chain's own spec states the halving reward (sec. 3), and rules are data rows (`chat_rules.pdl`, `policy.pdl`).
- **Why not at the first halving (block 1000):** by then 10,500 cones are already minted, so a 10,500 cap would stop mining at once. Applying it now avoids that.

## 2. Faucet credits (DECIDED)
- Faucet credits stay **outside the cap**, and they are **labelled** as such. They exist only on test chains.
- **Precedent:** `chain_faucet.c` header: "inflation by design on test chains", and the real cones chain refuses a faucet (no `chain.pdl`, kind = cones). I keep both.

## 3. Disclosure of the easier-mining rule (DECIDED)
- The rule is written as a `DISCLOSURE` row, shown before a user joins a chain. It must name: the hybrid corporate miners, that they mine on easier difficulty, and that the faucet uses that path.
- **Precedent:** the master bank design (section 8): "the buyer sees the full breakdown before paying", and the store design's screening before install. Users see the cost first; nothing is hidden in a balance.
- **Not built:** the row does not exist yet. The per-miner difficulty it describes is also not built (book 19 section 3).

## 4. Summary of the rows to add (none written yet)
| Row | File | Value |
|---|---|---|
| `REWARD_EPOCH` | `chain.pdl` | from the current head; cap 10,500 cones; R from the formula above |
| `FAUCET` | `chain.pdl` (test chains only) | enabled = 1 only where declared |
| `DISCLOSURE` | `chain.pdl` | the easier-mining text, shown before join |

## 5. Must-pass checks before any of this runs
1. Total minted at the end equals 10,500 cones exactly (the formula, not a typed number).
2. A block before `N` uses the old reward; a block at or after `N` uses `R`.
3. A faucet credit does not raise `total_minted` (documents section 2).
4. The real chain with no `chain.pdl` refuses a faucet claim (existing behaviour, kept).
5. A join without the `DISCLOSURE` shown is refused (once the join path exists).
6. Mutant: apply `R` to all blocks, not from `N`. Check 2 must go red.

## 6. Still open (cannot be decided from precedent)
- Whether the new cap should be exactly 10,500 cones, or a round number near it. I chose exact, because the existing schedule is exact.
- Whether a fresh genesis is preferred to an epoch row. An epoch row keeps the 3 blocks; a fresh genesis drops them. I chose the epoch row because it keeps history, and nothing is lost.
