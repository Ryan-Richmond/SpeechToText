import XCTest
@testable import Vox

final class ModelRegistryTests: XCTestCase {

    // This test requires registry.json to be in the test host's bundle.
    // If it fails, check that project.yml includes registry.json as a resource.

    func test_load_succeedsWithBundledRegistry() throws {
        let registry = try ModelRegistry.load()
        XCTAssertFalse(registry.models.isEmpty, "Registry should contain at least one model")
        XCTAssertFalse(registry.tiers.isEmpty, "Registry should contain at least one tier")
    }

    func test_defaultModels_ios_returnsE2B() throws {
        let registry = try ModelRegistry.load()
        let (stt, llm) = try registry.defaultModels(forTier: "ios")
        XCTAssertEqual(stt.kind, .stt)
        XCTAssertEqual(llm.kind, .llm)
        XCTAssertTrue(stt.filename.contains("small"))
        XCTAssertTrue(llm.filename.contains("E2B"))
    }

    func test_defaultModels_macDefault_returnsE4B() throws {
        let registry = try ModelRegistry.load()
        let (stt, llm) = try registry.defaultModels(forTier: "mac-default")
        XCTAssertEqual(stt.kind, .stt)
        XCTAssertEqual(llm.kind, .llm)
        XCTAssertTrue(stt.filename.contains("medium"))
        XCTAssertTrue(llm.filename.contains("E4B"))
    }

    func test_entry_throwsForUnknownModel() throws {
        let registry = try ModelRegistry.load()
        XCTAssertThrowsError(try registry.entry(for: "nonexistent-model"))
    }

    func test_allModels_haveValidURLs() throws {
        let registry = try ModelRegistry.load()
        for (id, entry) in registry.models {
            XCTAssertNotNil(URL(string: entry.url), "Model \(id) has invalid URL: \(entry.url)")
            XCTAssertFalse(entry.filename.isEmpty, "Model \(id) has empty filename")
            XCTAssertGreaterThan(entry.sizeBytes, 0, "Model \(id) has zero size")
            XCTAssertGreaterThan(entry.ramEstimateMb, 0, "Model \(id) has zero RAM estimate")
        }
    }
}
