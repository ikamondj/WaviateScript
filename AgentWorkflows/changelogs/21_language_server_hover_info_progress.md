# 21_language_server_hover_info_progress.md

## 21a
- Removed `float adsr(...)` and its `ADSR` alias from `WaviateCore`.
- Renamed the midi note specific `midiNoteAdsr` to `adsr` across `Waviate.hpp` and `WaviateCppLanguageModel.cpp`.
- Next step is to implement the hover tooltip in `CodeEditor` that uses the completion provider to resolve the symbol under the mouse and display its type/documentation.

## 21b
- Added HoverPopup class to display a temporary tooltip for language elements.
- Integrated a mouse listener and hover timer in CodeEditor.
- Added hover logic using the CompletionProvider data to display documentation and type signatures of variables and methods directly.

## 21c
- Fixed ambiguous conversion compile error by removing duplicate MouseListener inheritance.
- Fixed compile error for HoverPopup constructor by explicitly initializing ctiveTheme with WaviateThemes::fallback().
