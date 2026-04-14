// swift-tools-version: 6.0
import PackageDescription

let package = Package(
    name: "Vox",
    platforms: [
        .macOS(.v15),
        .iOS(.v18)
    ],
    products: [
        .library(name: "VoxCore", targets: ["VoxCore"]),
        .executable(name: "VoxCLI", targets: ["VoxCLI"])
    ],
    targets: [
        .target(
            name: "VoxCore"
        ),
        .executableTarget(
            name: "VoxCLI",
            dependencies: ["VoxCore"]
        ),
        .testTarget(
            name: "VoxCoreTests",
            dependencies: ["VoxCore"]
        )
    ]
)
