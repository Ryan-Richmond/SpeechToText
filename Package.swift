// swift-tools-version: 6.0
import PackageDescription

let package = Package(
    name: "Vox",
    platforms: [
        .macOS(.v15),
        .iOS(.v18)
    ],
    products: [
        .library(name: "VoxCore", targets: ["VoxCore"])
    ],
    dependencies: [
        .package(path: "Packages/whisper"),
        .package(url: "https://github.com/mattt/llama.swift", branch: "main")
    ],
    targets: [
        .target(
            name: "VoxCore",
            dependencies: [
                .product(name: "whisper", package: "whisper"),
                .product(name: "LlamaSwift", package: "llama.swift")
            ]
        ),
        .testTarget(
            name: "VoxCoreTests",
            dependencies: ["VoxCore"]
        )
    ]
)
