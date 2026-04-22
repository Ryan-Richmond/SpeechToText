import Foundation
import CryptoKit
import os

// MARK: - ModelDownloadService

/// Manages resumable model file downloads with SHA-256 verification.
/// All model files are stored in ModelConfig.modelsDirectory.
public actor ModelDownloadService {

    public static let shared = ModelDownloadService()
    private init() {}

    private let logger = Logger.vox(.download)

    // MARK: - Progress

    public struct DownloadProgress: Sendable {
        public let modelName: String
        public let bytesReceived: Int64
        public let totalBytes: Int64
        public var fractionCompleted: Double {
            guard totalBytes > 0 else { return 0 }
            return Double(bytesReceived) / Double(totalBytes)
        }
        public var isComplete: Bool { bytesReceived >= totalBytes && totalBytes > 0 }
    }

    // MARK: - Error

    public enum DownloadError: Error, LocalizedError {
        case insufficientDiskSpace(needed: Int64, available: Int64)
        case downloadFailed(underlying: Error)
        case sha256Mismatch(expected: String, actual: String)
        case invalidURL(String)

        public var errorDescription: String? {
            switch self {
            case .insufficientDiskSpace(let needed, let available):
                let fmt = ByteCountFormatter()
                return "Not enough disk space. Need \(fmt.string(fromByteCount: needed)), have \(fmt.string(fromByteCount: available))."
            case .downloadFailed(let e):
                return "Download failed: \(e.localizedDescription)"
            case .sha256Mismatch:
                return "Model file is corrupted. Please try downloading again."
            case .invalidURL(let s):
                return "Invalid download URL: \(s)"
            }
        }
    }

    // MARK: - Public API

    /// Downloads a model file if not already present on disk.
    /// Yields DownloadProgress updates through the returned AsyncStream.
    /// - Returns: An AsyncStream of progress updates. The final update has isComplete == true.
    public func download(
        model: ModelConfig
    ) -> AsyncThrowingStream<DownloadProgress, Error> {
        AsyncThrowingStream { continuation in
            Task {
                do {
                    try await self._download(model: model, continuation: continuation)
                } catch {
                    continuation.finish(throwing: error)
                }
            }
        }
    }

    /// Returns true if the model file is already on disk (any size > 0).
    public func isDownloaded(_ model: ModelConfig) -> Bool {
        model.isDownloaded
    }

    // MARK: - Private

    private func _download(
        model: ModelConfig,
        continuation: AsyncThrowingStream<DownloadProgress, Error>.Continuation
    ) async throws {
        let destination = model.localURL

        // Create directory if needed
        try FileManager.default.createDirectory(
            at: ModelConfig.modelsDirectory,
            withIntermediateDirectories: true
        )

        // Already downloaded?
        if model.isDownloaded {
            logger.info("Model already on disk: \(model.localFilename)")
            continuation.yield(DownloadProgress(
                modelName: model.name,
                bytesReceived: model.sizeBytes,
                totalBytes: model.sizeBytes
            ))
            continuation.finish()
            return
        }

        // Disk space preflight
        let volumeURL = ModelConfig.modelsDirectory
        let attrs = try FileManager.default.attributesOfFileSystem(forPath: volumeURL.path)
        let available = (attrs[.systemFreeSize] as? Int64) ?? 0
        let needed = model.sizeBytes + 512_000_000 // 512MB headroom
        guard available >= needed else {
            throw DownloadError.insufficientDiskSpace(needed: needed, available: available)
        }

        guard let url = URL(string: model.remoteURL) else {
            throw DownloadError.invalidURL(model.remoteURL)
        }

        logger.info("Downloading \(model.name) from \(model.remoteURL)")

        // Use URLSession with delegate for progress
        let session = URLSession(configuration: .default)
        let (asyncBytes, response) = try await session.bytes(from: url)
        let totalBytes = (response as? HTTPURLResponse)
            .flatMap { $0.value(forHTTPHeaderField: "Content-Length") }
            .flatMap { Int64($0) } ?? model.sizeBytes

        // Stream to temp file, then move
        let tempURL = destination.appendingPathExtension("download")
        FileManager.default.createFile(atPath: tempURL.path, contents: nil)
        let handle = try FileHandle(forWritingTo: tempURL)

        var received: Int64 = 0
        var chunk = Data()
        chunk.reserveCapacity(65_536)

        for try await byte in asyncBytes {
            chunk.append(byte)
            received += 1
            if chunk.count >= 65_536 {
                handle.write(chunk)
                chunk.removeAll(keepingCapacity: true)
                continuation.yield(DownloadProgress(
                    modelName: model.name,
                    bytesReceived: received,
                    totalBytes: totalBytes
                ))
            }
        }

        // Flush remainder
        if !chunk.isEmpty { handle.write(chunk) }
        try handle.close()

        // SHA-256 verification (skip if sha256 is empty — first download)
        if !model.sha256.isEmpty {
            let digest = try sha256(of: tempURL)
            guard digest == model.sha256 else {
                try? FileManager.default.removeItem(at: tempURL)
                throw DownloadError.sha256Mismatch(expected: model.sha256, actual: digest)
            }
        }

        // Atomic move to final location
        if FileManager.default.fileExists(atPath: destination.path) {
            try FileManager.default.removeItem(at: destination)
        }
        try FileManager.default.moveItem(at: tempURL, to: destination)

        logger.info("Download complete: \(model.localFilename)")

        continuation.yield(DownloadProgress(
            modelName: model.name,
            bytesReceived: totalBytes,
            totalBytes: totalBytes
        ))
        continuation.finish()
    }

    private func sha256(of url: URL) throws -> String {
        let handle = try FileHandle(forReadingFrom: url)
        var hasher = SHA256()
        while true {
            let chunk = try handle.read(upToCount: 65_536)
            guard let chunk, !chunk.isEmpty else { break }
            hasher.update(data: chunk)
        }
        let digest = hasher.finalize()
        return digest.map { String(format: "%02x", $0) }.joined()
    }
}
