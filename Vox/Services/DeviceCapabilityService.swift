import Foundation

struct DeviceCapabilityService: Sendable {

    enum Tier: String, Sendable {
        case ios = "ios"
        case macDefault = "mac-default"
        case macPower = "mac-power"
    }

    static func detectTier() -> Tier {
        let totalGB = Int(ProcessInfo.processInfo.physicalMemory / (1024 * 1024 * 1024))

        #if os(iOS)
        return .ios
        #else
        if totalGB >= 32 {
            return .macPower
        }
        return .macDefault
        #endif
    }

    static func totalRAMDescription() -> String {
        let gb = Double(ProcessInfo.processInfo.physicalMemory) / (1024 * 1024 * 1024)
        return String(format: "%.0f GB", gb)
    }

    static func meetsMinimumRequirement() -> Bool {
        let totalGB = Int(ProcessInfo.processInfo.physicalMemory / (1024 * 1024 * 1024))
        return totalGB >= 8
    }
}
