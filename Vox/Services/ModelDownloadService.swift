import Foundation
import OSLog

actor ModelDownloadService {

    enum State: Sendable {
        case idle
        case downloading(progress: Double)
        case verifying
        case ready
        case failed(Error)
    }

    private let fileManager = FileManager.default

    // MARK: - Public

    func download(entry: ModelRegistryEntry, to directory: URL) async throws -> URL {
        let destination = directory.appendingPathComponent(entry.filename)

        if fileManager.fileExists(atPath: destination.path) {
            Log.download.info("Model already exists: \(entry.filename)")
            return destination
        }

        try fileManager.createDirectory(at: directory, withIntermediateDirectories: true)

        let partial = destination.appendingPathExtension("partial")

        Log.download.info("Downloading \(entry.filename) (\(entry.sizeBytes) bytes)")

        guard let remoteURL = URL(string: entry.url) else {
            throw DownloadError.invalidURL(entry.url)
        }

        var request = URLRequest(url: remoteURL)
        request.httpMethod = "GET"

        // Resume partial downloads.
        if fileManager.fileExists(atPath: partial.path),
           let attrs = try? fileManager.attributesOfItem(atPath: partial.path),
           let existingSize = attrs[.size] as? Int64 {
            request.setValue("bytes=\(existingSize)-", forHTTPHeaderField: "Range")
            Log.download.info("Resuming from byte \(existingSize)")
        }

        let (asyncBytes, response) = try await URLSession.shared.bytes(for: request)

        guard let httpResponse = response as? HTTPURLResponse,
              (200...299).contains(httpResponse.statusCode) || httpResponse.statusCode == 206 else {
            throw DownloadError.httpError((response as? HTTPURLResponse)?.statusCode ?? -1)
        }

        let totalBytes = httpResponse.expectedContentLength
        let handle = try FileHandle(forWritingTo: partial.ensureFileExists())
        handle.seekToEndOfFile()

        var bytesWritten: Int64 = 0
        let chunkSize = 1024 * 256
        var buffer = Data()
        buffer.reserveCapacity(chunkSize)

        for try await byte in asyncBytes {
            buffer.append(byte)
            if buffer.count >= chunkSize {
                handle.write(buffer)
                bytesWritten += Int64(buffer.count)
                buffer.removeAll(keepingCapacity: true)

                if totalBytes > 0 {
                    let progress = Double(bytesWritten) / Double(totalBytes)
                    Log.download.debug("Download progress: \(Int(progress * 100))%")
                }
            }
        }

        if !buffer.isEmpty {
            handle.write(buffer)
        }

        try handle.close()

        // SHA-256 verification.
        if entry.sha256 != "TBD" && !entry.sha256.isEmpty {
            Log.download.info("Verifying SHA-256 for \(entry.filename)")
            let actualHash = try sha256(of: partial)
            if actualHash != entry.sha256.lowercased() {
                try? fileManager.removeItem(at: partial)
                throw DownloadError.checksumMismatch(expected: entry.sha256, actual: actualHash)
            }
        }

        try fileManager.moveItem(at: partial, to: destination)
        Log.download.info("Download complete: \(entry.filename)")
        return destination
    }

    func modelDirectory() throws -> URL {
        #if os(iOS)
        guard let container = fileManager.containerURL(forSecurityApplicationGroupIdentifier: "group.llc.meridian.vox") else {
            throw DownloadError.noAppGroup
        }
        return container.appendingPathComponent("models", isDirectory: true)
        #else
        let appSupport = try fileManager.url(
            for: .applicationSupportDirectory,
            in: .userDomainMask,
            appropriateFor: nil,
            create: true
        )
        return appSupport
            .appendingPathComponent("llc.meridian.vox", isDirectory: true)
            .appendingPathComponent("models", isDirectory: true)
        #endif
    }

    func isModelDownloaded(entry: ModelRegistryEntry) throws -> Bool {
        let dir = try modelDirectory()
        let path = dir.appendingPathComponent(entry.filename)
        return fileManager.fileExists(atPath: path.path)
    }

    func modelPath(for entry: ModelRegistryEntry) throws -> URL {
        let dir = try modelDirectory()
        return dir.appendingPathComponent(entry.filename)
    }

    func diskSpaceAvailableGB() -> Double? {
        guard let attrs = try? fileManager.attributesOfFileSystem(forPath: NSHomeDirectory()),
              let freeBytes = attrs[.systemFreeSize] as? Int64 else {
            return nil
        }
        return Double(freeBytes) / (1024 * 1024 * 1024)
    }

    // MARK: - Private

    private func sha256(of fileURL: URL) throws -> String {
        let handle = try FileHandle(forReadingFrom: fileURL)
        defer { try? handle.close() }
        var hasher = SHA256Hasher()
        while autoreleasepool(invoking: {
            let chunk = handle.readData(ofLength: 1024 * 1024)
            if chunk.isEmpty { return false }
            hasher.update(chunk)
            return true
        }) {}
        return hasher.finalize()
    }
}

// Minimal SHA-256 using CommonCrypto (available on Apple platforms).
import CommonCrypto

private struct SHA256Hasher {
    private var context = CC_SHA256_CTX()

    init() {
        CC_SHA256_Init(&context)
    }

    mutating func update(_ data: Data) {
        data.withUnsafeBytes { buffer in
            _ = CC_SHA256_Update(&context, buffer.baseAddress, CC_LONG(buffer.count))
        }
    }

    mutating func finalize() -> String {
        var digest = [UInt8](repeating: 0, count: Int(CC_SHA256_DIGEST_LENGTH))
        CC_SHA256_Final(&digest, &context)
        return digest.map { String(format: "%02x", $0) }.joined()
    }
}

private extension URL {
    func ensureFileExists() throws -> URL {
        if !FileManager.default.fileExists(atPath: path) {
            FileManager.default.createFile(atPath: path, contents: nil)
        }
        return self
    }
}

enum DownloadError: Error, LocalizedError {
    case invalidURL(String)
    case httpError(Int)
    case checksumMismatch(expected: String, actual: String)
    case noAppGroup

    var errorDescription: String? {
        switch self {
        case .invalidURL(let url): "Invalid download URL: \(url)"
        case .httpError(let code): "Download failed with HTTP \(code)"
        case .checksumMismatch(let expected, let actual):
            "SHA-256 mismatch. Expected \(expected.prefix(12))..., got \(actual.prefix(12))..."
        case .noAppGroup: "App Group container not configured."
        }
    }
}
