#!/usr/bin/env bash
set -euo pipefail

if ! command -v xcodegen >/dev/null 2>&1; then
    echo "xcodegen is required to generate Vox.xcodeproj from project.yml"
    echo "Install: brew install xcodegen"
    exit 1
fi

xcodegen generate
