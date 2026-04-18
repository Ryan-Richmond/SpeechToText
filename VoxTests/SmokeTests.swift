import XCTest
@testable import Vox

/// Sprint 0 smoke test: prove the test target builds and links against Vox.
final class SmokeTests: XCTestCase {

    func test_logSubsystemConfigured() {
        // The Logger instances are static, so this simply forces the type to
        // initialize. If the build is broken or symbols are missing, this
        // file won't compile.
        XCTAssertNotNil(Log.pipeline)
    }
}
