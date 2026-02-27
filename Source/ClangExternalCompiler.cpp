#include "ClangExternalCompiler.h"
#include <JuceHeader.h>
#include <filesystem>
#include <fstream>
#include "WaviateInput.h"

void ClangExternalCompiler::compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency) {
    // Initialize output to nullptr
    outSample = nullptr;
    outFrequency = nullptr;

    try {
        // Step 1: Prepend include directive
        std::string sourceWithInclude = "#include \"WaviateInput.h\"\n\n" + source;

        // Step 2: Get temp directory and create source file
        std::filesystem::path tempDir = std::filesystem::temp_directory_path();
        std::filesystem::path sourceFile = tempDir / "waviate_shader_temp.c";
        
        // Determine output file extension based on platform
        std::string outputExtension;
#if JUCE_WINDOWS
        outputExtension = ".dll";
#elif JUCE_MAC
        outputExtension = ".dylib";
#else
        outputExtension = ".so";
#endif
        std::filesystem::path outputFile = tempDir / ("waviate_shader_temp" + outputExtension);

        // Write source to temp file
        {
            std::ofstream file(sourceFile);
            if (!file.is_open()) {
                return;
            }
            file << sourceWithInclude;
        }

        // Step 3: Build clang compile command
        std::string compileCmd = "clang -shared -o \"" + outputFile.string() + "\" \"" + sourceFile.string() + "\"";

        // Step 4: Execute compilation
        int result = std::system(compileCmd.c_str());
        if (result != 0) {
            // Compilation failed
            std::filesystem::remove(sourceFile);
            return;
        }

        // Step 5: Load compiled library using JUCE's DynamicLibrary
        loadedLibrary = std::make_unique<juce::DynamicLibrary>();

        if (!loadedLibrary->open(outputFile.string()))
        {
            loadedLibrary.reset();
            return;
        }


        // Step 6: Look up functions
        SampleShader sampleFunc = (SampleShader)loadedLibrary->getFunction("sample_process");
        FrequencyShader frequencyFunc = (FrequencyShader)loadedLibrary->getFunction("frequency_process");

        // Step 7: Set output pointers
        if (sampleFunc) {
            outSample = sampleFunc;
        }
        if (frequencyFunc) {
            outFrequency = frequencyFunc;
        }

        // Clean up temp source file
        std::filesystem::remove(sourceFile);
        // Note: Keep the library loaded (library object stays in scope) so functions remain accessible

    } catch (...) {
        // If any exception occurs, ensure outputs are nullptrs
        outSample = nullptr;
        outFrequency = nullptr;
    }
}