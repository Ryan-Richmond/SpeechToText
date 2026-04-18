#!/usr/bin/env bash
# Vox SessionStart hook.
# Runs when a Claude Code session starts in this repo. Reports toolchain
# readiness so the agent (and you) know what's missing before writing code.
#
# Wire this up in .claude/settings.json (already configured).

set -uo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

echo "Vox session — toolchain check"
echo "─────────────────────────────"

check() {
    local name="$1"
    local cmd="$2"
    if command -v "$cmd" >/dev/null 2>&1; then
        local ver
        ver=$($cmd --version 2>&1 | head -n1 || echo "?")
        printf "  %-14s ✓  %s\n" "$name" "$ver"
    else
        printf "  %-14s ✗  not found  (run: make setup)\n" "$name"
    fi
}

check xcodebuild   xcodebuild
check xcodegen     xcodegen
check swiftformat  swiftformat
check swiftlint    swiftlint
check jq           jq
check curl         curl
check python3      python3

# Disk space (matters for model downloads — Mac tier needs ~3.75 GB)
echo ""
if command -v df >/dev/null 2>&1; then
    avail=$(df -Pg . 2>/dev/null | awk 'NR==2 {print $4}' || echo "?")
    echo "  Free disk:    ${avail} GB on $(pwd)"
fi

# Dev models present?
if [[ -d .dev-models ]]; then
    count=$(find .dev-models -maxdepth 1 -type f \( -name '*.gguf' -o -name '*.bin' \) 2>/dev/null | wc -l | tr -d ' ')
    echo "  Dev models:   $count file(s) in .dev-models/"
else
    echo "  Dev models:   not downloaded (run: make fetch-models)"
fi

# Xcode project status
if [[ -d Vox.xcodeproj ]]; then
    echo "  Xcode project: present"
else
    echo "  Xcode project: not generated (run: make generate)"
fi

echo ""
echo "Working branch: $(git branch --show-current 2>/dev/null || echo '(detached)')"
echo "Tip: run 'make help' for the full command list."
