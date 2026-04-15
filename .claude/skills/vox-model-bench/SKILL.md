---
name: vox-model-bench
description: Benchmark a Whisper or Gemma 4 model configuration on the current device. Reports WER, p50/p95 latency, peak RAM, and battery drain. Writes results to docs/benchmarks/. Run after any model change and before every release. Requires real hardware (not Simulator).
---

# vox-model-bench

Benchmarks are first-class artifacts in Vox. Every claim in the README ("< 2s latency," "< 5GB RAM") must be backed by a benchmark report in `docs/benchmarks/`.

## When to run

- After changing the default STT or LLM model.
- After a quantization change (Q4_K_M → Q8_0, etc.).
- After a `whisper.cpp` or `llama.cpp` version bump.
- Before every phase release.
- When investigating a user complaint about slowness or crashes.

## Requirements

- Real hardware (iPhone 16 or M-series Mac) — Simulator has no Metal, no Neural Engine.
- Xcode with `Vox.Benchmarks` test target (added in Sprint 5).
- `Instruments.app` for the RAM + battery trace (optional but recommended for release runs).
- 20+ reference audio clips from `VoxTests/Fixtures/audio/` (tracked in Git LFS).

## How to run

```bash
# 1. Run the Swift benchmark target
xcodebuild test \
  -scheme Vox \
  -only-testing:VoxTests/Benchmarks \
  -destination 'platform=macOS'  # or iOS device
  2>&1 | tee build/bench-raw.txt

# 2. Parse results and write markdown report
python3 .claude/skills/vox-model-bench/scripts/parse.py \
  --input build/bench-raw.txt \
  --device "M1 Mac 16GB" \
  --models "Whisper medium.en, Gemma 4 E4B Q4_K_M" \
  --output docs/benchmarks/model-bench-$(date +%Y%m%d).md

# 3. Commit the report
git add docs/benchmarks/
git commit -m "perf: model bench $(date +%Y%m%d) — <one-line summary of change>"
```

## What the benchmark target measures

The `VoxTests/Benchmarks` Swift target:

1. Iterates over all `.wav` files in `VoxTests/Fixtures/audio/` (must have paired `*.txt` reference transcripts).
2. For each clip, runs the full pipeline: `AudioCapture` → `STTEngine.transcribe` → `LLMEngine.generate`.
3. Records:
   - **WER** — standard word error rate vs the reference transcript.
   - **Latency** — wall-clock time from first sample to cleaned text (excludes clipboard paste).
   - **Peak RSS** — read from `mach_task_basic_info` at the inference actor.
4. Emits NDJSON to stdout: one line per clip, fields: `{id, wer, latency_ms, peak_rss_mb}`.

## Interpreting results

| Metric | iPhone target (p50) | Mac target (p50) |
|--------|---------------------|------------------|
| WER | ≤ 5% | ≤ 4% |
| Latency | ≤ 3,000 ms | ≤ 2,000 ms |
| Peak RAM | ≤ 1,900 MB | ≤ 5,000 MB |
| Battery (30-min session) | ≤ 5% drain | N/A |

A result above target is a **blocking issue** before release. File it as a GitHub Issue with the benchmark report attached.

## Battery measurement (iOS only)

Battery drain can only be measured on real hardware, not via code. Procedure:

1. Fully charge device.
2. Note battery % in Settings.
3. Run 30 minutes of continuous dictation (use `scripts/stress-dictate.sh` — loops the eval clips).
4. Note final battery %.
5. Record `(start - end)` in the benchmark report.

## Adding audio fixtures

```bash
# Convert a recording to the expected format (16kHz mono WAV)
ffmpeg -i recording.m4a -ar 16000 -ac 1 VoxTests/Fixtures/audio/021_accented_speech.wav

# Add paired reference transcript
echo "the quick brown fox jumped over the lazy dog" \
  > VoxTests/Fixtures/audio/021_accented_speech.txt

# Track in LFS
git lfs track "VoxTests/Fixtures/audio/*.wav"
git add VoxTests/Fixtures/audio/021_accented_speech.wav \
        VoxTests/Fixtures/audio/021_accented_speech.txt
```

Aim for a diverse eval set: clean speech, background noise, accented speech, fast speech, quiet speech, technical jargon, long utterances (> 30s).

## Output report format

See `docs/benchmarks/README.md` for the markdown schema. Always commit reports.
