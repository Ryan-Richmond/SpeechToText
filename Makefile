.PHONY: build test lint format xcodeproj build-apple test-apple privacy-audit session-start

build:
	swift build

test:
	swift test

format:
	@if command -v swiftformat >/dev/null 2>&1; then \
		swiftformat .; \
	else \
		echo "swiftformat not installed"; \
		exit 1; \
	fi

lint:
	@if command -v swiftlint >/dev/null 2>&1; then \
		swiftlint; \
	else \
		echo "swiftlint not installed"; \
		exit 1; \
	fi

xcodeproj:
	./scripts/bootstrap_xcodeproj.sh

build-apple:
	xcodebuild -project Vox.xcodeproj -scheme "Vox iOS" -destination '''platform=iOS Simulator,name=iPhone 16''' build
	xcodebuild -project Vox.xcodeproj -scheme "Vox macOS" -destination '''platform=macOS''' build

test-apple:
	xcodebuild -project Vox.xcodeproj -scheme "Vox macOS" -destination '''platform=macOS''' test

privacy-audit:
	./scripts/privacy_audit.sh

session-start:
	./scripts/session_start.sh
