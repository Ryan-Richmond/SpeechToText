import XCTest
@testable import Vox

final class DeviceCapabilityTests: XCTestCase {

    func test_meetsMinimumRequirement_returnsTrue_onModernHardware() {
        // This test runs on CI (macOS runner ≥ 16 GB) and dev machines.
        // It would only fail on <8 GB hardware, which we don't target.
        XCTAssertTrue(DeviceCapabilityService.meetsMinimumRequirement())
    }

    func test_detectTier_returnsMacTier_onMacOS() {
        #if os(macOS)
        let tier = DeviceCapabilityService.detectTier()
        XCTAssertTrue(tier == .macDefault || tier == .macPower)
        #else
        let tier = DeviceCapabilityService.detectTier()
        XCTAssertEqual(tier, .ios)
        #endif
    }

    func test_totalRAMDescription_returnsNonEmptyString() {
        let desc = DeviceCapabilityService.totalRAMDescription()
        XCTAssertFalse(desc.isEmpty)
        XCTAssertTrue(desc.hasSuffix("GB"))
    }
}
