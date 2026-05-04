/*
  ==============================================================================

    AppTheme.h - Shared colour themes for the WaviateScript editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <cstdint>
#include <vector>

struct WaviateTheme
{
    WaviateTheme(const char* themeId,
                 const char* themeName,
                 bool darkTheme,
                 uint32_t windowBackgroundIn,
                 uint32_t panelBackgroundIn,
                 uint32_t toolbarBackgroundIn,
                 uint32_t widgetBackgroundIn,
                 uint32_t outlineIn,
                 uint32_t textIn,
                 uint32_t mutedTextIn,
                 uint32_t accentIn,
                 uint32_t accentTextIn,
                 uint32_t editorBackgroundIn,
                 uint32_t editorTextIn,
                 uint32_t selectionIn,
                 uint32_t lineNumberBackgroundIn,
                 uint32_t lineNumberTextIn,
                 uint32_t caretIn,
                 uint32_t visualizerBackgroundIn,
                 uint32_t visualizerWaveformIn,
                 uint32_t errorIn,
                 uint32_t commentIn,
                 uint32_t keywordIn,
                 uint32_t opIn,
                 uint32_t identifierIn,
                 uint32_t numberIn,
                 uint32_t stringLiteralIn,
                 uint32_t bracketIn,
                 uint32_t punctuationIn,
                 uint32_t preprocessorIn)
        : id(themeId),
          name(themeName),
          isDark(darkTheme),
          windowBackground(windowBackgroundIn),
          panelBackground(panelBackgroundIn),
          toolbarBackground(toolbarBackgroundIn),
          widgetBackground(widgetBackgroundIn),
          outline(outlineIn),
          text(textIn),
          mutedText(mutedTextIn),
          accent(accentIn),
          accentText(accentTextIn),
          editorBackground(editorBackgroundIn),
          editorText(editorTextIn),
          selection(selectionIn),
          lineNumberBackground(lineNumberBackgroundIn),
          lineNumberText(lineNumberTextIn),
          caret(caretIn),
          visualizerBackground(visualizerBackgroundIn),
          visualizerWaveform(visualizerWaveformIn),
          error(errorIn),
          comment(commentIn),
          keyword(keywordIn),
          op(opIn),
          identifier(identifierIn),
          number(numberIn),
          stringLiteral(stringLiteralIn),
          bracket(bracketIn),
          punctuation(punctuationIn),
          preprocessor(preprocessorIn)
    {
    }

    const char* id;
    const char* name;
    bool isDark;

    juce::Colour windowBackground;
    juce::Colour panelBackground;
    juce::Colour toolbarBackground;
    juce::Colour widgetBackground;
    juce::Colour outline;
    juce::Colour text;
    juce::Colour mutedText;
    juce::Colour accent;
    juce::Colour accentText;

    juce::Colour editorBackground;
    juce::Colour editorText;
    juce::Colour selection;
    juce::Colour lineNumberBackground;
    juce::Colour lineNumberText;
    juce::Colour caret;

    juce::Colour visualizerBackground;
    juce::Colour visualizerWaveform;

    juce::Colour error;
    juce::Colour comment;
    juce::Colour keyword;
    juce::Colour op;
    juce::Colour identifier;
    juce::Colour number;
    juce::Colour stringLiteral;
    juce::Colour bracket;
    juce::Colour punctuation;
    juce::Colour preprocessor;
};

namespace WaviateThemes
{
    inline const std::vector<WaviateTheme>& all()
    {
        static const std::vector<WaviateTheme> themes {
            {
                "vscode-dark", "VS Code Dark", true,
                0xff1e1e1e, 0xff252526, 0xff2d2d30, 0xff333333, 0xff3c3c3c,
                0xffd4d4d4, 0xff858585, 0xff007acc, 0xffffffff,
                0xff1e1e1e, 0xffd4d4d4, 0xff264f78, 0xff1b1b1b, 0xff858585, 0xffaeafad,
                0xff111111, 0xff4fc1ff,
                0xfff44747, 0xff6a9955, 0xff569cd6, 0xffd4d4d4, 0xff9cdcfe,
                0xffb5cea8, 0xffce9178, 0xffffd700, 0xffd4d4d4, 0xffc586c0
            },
            {
                "vscode-light", "VS Code Light", false,
                0xffffffff, 0xfff3f3f3, 0xffe5e5e5, 0xffffffff, 0xffcccccc,
                0xff1f1f1f, 0xff6a6a6a, 0xff0078d4, 0xffffffff,
                0xffffffff, 0xff000000, 0xffadd6ff, 0xfff7f7f7, 0xff237893, 0xff000000,
                0xfff7f9fc, 0xff0078d4,
                0xffcd3131, 0xff008000, 0xff0000ff, 0xff000000, 0xff001080,
                0xff098658, 0xffa31515, 0xff0431fa, 0xff000000, 0xffaf00db
            },
            {
                "visual-studio-dark", "Visual Studio Dark", true,
                0xff1e1e1e, 0xff252526, 0xff2d2d30, 0xff3f3f46, 0xff3f3f46,
                0xffdcdcdc, 0xff9b9b9b, 0xff68217a, 0xffffffff,
                0xff1e1e1e, 0xffdcdcdc, 0xff264f78, 0xff252526, 0xff8f8f8f, 0xffffffff,
                0xff151515, 0xffb77ee0,
                0xfff48771, 0xff57a64a, 0xff569cd6, 0xffd4d4d4, 0xff9cdcfe,
                0xffb5cea8, 0xffd69d85, 0xffd4d4d4, 0xffd4d4d4, 0xffc586c0
            },
            {
                "visual-studio-light", "Visual Studio Light", false,
                0xffeeeeee, 0xfff5f5f5, 0xffeeeeee, 0xffffffff, 0xffc8c8c8,
                0xff1e1e1e, 0xff6f6f6f, 0xff0078d7, 0xffffffff,
                0xffffffff, 0xff000000, 0xffadd6ff, 0xfff5f5f5, 0xff2b91af, 0xff000000,
                0xfffafafa, 0xff0078d7,
                0xffa31515, 0xff008000, 0xff0000ff, 0xff000000, 0xff001080,
                0xff098658, 0xffa31515, 0xff000000, 0xff000000, 0xffaf00db
            },
            {
                "jetbrains-darcula", "JetBrains Darcula", true,
                0xff2b2b2b, 0xff313335, 0xff3c3f41, 0xff3c3f41, 0xff4b4f51,
                0xffa9b7c6, 0xff808080, 0xff589df6, 0xfff8fbff,
                0xff2b2b2b, 0xffa9b7c6, 0xff214283, 0xff313335, 0xff606366, 0xffbbbbbb,
                0xff202124, 0xff80cbc4,
                0xffbc3f3c, 0xff808080, 0xffcc7832, 0xffa9b7c6, 0xffa9b7c6,
                0xff6897bb, 0xff6a8759, 0xffa9b7c6, 0xffa9b7c6, 0xffbbb529
            },
            {
                "jetbrains-light", "JetBrains Light", false,
                0xffffffff, 0xfff7f8fa, 0xfff0f1f2, 0xffffffff, 0xffc9cdd2,
                0xff202124, 0xff73787f, 0xff4c83ff, 0xffffffff,
                0xffffffff, 0xff000000, 0xffd8ecff, 0xfff2f4f7, 0xff8a8f98, 0xff000000,
                0xfff8fafc, 0xff4c83ff,
                0xffbc3f3c, 0xff808080, 0xff000080, 0xff000000, 0xff000000,
                0xff1750eb, 0xff008000, 0xff000000, 0xff000000, 0xff808000
            },
            {
                "cyberpunk-neon", "Cyberpunk Neon", true,
                0xff0b0014, 0xff12001f, 0xff19002e, 0xff22003d, 0xff6022a5,
                0xfff4edff, 0xff9d8ac7, 0xff00f5ff, 0xff070011,
                0xff0b0014, 0xfff4edff, 0xff52276f, 0xff130021, 0xffa987ff, 0xff00f5ff,
                0xff090010, 0xffff2bd6,
                0xffff5370, 0xff00ff95, 0xffff2bd6, 0xff00f5ff, 0xfff4edff,
                0xffff9f1c, 0xfffaff00, 0xff00f5ff, 0xfff4edff, 0xff9d4edd
            },
            {
                "solarized-dark", "Solarized Dark", true,
                0xff002b36, 0xff073642, 0xff073642, 0xff0b3a46, 0xff586e75,
                0xff839496, 0xff657b83, 0xff268bd2, 0xfffdf6e3,
                0xff002b36, 0xff839496, 0xff073642, 0xff073642, 0xff586e75, 0xff93a1a1,
                0xff00212a, 0xff2aa198,
                0xffdc322f, 0xff586e75, 0xff268bd2, 0xff839496, 0xff93a1a1,
                0xffd33682, 0xff2aa198, 0xffb58900, 0xff839496, 0xffcb4b16
            },
            {
                "solarized-light", "Solarized Light", false,
                0xfffdf6e3, 0xffeee8d5, 0xffeee8d5, 0xfff8f1dc, 0xff93a1a1,
                0xff657b83, 0xff839496, 0xff268bd2, 0xfffdf6e3,
                0xfffdf6e3, 0xff657b83, 0xffeee8d5, 0xffeee8d5, 0xff93a1a1, 0xff586e75,
                0xfff4edda, 0xff2aa198,
                0xffdc322f, 0xff93a1a1, 0xff268bd2, 0xff657b83, 0xff586e75,
                0xffd33682, 0xff2aa198, 0xffb58900, 0xff657b83, 0xffcb4b16
            },
            {
                "dracula", "Dracula", true,
                0xff282a36, 0xff343746, 0xff44475a, 0xff44475a, 0xff6272a4,
                0xfff8f8f2, 0xffa6adc8, 0xffbd93f9, 0xff282a36,
                0xff282a36, 0xfff8f8f2, 0xff44475a, 0xff21222c, 0xff6272a4, 0xfff8f8f0,
                0xff20212b, 0xffff79c6,
                0xffff5555, 0xff6272a4, 0xffff79c6, 0xffff79c6, 0xfff8f8f2,
                0xffbd93f9, 0xfff1fa8c, 0xfff8f8f2, 0xfff8f8f2, 0xffffb86c
            },
            {
                "nord", "Nord", true,
                0xff2e3440, 0xff3b4252, 0xff434c5e, 0xff3b4252, 0xff4c566a,
                0xffd8dee9, 0xffa7b0c0, 0xff88c0d0, 0xff2e3440,
                0xff2e3440, 0xffd8dee9, 0xff4c566a, 0xff3b4252, 0xff6f7d95, 0xffeceff4,
                0xff242933, 0xff8fbcbb,
                0xffbf616a, 0xff616e88, 0xff81a1c1, 0xffd8dee9, 0xffd8dee9,
                0xffb48ead, 0xffa3be8c, 0xffeceff4, 0xffd8dee9, 0xffebcb8b
            },
            {
                "synthwave-sunset", "Synthwave Sunset", true,
                0xff241734, 0xff30204a, 0xff3b255d, 0xff3b255d, 0xff624a7b,
                0xfffff7ff, 0xffaaa0c4, 0xffff7edb, 0xff211327,
                0xff241734, 0xfffff7ff, 0xff5a316f, 0xff2a1a3f, 0xff8f83b7, 0xff36f9f6,
                0xff1b1028, 0xffff7edb,
                0xffff5370, 0xff848bbd, 0xfffede5d, 0xffff7edb, 0xfffff7ff,
                0xffff9e64, 0xff72f1b8, 0xff36f9f6, 0xfffff7ff, 0xff36f9f6
            }
        };

        return themes;
    }

    inline const WaviateTheme& fallback()
    {
        return all().front();
    }

    inline const WaviateTheme& findById(const juce::String& id)
    {
        for (const auto& theme : all())
            if (id == theme.id)
                return theme;

        return fallback();
    }

    inline int indexOf(const juce::String& id)
    {
        const auto& themes = all();

        for (int i = 0; i < static_cast<int>(themes.size()); ++i)
            if (id == themes[static_cast<size_t>(i)].id)
                return i;

        return 0;
    }

    inline juce::CodeEditorComponent::ColourScheme createCodeColourScheme(const WaviateTheme& theme)
    {
        juce::CodeEditorComponent::ColourScheme scheme;
        scheme.set("Error", theme.error);
        scheme.set("Comment", theme.comment);
        scheme.set("Keyword", theme.keyword);
        scheme.set("Operator", theme.op);
        scheme.set("Identifier", theme.identifier);
        scheme.set("Integer", theme.number);
        scheme.set("Float", theme.number);
        scheme.set("String", theme.stringLiteral);
        scheme.set("Bracket", theme.bracket);
        scheme.set("Punctuation", theme.punctuation);
        scheme.set("Preprocessor Text", theme.preprocessor);
        return scheme;
    }

    inline juce::LookAndFeel_V4::ColourScheme createLookAndFeelColourScheme(const WaviateTheme& theme)
    {
        return juce::LookAndFeel_V4::ColourScheme(
            theme.windowBackground,
            theme.widgetBackground,
            theme.panelBackground,
            theme.outline,
            theme.text,
            theme.accent,
            theme.accentText,
            theme.accent,
            theme.text);
    }
}
