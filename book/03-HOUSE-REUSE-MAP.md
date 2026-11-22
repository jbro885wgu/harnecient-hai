# hai: what we reuse from the house (READ = opened; FOUND = located only)

| Piece | Where | State |
|---|---|---|
| Install pipeline (curl, payload, launcher) | `x0.../xyz-installer-dev/tearit-install/install.sh`, `payload-src/` | READ (README); not re-run |
| FSM / decision layer design | `17.ai/AI-ROADMAP-...-2026-11-05.md`, `17.ai/AUTO-AND-AUTO-STUDIO-DESIGN.md` | FOUND; read before M3 |
| Concept bank and ranking | `harness_bank_op find` (`_shared-lib/harness/ops`), `&.widgits/concept-bank/` | RAN earlier (rank works); reuse as-is |
| Reward / punish weights | `harness_bank_op update`, Laplace weight | RAN (house finding, 2026-10-17 play bank) |
| Tool-use logging, A/B, scoring | `tool_log_op`, `tool_ab_op`, `tool_score_op`, `tool_rules.pdl` | RAN (27 / 25 / 28 checks, 0 fail, 2026-11-22) |
| Human-readable DATA rows, ledgers | HQ-IQ-BOOK no-ghost-data standard | READ (AGENTS.md) |
| Tomom / Gemma notes | `#.haiku+`, `#.Z.HUMAN_LLM` in the `x/` worktree | NOT USED (no model in hai) |
| Chat window | X11-HQ | NOT READ yet |

The `x/NNEST-12.00-opencode` worktree is on another branch (`opencode`). Copy only what we read.
