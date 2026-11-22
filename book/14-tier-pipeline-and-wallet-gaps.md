# Tier pipeline (free public, paid private, 0 cones) and the wallet gaps (2026-11-22)

Labels: **BUILT** (repo, harness, observed) · **HOUSE** (read in the house, checked) · **GAP** (a hole, not fixed) · **OWNER** (needs your decision or action).

## 1. The free-tier pipeline (BUILT)
- Rules: `chat/data/tier_rules.pdl`. Free = public chain, paid = private chain, price 0 millicones for both (owner: "0.00 cones for now"). Prices are millicones (1 cone = 1000), the unit `chain_send` uses.
- Op: `ops/hai_tier.c`.
  - `hai_tier check <rules> <free|paid> <public|private>`: exit 0 allowed, 2 setup, 4 refused.
  - `hai_tier pack <rules> <free|paid> <ledger> <outdir>`: public tiers only. Writes `pack.pdl` (SHA-256 and row count of the ledger) and a copy of the ledger. A private chain writes nothing (exit 4).
- A pack is a projection (a copy of the ledger); the ledger stays the authority.
- Harness `cases/hai_tier.pdl`: 10/10. Mutant (visibility check removed) turns 3 checks red.
- The SHA-256 in a pack matches `sha256sum` on the same file (checked by hand).
- **Not wired yet:** the rules are not yet read by `chain_create_wallet` or the send path. Wiring is a GAP (section 3, item 6).

## 2. Wallet gaps found in the house (checked 2026-11-22)
Sources: `44.xyz.01.00/041.pal-chain⛓️/` (ops, pal, wallets), `16.game/WALLET-CONVENTIONS-AND-OPEN-CALLS-2026-11-02.md`, `08-roadmap/design-docs/PAL-CHAIN-META-AND-SUBCHAINS.md`.

| # | Gap | Evidence | Label |
|---|---|---|---|
| 1 | **[RESET-DAY CHECKLIST, not flagged while dev-only: owner 2026-11-22]** Wallet password hashes are still in public history. Unsalted SHA-256 of `house` and `jb` wallet passwords. The 2026-11-02 untrack commit `7485cfcc0` stopped tracking them, but `git log --all` still lists both files, and they exist on `origin/claude`, `origin/grok` and `origin/main`. The repo is PUBLIC (`gh repo view`). | `git branch -r --contains 7485cfcc0`; history hits 2 | **OWNER**: change the two passwords now. Rewriting public history needs your yes (force-push, every clone affected). I did NOT touch it. |
| 2 | **[RESET-DAY CHECKLIST, not flagged while dev-only]** Unsalted hashes. `chain_create_wallet.c` says so itself: no salt in v1, and the owner's stated risk tolerance is "sha256 or even weaker, we refund if stolen". | `chain_create_wallet.c` header | **OWNER**: confirm the tolerance, or require a salted hash (the hai account already uses PBKDF2-SHA256 with salt, a house-tested pattern). |
| 3 | **Transactions are not signed in v1.** `chain_send.c` says "v1 has no transaction signing"; `tx_id` is a dedup key, not a signature. The Ed25519 `sig_op` and its harness cases (`chain_signed`) exist, but the send path does not use them. A signed harness proves the signer, not the send. | `chain_send.c` header lines 10-11; `sig_op` per the 2026-11-02 doc | GAP: wire `sig_op` into send, and make a case fail if an unsigned send is accepted. |
| 4 | **Shell injection in the balance call (NOT RUN, read from code).** `chain_send.c` builds `cd '<root>' && ./ops/+x/chain_balance.+x <wallet_id>` and runs it through `popen`. The id is not quoted. The RECIPIENT id is format-checked (line 138), but the SENDER id is passed straight to `current_balance(from)` on line 161 with no check. A sender id with a `;` or a space would run extra commands. Who can supply the sender id: whatever calls `chain_send` (a local user, or any path that feeds it a peer's tx; not checked). | `chain_send.c` lines 56, 138, 161 | GAP, **high**: validate the sender with the same `valid_wallet_id` check, call the op with argv instead of a shell, and add a harness case that sends a hostile sender id and expects refusal. I have not run that case. |
| 5 | **Balance is derived, not ledgered.** The send code says the balance is "derived" by shelling out to `chain_balance`, a "limitation" of the spec. That conflicts with the no-ghost-data rule (the ledger is the only authority). | `chain_send.c` lines 2-19 | GAP: the balance should be a projection replayed from the ledger, with a case that a hand-edited balance file changes nothing. |
| 6 | **Tier rules not wired.** Nothing reads `tier_rules.pdl` when a wallet is created or a chain is published. | this doc, section 1 | GAP: wire `hai_tier check` into the publish step, then into wallet creation. |
| 7 | **The spec is missing.** `PAL-CHAIN-STANDARD.txt` is cited by every `chain_*.c` and is absent from the tree and git history. | `PAL-CHAIN-META-AND-SUBCHAINS.md` header | **OWNER**: recover it, or write it. Until then no chain change can be checked against a rule. |
| 8 | **Faucet makes test coins.** `chain_faucet.c` exists. Test coins are not money, but a faucet left on in a public build makes "0.00 cones" look like a price. | `ops/chain_faucet.c`; hai faucet ships off (book 08) | OWNER: keep the faucet off in any public pack; say so in the store page. |
| 9 | **Unresolved branch.** Local `kilo` and `origin/kilo` diverged (2026-11-02). Pushed as `kilo-local-2026-11-02`. The merge is undecided. | wallet doc section 4 | OWNER. |

## 3. What I did not do, and why
- I did not change the passwords (only you can), and did not rewrite history (it breaks every clone and is a public, irreversible-in-practice change).
- I did not touch `chain_send.c` (items 3 to 5). They change a live money path and need the spec (item 7) first.
- I did not wire the tier into `chain_create_wallet.c` (item 6). It is a shared, live file.

## 4. Priority order (my recommendation)
1. Owner: change the `house` and `jb` passwords (item 1). Decide on history rewrite.
2. Fix the shell injection (item 4) with a hostile-id harness case. Smallest, highest risk.
3. Recover or write the spec (item 7). Everything else depends on it.
4. Wire signing (3) and ledgered balance (5), each with a case that can fail.
5. Wire the tier (6) once 2 to 4 are green.

## 5. Gap 4 reproduced (2026-11-22, by the agent as Haiku; run in a scratch root, live code untouched)
- Method: a scratch copy of `chain_send.c` with `PRISC_PROJECT_ROOT` pointing to a scratch root whose `chain_balance` stub prints `5000`. Sender id `x;touch <scratch>/PWNED_LIVE`, recipient `bob`.
- **Live code:** exit 0, the marker file is **PRESENT**. The sender id ran a shell command. **Reproduced.**
- **One-line fix, on the scratch copy only:** insert before `long balance = current_balance(from);`:
  `if (!valid_wallet_id(from)) { fprintf(stderr, "Invalid sender wallet_id.\n"); return 1; }`
  Hostile sender: exit 1, marker **absent**. Honest sender `alice` to `bob`: "Sent 10 millicones", exit 0.
- **Not yet done:** the fix is NOT applied to the house file (`041.pal-chain⛓️/ops/chain_send.c` is on the house repo, a live money path, with concurrent agents). A stronger agent or the owner applies it, then adds the hostile-id case to the house harness.
- Caveat: the balance was a stub. The real `chain_balance` was not run.
