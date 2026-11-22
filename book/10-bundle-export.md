# E2: export and reload of a haibot ledger, as .pdl or .csv (built 2026-11-22)

Labels: **BUILT** (repo, harness case, observed) · **GAP** (known, not done).

## 1. What was built
- `haibot/hai_bundle.c`: `export <ledger> pdl|csv <out>`, `check <file>`, `import <file> <ledger>`. Rows are copied verbatim; nothing is recomputed.
- `docs/bundle.js`: the same logic in JS. Same output text.
- Harness `cases/hai_bundle.pdl`: **10/10 PASS**. Negatives: a non-empty target ledger (exit 5), a stat row (exit 4, line named), a csv `n` that does not match its cells (exit 4).
- Mutant with CSV quoting off: **3 FAIL** (the import, the byte-identical round trip, and the refusal). So the round trip depends on quoting.

## 2. The formats
- **PDL**: first line `# bundle v1`, then one ledger row per line (`TURN|...`, `GRADE|...`). The house's row format, unchanged.
- **CSV**: first line `kind,n,f1,...,f8`. Each row: the kind, `n` = the total field count **including the kind**, then the values (RFC 4180 quoting: a value with a comma or a quote is wrapped in quotes, quotes doubled). The `n` is what lets a trailing empty value survive.
- **Allowed kinds**: `TURN` (8 fields) and `GRADE` (9 fields) only. Anything else is refused: a bundle cannot set a stat, a state, or a rule. That is the no-ghost-data rule applied to files.

## 3. The round trip (the test that matters)
Ledger (a real chat, with a comma in the input) → `export csv` → `import` into a new ledger → `export pdl`. The last file must be **byte identical** to the first `export pdl`. Verified in C (the harness) and in JS (same bytes on the same ledger).

## 4. JS parity (run 2026-11-22)
On the same ledger: JS `exportCsv` = C csv (identical); JS `exportPdl` = C pdl (identical); JS csv → ledger → pdl = C pdl (identical). Run by hand; not yet a harness case (see GAP).

## 5. GAP (honest)
- The JS parity is not a harness case yet (same gap as E1).
- A bundle carries chat rows only (TURN, GRADE). Element picks, party and PC rows, and credits are not in a bundle yet; they join the allowed list when those ledgers exist.
- A shared file from another person is a copy: `import` never merges into an existing ledger (exit 5). Whether a transfer is allowed is still an owner decision (book 08 section 8).
- No signature on a bundle. A file can be edited by hand and imported if its rows pass the allowed-kind check. The check stops bad kinds and bad counts, not a changed number in an allowed row. The replay of the ledger still decides the state, but a hand-tuned grade would be accepted. Owed: a row checksum.
