import Testing
@testable import VoxCore

struct PipelineActorTests {
    @Test
    func dictateTrimsLeadingAndTrailingWhitespace() async {
        let actor = PipelineActor()
        let output = await actor.dictate(transcript: "  hello world  ")

        #expect(output == "hello world")
    }

    @Test
    func dictateReturnsEmptyStringForWhitespaceOnlyInput() async {
        let actor = PipelineActor()
        let output = await actor.dictate(transcript: "  \n \t ")

        #expect(output.isEmpty)
    }
}
