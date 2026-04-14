#!/usr/bin/env bash
set -euo pipefail

violations=0
patterns='URLSession|Telemetry|Analytics|Firebase|Mixpanel|Amplitude'

if rg -n "$patterns" Vox Sources Tests >/dev/null 2>&1; then
    echo "Potential privacy-sensitive references found:"
    rg -n "$patterns" Vox Sources Tests || true
    violations=1
fi

if [ "$violations" -ne 0 ]; then
    echo "Privacy audit failed"
    exit 1
fi

echo "Privacy audit passed"
