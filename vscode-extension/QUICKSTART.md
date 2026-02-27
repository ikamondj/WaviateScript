# Quick Start Guide - Waviate Script VS Code Extension

## Installation

### From GitHub
```bash
cd vscode-extension
npm install
```

### From VSIX Package
```bash
npm install -g @vscode/vsce
vsce package
# Then in VS Code: Extensions → Install from VSIX
```

## Usage Examples

### 1. Create Your First Shader
```bash
# Create a new file with .wc extension
touch my_filter.wc
```

In VS Code, you now have:
- ✅ C syntax highlighting
- ✅ Waviate API autocomplete
- ✅ Code snippets (type `waviate_sample`)

### 2. Use a Snippet
Type: `waviate_sample` then press **Ctrl+Space**

Result:
```c
float sample_process(const WaviateSampleInput* in, WaviateSampleStateWriter* out) {
    // Process one audio sample
    float input = in->inputDeviceSamples[in->outputChannel][in->positionInBlock];
    float output = input; // Your processing here
    return output;
}
```

### 3. Use Autocomplete
Start typing: `in->midi`

You'll see autocomplete suggestions:
- `midiNoteOn` - MIDI note states
- `midiCCValue` - MIDI CC values
- `midiSegmentSize` - MIDI block size

### 4. View Documentation
Hover over any Waviate API symbol:
- `WaviateSampleInput` - Full struct documentation
- `midiNoteOn` - Field description
- `sample_process` - Function signature

### 5. IntelliSense Generate
When you open a `.wc` or `.wcpp` file:
- Extension automatically creates `.vscode/c_cpp_properties.json`
- IntelliSense automatically picks up Waviate API
- No manual configuration needed!

## Complete Example: Simple Pass-Through Shader

Create `passthrough.wc`:

```c
#include "WaviateInput.h"

float sample_process(const WaviateSampleInput* in, WaviateSampleStateWriter* out) {
    // Get input sample
    float input = in->inputDeviceSamples[in->outputChannel][in->positionInBlock];
    
    // Pass through unchanged
    return input;
}
```

Features automatically enabled:
- ✅ Syntax highlighting for all C keywords
- ✅ IntelliSense on `in->` suggests all fields
- ✅ Hover on fields shows documentation
- ✅ Line numbers and code folding
- ✅ Bracket matching

## Complete Example: MIDI-Controlled Gain

Create `midi_gain.wc`:

```c
float sample_process(const WaviateSampleInput* in, WaviateSampleStateWriter* out) {
    // Check if MIDI note 60 is being held
    if (in->midiNoteOn[60]) {
        // Get input
        float input = in->inputDeviceSamples[in->outputChannel][in->positionInBlock];
        
        // Get CC value (0-127) and normalize to 0.0-1.0
        float gain = in->midiCCValue[7] / 127.0f;  // CC 7 is main volume
        
        // Apply gain
        return input * gain;
    }
    
    return 0.0f;  // Mute when note off
}
```

Try typing to see:
- Autocomplete for `midiNoteOn` and `midiCCValue`
- Documentation on hover showing field descriptions
- No compilation errors even though we didn't #include WaviateInput.h!

## File Types

| Extension | Language | Use Case |
|-----------|----------|----------|
| `.wc` | C | Sample-wise processing, simple DSP |
| `.wcpp` | C++ | Advanced DSP with STL, object-oriented |
| `.wrs` | Rust | Advanced DSP with Rust safety guarantees |

## Tips & Tricks

### Snippet Shorthand
- `waviate_sample` → Full sample processing template
- `input_samples` → Access `in->inputDeviceSamples[ch][samp]`
- `sample_pos` → Get current position in block
- `midi_note` → Check MIDI note state
- `midi_cc` → Get MIDI CC value
- `sample_rate` → Get audio sample rate

### Keyboard Shortcuts
- **Ctrl+Space** - Trigger autocomplete
- **Ctrl+K Ctrl+I** - Show hover information
- **F12** - Go to definition (limited in web view)
- **Ctrl+/** - Toggle line comment

### Code Navigation
- **Ctrl+P** - Quick file open
- **Ctrl+G** - Go to line
- **Ctrl+F** - Find in file
- **Ctrl+H** - Find and replace

## Troubleshooting

### IntelliSense Not Working
- Ensure `.wc` or `.wcpp` file is open
- Wait a moment for IntelliSense to index (usually instant)
- Check `.vscode/c_cpp_properties.json` was created

### c_cpp_properties.json Not Generated
- Make sure you opened a `.wc` or `.wcpp` file
- Check that you have write permissions in the workspace folder
- Manually create `.vscode/c_cpp_properties.json` if needed

### Syntax Highlighting Not Applied
- Check file extension (should be `.wc`, `.wcpp`, or `.wrs`)
- Extension should activate within VS Code (see status bar)
- Restart VS Code if needed

## Next Steps

1. **Explore the API** - Start typing `in->` and see all available fields
2. **Read the full documentation** - See `include/WaviateInput.h` for detailed struct definition
3. **Try different shaders** - Create filters, generators, analysis functions
4. **Share feedback** - Report issues or suggest improvements

## Resources

- [Waviate Script Rules](../../Docs/rules.md) - Official specification
- [Waviate API Documentation](../../Docs/input_api_c.md) - C API reference
- [VS Code Extension API](https://code.visualstudio.com/api) - Extension development
- [IMPLEMENTATION.md](./IMPLEMENTATION.md) - Technical details of this extension

---

Happy shader coding! 🎵
