import SwiftUI

// MARK: - Design System Colors

public extension Color {
    // Brand
    static let voxPurple      = Color(hue: 0.74, saturation: 0.78, brightness: 0.92)
    static let voxPurpleDark  = Color(hue: 0.74, saturation: 0.85, brightness: 0.55)
    static let voxBlue        = Color(hue: 0.61, saturation: 0.72, brightness: 0.95)

    // State colors
    static let voxRecording   = Color(hue: 0.0,  saturation: 0.82, brightness: 0.90)
    static let voxProcessing  = Color(hue: 0.61, saturation: 0.80, brightness: 0.95)
    static let voxDone        = Color(hue: 0.35, saturation: 0.75, brightness: 0.85)
    static let voxIdle        = Color.primary.opacity(0.08)

    // Overlay
    static let overlayBackground = Color.black.opacity(0.75)
}

// MARK: - Typography

public extension Font {
    static let voxTitle     = Font.system(size: 28, weight: .bold, design: .rounded)
    static let voxHeadline  = Font.system(size: 17, weight: .semibold, design: .rounded)
    static let voxBody      = Font.system(size: 15, weight: .regular, design: .rounded)
    static let voxCaption   = Font.system(size: 12, weight: .medium, design: .rounded)
    static let voxMono      = Font.system(size: 13, weight: .regular, design: .monospaced)
}

// MARK: - Spacing

public enum Spacing {
    public static let xs: CGFloat = 4
    public static let sm: CGFloat = 8
    public static let md: CGFloat = 16
    public static let lg: CGFloat = 24
    public static let xl: CGFloat = 40
}

// MARK: - Corner Radius

public enum CornerRadius {
    public static let pill: CGFloat  = 999
    public static let card: CGFloat  = 16
    public static let small: CGFloat = 8
}
