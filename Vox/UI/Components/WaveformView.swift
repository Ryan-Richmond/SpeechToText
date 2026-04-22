import SwiftUI

// MARK: - WaveformView

/// Animated waveform bars that respond to live RMS audio levels.
/// Used in the dictation overlay during recording.
public struct WaveformView: View {
    public let rmsLevel: Float
    public var barCount: Int = 9

    @State private var phases: [Double] = []

    public init(rmsLevel: Float, barCount: Int = 9) {
        self.rmsLevel = rmsLevel
        self.barCount = barCount
    }

    public var body: some View {
        HStack(spacing: 3) {
            ForEach(0..<barCount, id: \.self) { i in
                RoundedRectangle(cornerRadius: 2)
                    .fill(Color.voxPurple.opacity(0.9))
                    .frame(width: 3, height: barHeight(for: i))
                    .animation(
                        .easeInOut(duration: 0.08 + Double(i) * 0.01),
                        value: rmsLevel
                    )
            }
        }
    }

    private func barHeight(for index: Int) -> CGFloat {
        let base: CGFloat = 4
        let maxHeight: CGFloat = 28
        let centerFactor = 1.0 - abs(Double(index) - Double(barCount) / 2.0) / (Double(barCount) / 2.0)
        let rms = CGFloat(min(rmsLevel * 4, 1.0))  // amplify for visual
        let noise = CGFloat.random(in: 0.8...1.2)
        return base + (maxHeight - base) * rms * CGFloat(centerFactor) * noise
    }
}

// MARK: - ShimmerModifier

/// A horizontal shimmer animation for the processing state.
struct ShimmerModifier: ViewModifier {
    @State private var phase: CGFloat = -1

    func body(content: Content) -> some View {
        content
            .overlay(
                GeometryReader { geo in
                    LinearGradient(
                        gradient: Gradient(colors: [
                            .white.opacity(0),
                            .white.opacity(0.35),
                            .white.opacity(0)
                        ]),
                        startPoint: .leading,
                        endPoint: .trailing
                    )
                    .frame(width: geo.size.width * 0.5)
                    .offset(x: phase * geo.size.width * 1.5)
                    .animation(
                        .linear(duration: 1.2).repeatForever(autoreverses: false),
                        value: phase
                    )
                }
                .clipped()
            )
            .onAppear { phase = 1 }
    }
}

public extension View {
    func shimmer() -> some View {
        modifier(ShimmerModifier())
    }
}
