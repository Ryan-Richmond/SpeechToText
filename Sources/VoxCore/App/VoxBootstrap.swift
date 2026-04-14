public struct VoxBootstrap {
    private let pipelineActor: PipelineActor

    public init(pipelineActor: PipelineActor = PipelineActor()) {
        self.pipelineActor = pipelineActor
    }

    public func runDictationPreview(input: String) async -> String {
        await pipelineActor.dictate(transcript: input)
    }
}
