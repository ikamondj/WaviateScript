# Editor UI Transformation - Visual Guide

## Old vs New Layout

### Before: Button-Heavy Design
```
┌────────────────────────────────────────────────────────┐
│ Toolbar (36px)                                         │
│ ┌──────┬─────────┬──────────┬────────┬──────────────┐ │
│ │[New C]│[New C++]│[New Rust]│[Open...]│ Loaded: foo │
│ │       │(premium)│         │         │              │
│ └──────┴─────────┴──────────┴────────┴──────────────┘ │
├────────────────────────────────────────────────────────┤
│                                                        │
│                  Code Editor Area                      │
│                                                        │
│                                                        │
└────────────────────────────────────────────────────────┘

Issues with Old Design:
✗ Buttons get reordered with premium
✗ Hard to add more language options
✗ No keyboard support
✗ No help or documentation
✗ Limited expandability
✗ Cluttered toolbar
```

### After: Menu-Driven Design
```
┌────────────────────────────────────────────────────────┐
│ Toolbar (32px)                                         │
│ ┌──────┬──────┬──────────────────────────────────────┐ │
│ │[File]│[Help]│ Script: untitled.wc (unsaved)       │ │
│ └──────┴──────┴──────────────────────────────────────┘ │
│  ▼ File Menu        ▼ Help Menu                        │
│  ├─ New             ├─ About                           │
│  │  ├─ C (.wc)      ├─ Documentation                  │
│  │  ├─ C++ (.wcpp)  └─ Keyboard Shortcuts             │
│  │  └─ Rust (.wrs)                                    │
│  ├─ Open... (Ctrl+O)                                 │
│  ├─ Save (Ctrl+S)                                    │
│  ├─ Save As... (Ctrl+Shift+S)                        │
│  └─ Exit                                              │
├────────────────────────────────────────────────────────┤
│                                                        │
│                  Code Editor Area                      │
│                                                        │
│                                                        │
└────────────────────────────────────────────────────────┘

Benefits of New Design:
✓ Clean, professional appearance
✓ Unlimited language support via menu
✓ Premium integration seamless
✓ Keyboard shortcuts (Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Shift+S)
✓ Help documentation integrated
✓ Easy to extend with new menus
✓ Compact toolbar with relevant info
```

## Keyboard Shortcut Quick Reference

### Visual Cheat Sheet
```
┌─────────────────────────────────────────────┐
│  WAVIATE SCRIPT - KEYBOARD SHORTCUTS        │
├─────────────────────────────────────────────┤
│                                             │
│  New File ..................... Ctrl+N     │
│  Open File .................... Ctrl+O     │
│  Save File .................... Ctrl+S     │
│  Save As ................... Ctrl+Shift+S  │
│                                             │
│  When Transient File:                       │
│  └─ Ctrl+S shows Save As dialog             │
│                                             │
│  When Existing File:                        │
│  └─ Ctrl+S saves directly to file           │
│                                             │
│  Always:                                    │
│  └─ Ctrl+Shift+S shows Save As dialog       │
│                                             │
└─────────────────────────────────────────────┘
```

## File Status Display Evolution

### Display Examples

|Scenario | Before | After |
|---------|--------|-------|
| No file | "No script loaded" | "No file loaded" |
| New unsaved | "• untitled.wc" | "• untitled.wc (unsaved)" |
| Existing file | "Loaded: foo.wc" | "foo.wc" |
| Existing modified | "Loaded: foo.wc" | "foo.wc" *(could add indicator)* |

### Status Label Design
```
Old:
┌─────────────────────────┐
│ • untitled.wc           │  ← Minimal info
└─────────────────────────┘

New:
┌─────────────────────────┐
│ • untitled.wc (unsaved) │  ← Clear state
└─────────────────────────┘
```

## File Menu Organization

### Logical Hierarchy
```
File
├─ New                         (Ctrl+N)
│  ├─ C File (.wc)            ──────────┐
│  ├─ C++ File (.wcpp)                  ├─ Creates new,
│  └─ Rust File (.wrs)         PREMIUM  │  shows Save As
│
├─ ─────────────────           (separator)
│
├─ Open...                     (Ctrl+O)  ──── Loads existing
├─ ─────────────────           (separator)
│
├─ Save                        (Ctrl+S)  ──── Smart:
│                                            ├─ New → Save As
│                                            └─ Existing → Direct
├─ Save As...                  (Ctrl+Shift+S) ──── Always dialog
│ ─────────────────            (separator)
│
└─ Exit                                   ──── Close
```

## State Diagram: File Lifecycle

```
                    ┌──────────────────────────┐
                    │   User Starts Editor     │
                    │   (No file loaded)       │
                    └────────────┬─────────────┘
                                 │
                    ┌────────────┴─────────────────────┐
                    │                                  │
                    ▼                                  ▼
        ┌──────────────────────────┐    ┌──────────────────────────┐
        │    User Presses Ctrl+N   │    │    User Presses Ctrl+O   │
        │   (File → New)           │    │   (File → Open)          │
        │                          │    │                          │
        │ Shows Language Menu      │    │ Shows File Dialog        │
        │ ├─ C                     │    │                          │
        │ ├─ C++                   │    │ Selects: existing.wc     │
        │ └─ Rust (premium)        │    │                          │
        └────────────┬─────────────┘    └────────────┬─────────────┘
                     │                               │
                     │ User Selects Language        │ File Selected
                     │                               │
                     ▼                               ▼
    ┌─────────────────────────────────────────────────────────────┐ │
    │                                                             │
    │  isFileTransient = true                                    │ │
    │  Status: "• untitled.wc (unsaved)"                         │ │
    │  ┌─────────────────────────────────────────────────────┐ │ │
    │  │              Transient File State                    │ │ │
    │  │  (New file not yet saved to disk)                  │ │ │
    │  └─────────────────────────────────────────────────────┘ │ │
    │                      │                                     │ │
    │                      ├─► User Presses Ctrl+S              │ │
    │                      │   └─► Save As Dialog               │ │
    │                      │       └─► File Saved               │ │
    │                      │           └─► isFileTransient = false
    │                      │               Status: "untitled.wc"
    │                      │
    │                      └─► User Presses Ctrl+Shift+S        │ │
    │                          └─► Save As Dialog               │ │
    │                              └─► File Saved               │ │
    │                                  └─► isFileTransient = false
    │                                                             │ │
    └──────────────────────────────────────────────────────────────┘ │
                     │                                               │
                     └─► isFileTransient = false                 ◄────┘
                         Status: "existing.wc"
                         ┌─────────────────────────────────────────┐
                         │      Existing File State               │
                         │  (File saved to disk)                  │
                         ├────────────────────────────────────────┤
                         │ Ctrl+S → Direct save (no dialog)       │
                         │ Ctrl+Shift+S → Save As (shows dialog) │
                         └─────────────────────────────────────────┘
```

## Keyboard Shortcut Flow

### New File Workflow
```
Press Ctrl+N
    ↓
┌─────────────────────────┐
│  File → New Submenu     │
│  ├─ C                   │  Select Language
│  ├─ C++ (highlighted)   │
│  └─ Rust (premium)      │
└─────────────────────────┘
    ↓
Select C++
    ↓
┌─────────────────────────┐
│ Save New C++ File       │  File Dialog
│ ┌───────────────────┐   │
│ │ Documents/        │   │ Navigate & Name
│ │ ├─ hello.wcpp ✓   │   │
│ │ ├─ untitled.wcpp  │   │
│ └───────────────────┘   │
└─────────────────────────┘
    ↓
Save
    ↓
┌─────────────────────────────────┐
│ FILE LOADED & COMPILED          │
│ Status: untitled.wcpp           │
│ Editor shows: template code     │
│ Ready for editing               │
└─────────────────────────────────┘
```

### Save Workflow (New File)
```
Ctrl+S
   ↓
Is file transient?
   ├─ YES (New, Never Saved)
   │   └─► Save As Dialog ──► File Saved
   │       isFileTransient = false
   │       Status: "filename" (no "unsaved")
   │       Auto-compile
   │
   └─ NO (Already Saved)
       └─► Direct Save ──► File Saved
           Auto-compile
           Status: "filename"
```

### Save Workflow (Existing File)
```
Ctrl+S
   ↓
Is file transient?
   └─ NO (Already Saved)
       └─► Direct Save ──► File Saved
           Status: "filename"
           Auto-compile

Ctrl+Shift+S
   ↓
Always shows Save As Dialog
   ↓
─► File Saved (to new location if chosen)
   isFileTransient = false
   Status updated
   Auto-compile
```

## Transient State Indicator

### Visual Feedback
```
New File:
┌────────────────────────────────────────────┐
│ [File] [Help]  • untitled.wc (unsaved)     │  ◄─ Bullet + "(unsaved)"
└────────────────────────────────────────────┘     indicates transient

After Ctrl+S and Save:
┌────────────────────────────────────────────┐
│ [File] [Help]  hello.wcpp                   │  ◄─ Clean, no indicators
└────────────────────────────────────────────┘     means saved
```

## Help Menu Contents

### Help → About
```
╔════════════════════════════════════════════╗
║       About Waviate Script                  ║
╠════════════════════════════════════════════╣
║                                            ║
║  Waviate Script                            ║
║  Interactive Audio Plugin Editor           ║
║                                            ║
║  Create and compile audio processing       ║
║  scripts in C, C++, and Rust.              ║
║                                            ║
║  🔗 https://github.com/ikamondj/Waviate    ║
║                                            ║
╚════════════════════════════════════════════╝
```

### Help → Keyboard Shortcuts
```
╔════════════════════════════════════════════╗
║       Keyboard Shortcuts                    ║
╠════════════════════════════════════════════╣
║                                            ║
║  Ctrl+N - New File                          ║
║  Ctrl+O - Open File                         ║
║  Ctrl+S - Save File                         ║
║  Ctrl+Shift+S - Save As                     ║
║                                            ║
╚════════════════════════════════════════════╝
```

### Help → Online Documentation
```
Clicking opens GitHub Wiki page in browser:
https://github.com/ikamondj/WaviateScript/wiki
```

## Component Hierarchy Changes

### Before
```
WaviateScriptAudioProcessorEditor
├── topBar (Component)
│   ├── newButton (TextButton)
│   ├── newCppButton (TextButton)
│   ├── newRustButton (TextButton) [Premium]
│   ├── openButton (TextButton)
│   └── currentScriptLabel (Label)
├── codeEditor (CodeEditor)
└── emptyStateLabel (Label)
```

### After
```
WaviateScriptAudioProcessorEditor (+ KeyListener)
├── toolbar (Component)
│   ├── fileMenuButton (TextButton) [creates PopupMenu]
│   ├── helpMenuButton (TextButton) [creates PopupMenu]
│   └── currentFileLabel (Label)
├── codeEditor (CodeEditor)
└── emptyStateLabel (Label)

+ Keyboard handling: keyPressed()
```

**Advantages:**
- Fewer direct UI components
- Menus created dynamically
- Cleaner hierarchy
- More maintainable

## File Extension Support

### Standard Extensions
| Language | Extension | Premium | Supported |
|----------|-----------|---------|-----------|
| C | .wc | No | ✅ All |
| C++ | .wcpp | No | ✅ All |
| Rust | .wrs | Yes | ✅ Premium |

### Future Extensions (Possible)
| Language | Extension | Notes |
|----------|-----------|-------|
| Python | .wpy | Could use libpython |
| WebAssembly | .wwasm | Compile to WASM |
| GLSL | .wglsl | GPU shaders |

