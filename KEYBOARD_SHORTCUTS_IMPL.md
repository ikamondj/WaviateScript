# Keyboard Shortcut Implementation Details

## Architecture

The plugin editor uses JUCE's `KeyListener` interface for keyboard event handling. This provides robust, hierarchical key event processing.

### Inheritance
```cpp
class WaviateScriptAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::KeyListener
```

**Why private?**
- Prevents direct external access to keyPressed()
- Only available through Component's key event system
- Standard JUCE pattern

### Registration
```cpp
// In constructor:
addKeyListener(this);
```

This registers the editor as a key listener for its own component hierarchy.

## Key Handling Flow

```
User presses key
    ↓
JUCE OS layer
    ↓
JuceComponent receives key event
    ↓
Component::keyPressed() called on all registered KeyListeners
    ↓
WaviateScriptAudioProcessorEditor::keyPressed()
    ↓
    ├─ Check for Ctrl+N → showNewFileMenu()
    ├─ Check for Ctrl+O → openFile()
    ├─ Check for Ctrl+S → saveFile()
    ├─ Check for Ctrl+Shift+S → saveFileAs()
    │
    └─ Return true (consumed) or false (let it propagate)
```

## Implementation Details

### Keyboard Shortcut Matching

```cpp
bool WaviateScriptAudioProcessorEditor::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    // Method 1: Using getKeyCode() directly
    if (key.isKeyCode(juce::KeyPress::createFromDescription("ctrl+n").getKeyCode())) {
        showNewFileMenu();
        return true;  // Signal we consumed the key
    }
    
    // Returns false for unhandled keys (allows propagation)
    return false;
}
```

### Understanding the KeyPress Object

```cpp
struct juce::KeyPress {
    int keyCode      // Virtual key code
    int modifiers    // Bitmask of Ctrl, Shift, Alt, Cmd
    juce::String getText()  // Human-readable description
};
```

### Modifier Bitmasks

These can be combined with `|` operator:
```cpp
juce::ModifierKeys::ctrlModifier      // Ctrl key
juce::ModifierKeys::shiftModifier     // Shift key
juce::ModifierKeys::altModifier       // Alt key
juce::ModifierKeys::commandModifier   // Cmd key (Mac) / Win key (Windows)
```

## Current Shortcuts

```cpp
if (key.isKeyCode(...("ctrl+n")...)) {     // Ctrl+N
if (key.isKeyCode(...("ctrl+o")...)) {     // Ctrl+O
if (key.isKeyCode(...("ctrl+s")...)) {     // Ctrl+S
if (key.isKeyCode(...("ctrl+shift+s")...)){ // Ctrl+Shift+S
```

## Adding New Shortcuts

### Pattern 1: Simple Action
```cpp
// Add in keyPressed():
if (key.isKeyCode(juce::KeyPress::createFromDescription("ctrl+e").getKeyCode())) {
    exportFile();
    return true;
}

// Create the handler method:
void WaviateScriptAudioProcessorEditor::exportFile()
{
    // Implementation
}
```

### Pattern 2: Conditional Shortcut
```cpp
// Only enable save if file is open
if (key.isKeyCode(juce::KeyPress::createFromDescription("ctrl+s").getKeyCode())) {
    if (currentScriptFile.existsAsFile()) {
        saveFile();
        return true;
    }
}
```

### Pattern 3: Multiple Keys for Same Action
```cpp
// Both Backspace and Delete delete
if (key.getKeyCode() == juce::KeyPress::backspaceKey ||
    key.getKeyCode() == juce::KeyPress::deleteKey) {
    deleteSelection();
    return true;
}
```

## Avoiding Conflicts

### JUCE-Reserved Shortcuts
Be careful with these common shortcuts:
- Ctrl+C, Ctrl+X, Ctrl+V (handled by juce::TextEditor automatically)
- Tab/Shift+Tab (focus navigation)
- Escape (often cancels dialogs)

### Mac vs Windows
```cpp
// Handle both Windows Ctrl and Mac Cmd
const auto isMod = key.getModifiers().isCtrlDown();  // Cross-platform
```

## Debugging Keyboard Input

### Verbose Logging
```cpp
bool WaviateScriptAudioProcessorEditor::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    // Temporary debugging
    JUCE_LOG(DBG, "Key: " + key.getTextDescription());
    JUCE_LOG(DBG, "KeyCode: " + juce::String(key.getKeyCode()));
    JUCE_LOG(DBG, "Modifiers: " + juce::String(key.getModifiers().getRawFlags()));
    
    // ... rest of implementation
}
```

### Key Code Reference
```cpp
// JUCE special keys
juce::KeyPress::returnKey        // Enter/Return
juce::KeyPress::escapeKey        // Escape
juce::KeyPress::tabKey           // Tab
juce::KeyPress::backspaceKey     // Backspace
juce::KeyPress::deleteKey        // Delete
juce::KeyPress::spaceKey         // Space
juce::KeyPress::F1Key through F12Key  // Function keys
juce::KeyPress::leftKey          // Arrow keys...
```

## Event Propagation

### Returning true (Consumed)
```cpp
return true;  // Key was handled, don't propagate further
```

**Effects:**
- Stops event propagation
- Parent components won't see the key
- Prevents default system behavior (usually)

### Returning false (Not Consumed)
```cpp
return false;  // Key not handled, propagate to parent
```

**Effects:**
- Event continues to parent component's KeyListeners
- Other text input components might process it
- Default system behavior may occur

## Testing Keyboard Shortcuts

### Manual Testing
```
1. Launch plugin editor
2. Press Ctrl+N → Should show New File menu
3. Press Ctrl+O → Should show Open dialog
4. Press Ctrl+S with no file → Should show Save As
5. Press Ctrl+S with existing file → Should save directly
6. Press Ctrl+Shift+S → Should show Save As (always)
```

### Unit Test Example
```cpp
void testKeyboardShortcut()
{
    WaviateScriptAudioProcessor processor;
    WaviateScriptAudioProcessorEditor editor(processor);
    
    // Now press Ctrl+N programmatically:
    juce::KeyPress newFileKey = 
        juce::KeyPress::createFromDescription("ctrl+n");
    
    bool wasConsumed = editor.keyPressed(newFileKey, &editor);
    assertThat(wasConsumed, isTrue());
}
```

## Cross-Platform Considerations

### Windows vs Mac Modifiers
```cpp
// Windows: Ctrl+S
// Mac: Cmd+S

// JUCE handles this with ModifierKeys::ctrlModifier
// which maps to Ctrl on Windows, Cmd on Mac automatically
```

### Focus Issues
If shortcuts don't work:
1. Ensure KeyListener is added: `addKeyListener(this);`
2. Check component has focus (ComponentListener can help)
3. Verify parent component isn't consuming keys
4. Check for platform-specific key stripping

## Performance Considerations

### Tight Loop
```cpp
bool keyPressed(const juce::KeyPress& key, juce::Component*)
{
    // Called for EVERY key press in the component
    // Keep this fast - complex work should be in handlers
    if (key.isKeyCode(...)) {
        // Simple check only
        showNewFileMenu();  // This is fine - menu creation is lazy
        return true;
    }
    return false;  // Return quickly if not our key
}
```

## Future Enhancements

### Store Shortcuts in Configuration
```cpp
class ShortcutMap {
    std::map<juce::String, std::function<void()>> shortcuts;
    
public:
    void registerShortcut(const juce::String& desc, std::function<void()> fn) {
        shortcuts[desc] = fn;
    }
    
    bool handle(const juce::KeyPress& key) {
        for (auto& [desc, fn] : shortcuts) {
            if (key.isKeyCode(juce::KeyPress::createFromDescription(desc).getKeyCode())) {
                fn();
                return true;
            }
        }
        return false;
    }
};
```

### User-Configurable Shortcuts
Future: Allow users to rebind shortcuts in preferences.

### Accelerator Table
Could use MidiMessages for MIDI control in parallel to keyboard.

## JUCE Documentation

Reference:
- `juce::KeyListener` - Base class
- `juce::KeyPress` - Key event data
- `juce::KeyboardFocusTraverser` - Tab order management
- `KeyboardListener` - Deprecated, use KeyListener

## Common Pitfalls

❌ **Don't** use `key.getKeyCode() == 'N'`
```cpp
// WRONG - character codes vary by locale and caps lock
```

✅ **Do** use KeyPress::createFromDescription()
```cpp
// RIGHT - JUCE handles platform differences
if (key.isKeyCode(juce::KeyPress::createFromDescription("ctrl+n").getKeyCode()))
```

❌ **Don't** forget to return true
```cpp
// WRONG - will propagate to parent and may cause issues
void keyPressed() {
    showFileMenu();
    // Missing: return true;
}
```

✅ **Do** consume handled keys
```cpp
// RIGHT
bool keyPressed() {
    showFileMenu();
    return true;  // Signal consumed
}
```

❌ **Don't** forget addKeyListener()
```cpp
// WRONG - shortcut won't work
class MyEditor : public juce::AudioProcessorEditor, private juce::KeyListener {
    MyEditor() {
        // Missing: addKeyListener(this);
    }
};
```

✅ **Do** register listener
```cpp
// RIGHT
MyEditor() {
    addKeyListener(this);  // Register
}
```

