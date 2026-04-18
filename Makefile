# Vox — common engineering commands.
# All commands assume macOS + Xcode 16+ unless otherwise noted.

XCODE_PROJECT := Vox.xcodeproj
SCHEME_MAC    := Vox
SCHEME_IOS    := VoxIOS
DESTINATION_MAC := platform=macOS,arch=arm64
DESTINATION_IOS := platform=iOS Simulator,name=iPhone 16

.PHONY: help generate build build-mac build-ios test lint format \
        privacy-audit prompt-eval entitlement-diff fetch-models \
        clean clean-models setup all

help:
	@echo "Vox — engineering commands"
	@echo ""
	@echo "  make setup            One-time: install xcodegen, swiftformat, swiftlint, jq"
	@echo "  make generate         Regenerate Vox.xcodeproj from project.yml"
	@echo "  make build            Build both iOS and macOS"
	@echo "  make build-mac        Build macOS only"
	@echo "  make build-ios        Build iOS Simulator only"
	@echo "  make test             Run unit tests (macOS)"
	@echo "  make lint             swiftlint"
	@echo "  make format           swiftformat (writes changes)"
	@echo "  make privacy-audit    Run vox-privacy-audit"
	@echo "  make prompt-eval      Run vox-prompt-eval scorer (requires results.json)"
	@echo "  make entitlement-diff Run vox-entitlement-diff vs last tag"
	@echo "  make fetch-models     Download dev model weights to .dev-models/"
	@echo "  make clean            Remove build artifacts"
	@echo "  make clean-models     Remove .dev-models/"
	@echo "  make all              format + lint + build + test + privacy-audit"

setup:
	@command -v brew >/dev/null || { echo "Install Homebrew first: https://brew.sh"; exit 1; }
	brew install xcodegen swiftformat swiftlint jq
	@echo "Setup complete. Run 'make generate' next."

generate:
	xcodegen generate

build: build-mac build-ios

build-mac: $(XCODE_PROJECT)
	xcodebuild -project $(XCODE_PROJECT) -scheme $(SCHEME_MAC) \
		-destination "$(DESTINATION_MAC)" build

build-ios: $(XCODE_PROJECT)
	xcodebuild -project $(XCODE_PROJECT) -scheme $(SCHEME_IOS) \
		-destination "$(DESTINATION_IOS)" build

test: $(XCODE_PROJECT)
	xcodebuild -project $(XCODE_PROJECT) -scheme $(SCHEME_MAC) \
		-destination "$(DESTINATION_MAC)" test

lint:
	swiftlint --strict

format:
	swiftformat .

privacy-audit:
	bash .claude/skills/vox-privacy-audit/scripts/audit.sh

prompt-eval:
	@test -f build/prompt-eval-results.json || \
		{ echo "Run the Swift PromptEvalTests target first to produce build/prompt-eval-results.json"; exit 1; }
	python3 .claude/skills/vox-prompt-eval/scripts/score.py \
		--results build/prompt-eval-results.json --threshold 0.85

entitlement-diff:
	bash .claude/skills/vox-entitlement-diff/scripts/diff.sh

fetch-models:
	bash scripts/fetch-dev-models.sh

clean:
	rm -rf build DerivedData .build

clean-models:
	rm -rf .dev-models

all: format lint build test privacy-audit

# Auto-generate the xcodeproj if missing
$(XCODE_PROJECT): project.yml
	xcodegen generate
