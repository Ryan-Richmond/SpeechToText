import VoxCore

@main
struct VoxCLI {
    static func main() async {
        let bootstrap = VoxBootstrap()
        let preview = await bootstrap.runDictationPreview(input: "  hello vox  ")
        print(preview)
    }
}
