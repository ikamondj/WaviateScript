# Plugin Editor - Quick Reference

## For Users

### Create a New File
**Method 1:** Click `File` menu → `New` → Select language  
**Method 2:** Press `Ctrl+N` → Select language

Supported languages:
- C (.wc)
- C++ (.wcpp)  
- Rust (.wrs) - Premium only

### Open Existing File
**Method 1:** Click `File` menu → `Open`  
**Method 2:** Press `Ctrl+O`

### Save Your Work
| Action | Method |
|--------|--------|
| Save to current file | Ctrl+S |
| Save As (always shows dialog) | Ctrl+Shift+S |
| First save of new file | Ctrl+S (triggers Save As) |

### File Status Display
```
Filename.ext          → Saved and up-to-date
• Filename.ext (u)    → Unsaved/transient file
No file loaded        → Empty state
```

### Keyboard Shortcuts
```
Ctrl+N     New file (shows language menu)
Ctrl+O     Open file
Ctrl+S     Save (or Save As if new)
Ctrl+Shift+S  Save As dialog
```

### Help
Click `Help` menu for:
- About Waviate Script
- Online Documentation  
- Keyboard Shortcuts

---

## For Developers

### Class: WaviateScriptAudioProcessorEditor

**Key Members:**
```cpp
bool isFileTransient    // true = user hasn't saved yet
bool isFileModified     // true = unsaved changes
juce::File currentScriptFile
```

**Key Methods:**
```cpp
void showFileMenu()
void createNewFile(const juce::String& languageExtension)
void saveFile()
void saveFileAs()
bool keyPressed(const juce::KeyPress& key, juce::Component*)
```

### File Lifecycle

```
New File
  ├─ isFileTransient = true
  ├─ Status: "• untitled.wc (unsaved)"
  └─ User presses Ctrl+S
      └─ Save As dialog
          └─ File saved
              └─ isFileTransient = false
                  └─ Status: "untitled.wc"

Open File
  ├─ isFileTransient = false
  ├─ Status: "filename.wc"
  └─ User presses Ctrl+S
      └─ Direct save (no dialog)
```

### Template System

Templates are provider by generator classes:
- `CfileTemplateGenerator` → C templates
- `CppFileTemplateGenerator` → C++ templates
- `RustFileTemplateGenerator` → Rust templates (premium)

To add new language:
1. Create template generator class
2. Add to PluginEditor member variables
3. Update `getTemplateForLanguage()` method
4. Update `showNewFileMenu()` to add menu item
5. Handle in language selection switch

### Auto-Compilation

Files auto-compile after successful save:
```cpp
// In handleSaveFileDialogResult():
audioProcessor.loadProgram(file);  // Triggers compile
```

### Extending the File Menu

```cpp
void WaviateScriptAudioProcessorEditor::showFileMenu()
{
    juce::PopupMenu fileMenu;
    
    fileMenu.addItem(1, "New", true);
    fileMenu.addItem(2, "Open...", true);
    // ADD NEW ITEM HERE:
    fileMenu.addItem(6, "Export...", true);
    
    fileMenu.showMenuAsync(..., [this](int result) {
        switch (result) {
            // ...
            case 6: exportFile(); break;  // ADD CASE HERE
        }
    });
}
```

### Code Organization

**Private Methods by Category:**

File Menu Operations:
- `showFileMenu()`
- `showNewFileMenu()`

File Operations:
- `createNewFile()`
- `openFile()`
- `saveFile()`
- `saveFileAs()`

Dialog Result Handlers:
- `handleNewFileDialogResult()`
- `handleOpenFileDialogResult()`
- `handleSaveFileDialogResult()`

File Management:
- `loadScriptFile()`
- `updateFileLabel()`

Helpers:
- `getTemplateForLanguage()`
- `getLanguageDisplayName()`
- `getDefaultSaveDirectory()`

UI State:
- `showEmptyState()`
- `hideEmptyState()`

---

## Best Practices Used

✅ KeyListener for keyboard input  
✅ Lazy initialization (CodeEditor)  
✅ PopupMenu for menus (JUCE standard)  
✅ Async file dialogs (non-blocking)  
✅ Clear naming conventions  
✅ Single responsibility per method  
✅ DRY principle - no repeated code  
✅ Proper unique_ptr resource management  
✅ `#ifdef` for premium features  

---

## Common Tasks

### Add a New Keyboard Shortcut
```cpp
// In keyPressed():
if (key.isKeyCode(juce::KeyPress::createFromDescription("ctrl+e").getKeyCode())) {
    exportFile();
    return true;
}
```

### Add Language Support
```cpp
// 1. Update showNewFileMenu()
newMenu.addItem(4, "New Language File (.ext)", true);

// 2. Update createNewFile() switch
case 4: createNewFile(".ext"); break;

// 3. Update getTemplateForLanguage()
else if (languageExtension == ".ext") {
    return newTemplateGen.getDefaultFileSource();
}

// 4. Update getLanguageDisplayName()
else if (languageExtension == ".ext") {
    return "New Language File";
}
```

### Add Help Menu Item
```cpp
// In help menu lambda:
helpMenu.addItem(4, "New Help Topic");

// In the result handler:
case 4: 
    juce::AlertWindow::showMessageBox(...);
    break;
```

---

## Keyboard Shortcut Syntax

JUCE keyboard shortcuts use this format:
```cpp
juce::KeyPress::createFromDescription("ctrl+n")      // Ctrl+N
juce::KeyPress::createFromDescription("ctrl+shift+s")  // Ctrl+Shift+S
juce::KeyPress::createFromDescription("alt+f")        // Alt+F
```

---

## Debugging Tips

**Check file state:**
```cpp
JUCE_LOG(DBG, "isFileTransient: " + juce::String(isFileTransient));
JUCE_LOG(DBG, "File: " + currentScriptFile.getFullPathName());
```

**Verify editor creation:**
```cpp
// Add to resized():
if (editor == nullptr) {
    JUCE_LOG(DBG, "WARNING: Editor not created!");
}
```

**Check compile results:**
```cpp
// After loadProgram():
auto sampleShader = audioProcessor.activeSampleShader.load();
if (sampleShader) {
    JUCE_LOG(DBG, "Compile successful!");
}
```

