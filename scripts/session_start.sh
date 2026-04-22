#!/usr/bin/env bash
set -euo pipefail

check_tool() {
    local tool="$1"
    if command -v "$tool" >/dev/null 2>&1; then
        echo "[ok] $tool: $(command -v "$tool")"
    else
        echo "[missing] $tool"
    fi
}

echo "Vox session-start checks"
check_tool xcodebuild
check_tool swiftformat
check_tool swiftlint

available_kb=$(df -k . | awk 'NR==2 {print $4}')
available_gb=$((available_kb / 1024 / 1024))

echo "[info] disk available: ${available_gb}GB"
if [ "$available_gb" -lt 20 ]; then
    echo "[warn] less than 20GB available; model downloads may fail"
fi
