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

build-apple: xcodeproj
	xcodebuild -project Vox.xcodeproj -scheme "Vox-iOS" -destination 'generic/platform=iOS Simulator' CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO build
	xcodebuild -project Vox.xcodeproj -scheme "Vox-macOS" -destination 'platform=macOS' CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO build

test-apple: xcodeproj
	xcodebuild -project Vox.xcodeproj -scheme "Vox-macOS" -destination 'platform=macOS' CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO test

privacy-audit:
	./scripts/privacy_audit.sh

session-start:
	./scripts/session_start.sh
