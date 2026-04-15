---
name: vox-prompt-eval
description: Run Vox's cleanup (and later command-mode) system prompts against a golden fixture set and report pass/fail with diffs. Use after any change to cleanup.system.txt, command.system.txt, or the LLM engine. Also the foundation for tone-matching regression tests in Phase 2.
---

# vox-prompt-eval

The cleanup prompt is the single highest-leverage piece of text in the product. This skill is how we stop it from regressing.

## When to run

- After editing `Vox/Resources/Prompts/cleanup.system.txt` or `command.system.txt`.
- After upgrading the LLM model (Gemma 4 E2B → E4B → successor).
- Before any release (auto-gated in CI).
- When a user reports "my dictations got worse" — we need reproducible regression cases.

## Fixture format

Goldens live under `VoxTests/Fixtures/cleanup_goldens/`. Each example is a directory:

```
VoxTests/Fixtures/cleanup_goldens/
├── 001_filler_removal/
│   ├── input.txt        # raw Whisper-style transcript
│   ├── context.json     # { "app": "Mail", "dictionary": [], "style": null }
│   └── reference.txt    # the ideal cleaned output
├── 002_self_correction/
│   └── ...
└── 003_bullets/
    └── ...
```

New categories to cover:

| ID range | Category |
|----------|----------|
| 001–019 | Filler removal |
| 020–039 | Punctuation + capitalization |
| 040–059 | Self-corrections |
| 060–079 | Numbered / bulleted lists |
| 080–099 | App-context formality (Mail vs iMessage vs Slack vs Xcode) |
| 100–119 | Dictionary hinting (jargon, proper nouns) |
| 120–139 | Tone matching (Phase 2) |
| 140–159 | Command Mode rewrites |

Minimum bar: **20 goldens per category before that category gates CI**.

## How the harness works

```bash
# Run locally (requires Phase 1 Swift code)
xcodebuild test \
  -scheme Vox \
  -only-testing:VoxTests/PromptEvalTests \
  -destination 'platform=macOS'

# Or run the standalone Python scorer against a JSON dump from the Swift harness
python3 .claude/skills/vox-prompt-eval/scripts/score.py \
  --results build/prompt-eval-results.json \
  --threshold 0.85
```

The Swift test target:
1. Loads each golden's `input.txt` + `context.json`.
2. Runs `PipelineActor.dictate(...)` stopped before the paste step.
3. Writes `{id, input, reference, got, score}` rows to `build/prompt-eval-results.json`.

The scorer (`scripts/score.py`) computes:
- **Exact match** (binary).
- **Normalized edit distance** (Levenshtein / max-len).
- **Semantic similarity** via a simple word-overlap baseline (optional: plug in an embedding model later).

A golden "passes" if:
- Exact match, **or**
- Normalized edit distance ≤ 0.10 **and** no new content introduced (no tokens in `got` that weren't in `input` or `reference`, excluding punctuation/case).

Category pass rate ≥ threshold (default 0.85) → pass. Below → fail, with per-case diffs printed.

## Authoring a new golden

Use `scripts/new-golden.sh 042_my_new_case`:

```bash
bash .claude/skills/vox-prompt-eval/scripts/new-golden.sh 042_self_correction_datetime
```

This creates the directory and skeleton files. Fill in `input.txt` (raw), `context.json` (app + dictionary + style), `reference.txt` (desired output).

## Outputs

- Console: per-case status (`PASS`/`FAIL`), category rollup, overall score.
- `docs/benchmarks/prompt-eval-YYYYMMDD.md`: timestamped markdown report. Commit this on every prompt change so we have history.

## How to respond to failures

1. Read the diff for each failing case.
2. If the new prompt is legitimately better in a way the golden doesn't capture: **update the golden** (with a comment in the commit: "tightening golden to match improved self-correction handling"). Never weaken a golden to silence noise.
3. If the new prompt regressed: either revert or iterate.
4. If the failure is flaky (LLM non-determinism), set temperature=0 in the harness config. We intentionally run at `temperature=0` for reproducibility.
