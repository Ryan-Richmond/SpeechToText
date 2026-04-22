#!/usr/bin/env bash
set -euo pipefail

violations=0

# Analytics/telemetry SDKs are never permitted anywhere
analytics_patterns='Telemetry|Analytics|Firebase|Mixpanel|Amplitude'
if rg -n "$analytics_patterns" Vox Sources Tests >/dev/null 2>&1; then
    echo "Potential analytics/telemetry references found:"
    rg -n "$analytics_patterns" Vox Sources Tests || true
    violations=1
fi

# URLSession is only permitted in ModelDownloadService for model file downloads.
# Any other use risks sending user data off-device.
if rg -n 'URLSession' --glob '!**/ModelDownloadService.swift' Vox Sources Tests >/dev/null 2>&1; then
    echo "Unexpected URLSession usage (only ModelDownloadService may use URLSession for model downloads):"
    rg -n 'URLSession' --glob '!**/ModelDownloadService.swift' Vox Sources Tests || true
    violations=1
fi

if [ "$violations" -ne 0 ]; then
    echo "Privacy audit failed"
    exit 1
fi

echo "Privacy audit passed"
