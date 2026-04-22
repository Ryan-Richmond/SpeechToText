# Cleanup Prompt Golden Fixtures

This directory contains the regression test cases for Vox's cleanup system prompt. Every case is a directory with three files:

```
<id>_<description>/
├── input.txt      # Raw Whisper transcript (what STT spits out)
├── context.json   # App context + personal dictionary hints
└── reference.txt  # The ideal cleaned output we expect
```

## Scoring rules

A case **passes** if the model output:
1. Exactly matches `reference.txt` (after whitespace normalization), **or**
2. Has normalized edit distance ≤ 10% from `reference.txt` **and** introduces no tokens that weren't in `input.txt` or `reference.txt`.

Run the full suite: `python3 .claude/skills/vox-prompt-eval/scripts/score.py --results build/prompt-eval-results.json`

Add a new case: `bash .claude/skills/vox-prompt-eval/scripts/new-golden.sh <id_description>`

## Category map

| ID range | Category | Min before CI gates |
|----------|----------|---------------------|
| 001–019 | Filler removal | 10 |
| 020–039 | Punctuation + capitalization | 10 |
| 040–059 | Self-corrections | 10 |
| 060–079 | Numbered / bulleted lists | 5 |
| 080–099 | App-context formality | 5 |
| 100–119 | Dictionary hinting | 5 |
| 120–139 | Tone matching (Phase 2) | 10 |
| 140–159 | Command Mode rewrites | 10 |

## Large audio files

Actual `.wav` / `.m4a` files for benchmarking live in `audio/` (tracked via Git LFS). The text goldens here are the transcripts already extracted from those recordings — the prompt-eval harness works against text only, so LFS is not required for CI.
