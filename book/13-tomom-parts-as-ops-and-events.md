# tomom parts rebuilt as ops and events (plan, 2026-11-22)

Labels: **HOUSE** (read in the house, file and line counts measured) · **BUILT** (repo, harness, observed) · **PLAN** (not started) · **OPEN** (owner decision).

## 0. What the owner asked (2026-11-22)
"we can reuse rebuild tomom parts as events/ops thats the real desire." And earlier: "we do have attention and ff back prop even if we skip them and choose to do them by hand."

So the goal is not to run tomom as a model. It is to take each of its parts (attention, feed-forward, backprop, optimizer, mixture-of-experts router, meta-RL) and rebuild each one as a **small op with a harness check** that an **event page** calls, with every number in a text row. The weights stay human-readable, as tomom already designed them.

## 1. What tomom is (HOUSE, read 2026-11-22)
Folder: `#.Z.HUMAN_LLM/3.stage.llm.tomom@qroq.fame]921🐋️/`. Feasibility note: `#.Z.HUMAN_LLM/^.feasibility_handtune_aug01_2026.txt` (2026-08-01). Last touched 2025-12-16; **never smoke-tested since** (the note says so).

| Part | File | Lines (measured) | What it does |
|---|---|---|---|
| attention | `attention.c` | 302 | W_q, W_k, W_v per token; softmax over scores (`softmax`, line 43) |
| feed-forward | `forward_prop.c`, `mlp_layer.c` | 310, 218 | the layer after attention |
| backprop | `backward_prop.c`, `grad_*.txt` | 490 | gradients from the loss |
| optimizer | `optimizer.c` | 474 | Adam, moments in `corpuses/*.m/.v` |
| vocabulary | `vocab_model.txt` | text | per token: number, word, embedding, pe, weight, bias1-4 |
| MoE router | `chatbot_moe_v1.c`, `curriculum_bank.txt` | 559 | picks which vocabulary/corpus answers a prompt |
| meta-RL | `meta_rl/meta_rl.c`, `meta_rl_weights.txt` | text | linear score over prompt features (word count, question, sentiment) picks a corpus |
| server | `http_server.c` | — | an editor UI |

Readable: yes. `meta_rl_weights.txt` holds lines like `rl_weight_6 -0.520000`. `attn_scores.txt` holds the attention scores as text.

## 2. The map: each part becomes an op, a check, and an event
| tomom part | hai op (to build) | what its harness proves (hand-computed) | event that calls it |
|---|---|---|---|
| softmax inside attention | `softmax_op` | softmax of equal inputs is uniform; output sums to 1; a lower temperature (RULE row) sharpens it | attention step |
| attention scores | `attn_op` | score = dot(q, k) for a 2-word example, worked by hand | "which concept does this input attend to" |
| feed-forward | `ff_op` | a 2×2 layer with ReLU, output by hand | "score each concept from its word counts" |
| gradient from a grade | `grad_op` | loss = 1 − grade/4 (grade 0 → 1, grade 4 → 0); gradient = loss × input, by hand | a GRADE row, so the grade IS the training signal |
| optimizer | `opt_op` (SGD first; Adam later) | one SGD step: w − lr × grad, by hand; lr is a RULE row | after each grade |
| MoE router | `route_op` | the top-scoring concept bank wins; ties go to file order | the turn event, before the concept match |
| meta-RL score | `route_op` features | features from the input (word count, has a question mark, match share) times weights rows | the same router, with a learned bias |

Every op: one `--describe` line, numbers in RULE or weight rows, a case that can fail, and a mutant that turns it red.

## 3. What this means in hai terms (honest)
- hai's concept match is already a hand-tuned attention over concepts: matched words × weight (book 12). The rebuild makes that explicit: the scores become `softmax_op` outputs, and the weights become rows that `opt_op` moves after each grade.
- The grade is a loss. Today a grade moves a count (Laplace). With `grad_op` and `opt_op` it moves a weight by a gradient step. Both are readable; the step version can learn things a count cannot.
- Attention over **words in a sentence** (tomom's real attention across tokens) is NOT in the plan yet. It needs sequences, which hai does not have. It stays OPEN until the first six ops are green.
- tomom's own vocabulary is a toy: one scalar embedding per token. We do not copy its numbers; we copy the shapes.

## 4. Event rows needed (the one gap in E1)
Event pages can only write rows and set values (book 09). To call an op they need a `CALL` row:
`THEN | CALL | op_name | arg...`, where `op_name` must be on a whitelist file, the arguments are resolved like `APPEND` fields, and the op's output lines become `FACT` values. This is PLAN; the interpreter stays refused for any op not on the whitelist.

## 5. Build order (PLAN, one at a time, each green before the next)
1. `softmax_op` + case (uniform, sum to 1, temperature sharpens). Mutant: no normalisation.
2. `grad_op` + case (loss from grade; gradient by hand). Mutant: wrong sign.
3. `opt_op` SGD + case (one step by hand). Mutant: lr ignored.
4. `ff_op` (2×2 ReLU by hand). Mutant: no ReLU.
5. `route_op` with weights rows (ties to file order). Mutant: ties to later file.
6. `CALL` row in the event interpreter (C and JS, parity), whitelist only.
7. Event pages that call steps 1 to 5 after each grade.
8. Later: Adam (needs the moments as rows), attention across a sentence (OPEN).

## 6. OPEN (owner decides)
- Start with step 1 (softmax) now? (My recommendation: yes, it is small and checks the idea.)
- Keep tomom's Adam, or SGD only at first? (My default: SGD.)
- Whether the router may change which concept bank answers, or only rank options (default: rank only, until the harness proves it).
- Whether any of this replaces the count-based weights, or runs beside them (default: beside, compared by the calibration measure).

## 7. Status (observed 2026-11-22)
Nothing in sections 2 to 5 is built yet. Built this session: calibration (`hai_chat calib`, ECE 0.2300 on the hand-checked ledger; `hai_calibration` 6/6; mutant red; JS port identical).
