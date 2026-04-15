#!/usr/bin/env python3
"""
parse.py — parse xcodebuild benchmark output (NDJSON lines) into a markdown report.

Usage:
    python3 parse.py \
        --input build/bench-raw.txt \
        --device "M1 Mac 16GB" \
        --models "Whisper medium.en, Gemma 4 E4B Q4_K_M" \
        --output docs/benchmarks/model-bench-20260414.md

NDJSON line schema (emitted by VoxTests/Benchmarks):
    {"id": "001_clean_speech", "wer": 0.032, "latency_ms": 1724, "peak_rss_mb": 4312}
"""
from __future__ import annotations

import argparse
import json
import statistics
import sys
from datetime import date
from pathlib import Path
from typing import Any


TARGETS: dict[str, dict[str, dict[str, float]]] = {
    "ios": {
        "wer_pct":     {"target": 5.0,    "better": "lower"},
        "latency_ms":  {"target": 3000.0, "better": "lower"},
        "peak_rss_mb": {"target": 1900.0, "better": "lower"},
    },
    "mac": {
        "wer_pct":     {"target": 4.0,    "better": "lower"},
        "latency_ms":  {"target": 2000.0, "better": "lower"},
        "peak_rss_mb": {"target": 5000.0, "better": "lower"},
    },
}


def pct(v: float) -> str:
    return f"{v:.1f}%"


def ms(v: float) -> str:
    return f"{v:.0f} ms"


def mb(v: float) -> str:
    return f"{v:.0f} MB"


def pass_fail(value: float, target: float, better: str) -> str:
    ok = value <= target if better == "lower" else value >= target
    return "✓" if ok else "✗ OVER TARGET"


def percentile(data: list[float], p: int) -> float:
    if not data:
        return 0.0
    data_s = sorted(data)
    k = (len(data_s) - 1) * p / 100
    lo, hi = int(k), min(int(k) + 1, len(data_s) - 1)
    return data_s[lo] + (data_s[hi] - data_s[lo]) * (k - lo)


def parse_rows(path: Path) -> list[dict[str, Any]]:
    rows = []
    for line in path.read_text().splitlines():
        line = line.strip()
        if line.startswith("{"):
            try:
                rows.append(json.loads(line))
            except json.JSONDecodeError:
                pass
    return rows


def render(rows: list[dict[str, Any]], device: str, models: str, platform: str) -> str:
    today = date.today().isoformat()
    t = TARGETS.get(platform, TARGETS["mac"])

    wers        = [r["wer"] * 100 for r in rows]
    latencies   = [r["latency_ms"] for r in rows]
    rss_values  = [r["peak_rss_mb"] for r in rows]

    p50_wer   = percentile(wers, 50)
    p95_wer   = percentile(wers, 95)
    p50_lat   = percentile(latencies, 50)
    p95_lat   = percentile(latencies, 95)
    p50_rss   = percentile(rss_values, 50)
    max_rss   = max(rss_values, default=0.0)

    lines = [
        f"# Model Bench — {today} — {device}",
        "",
        "## Configuration",
        "",
        f"- **Device:** {device}",
        f"- **Models:** {models}",
        f"- **Platform target:** {platform}",
        f"- **Clips evaluated:** {len(rows)}",
        "",
        "## Results",
        "",
        "| Metric | p50 | p95 | Target | Pass? |",
        "|--------|-----|-----|--------|-------|",
        f"| WER | {pct(p50_wer)} | {pct(p95_wer)} | ≤ {pct(t['wer_pct']['target'])} | {pass_fail(p50_wer, t['wer_pct']['target'], 'lower')} |",
        f"| Latency | {ms(p50_lat)} | {ms(p95_lat)} | ≤ {ms(t['latency_ms']['target'])} | {pass_fail(p50_lat, t['latency_ms']['target'], 'lower')} |",
        f"| Peak RSS (p50) | {mb(p50_rss)} | {mb(max_rss)} (max) | ≤ {mb(t['peak_rss_mb']['target'])} | {pass_fail(max_rss, t['peak_rss_mb']['target'], 'lower')} |",
        "",
        "## Per-clip detail",
        "",
        "| ID | WER | Latency (ms) | Peak RSS (MB) |",
        "|----|-----|-------------|---------------|",
    ]
    for r in rows:
        lines.append(
            f"| {r['id']} | {pct(r['wer'] * 100)} | {r['latency_ms']:.0f} | {r['peak_rss_mb']:.0f} |"
        )

    all_pass = all([
        p50_wer  <= t["wer_pct"]["target"],
        p50_lat  <= t["latency_ms"]["target"],
        max_rss  <= t["peak_rss_mb"]["target"],
    ])
    lines += ["", f"## Overall: {'PASS' if all_pass else 'FAIL'}"]
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--input",   type=Path, required=True)
    ap.add_argument("--device",  required=True)
    ap.add_argument("--models",  required=True)
    ap.add_argument("--platform", choices=["ios", "mac"], default="mac")
    ap.add_argument("--output",  type=Path, required=True)
    args = ap.parse_args()

    rows = parse_rows(args.input)
    if not rows:
        print("error: no NDJSON rows found in input file", file=sys.stderr)
        return 1

    report = render(rows, args.device, args.models, args.platform)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(report)
    print(f"Wrote {args.output} ({len(rows)} clips)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
