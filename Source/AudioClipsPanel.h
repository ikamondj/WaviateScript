#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "AppTheme.h"
#include <iomanip>
#include <sstream>

class AudioClipsPanel : public juce::Component
{
public:
    AudioClipsPanel(WaviateScriptAudioProcessor& processor, std::function<void()> onRecompileNeeded)
        : processor(processor), onRecompileNeeded(onRecompileNeeded)
    {
        // Add Header
        titleLabel.setText("Audio Clips", juce::dontSendNotification);
        titleLabel.setFont(juce::Font(16.0f, juce::Font::bold));
        addAndMakeVisible(titleLabel);

        addButton.setButtonText("+");
        addButton.onClick = [this] { addManualClip(); };
        addAndMakeVisible(addButton);

        // Add scroll viewport
        viewport.setViewedComponent(&listContent, false);
        addAndMakeVisible(viewport);

        // Add Footer
        memoryLabel.setFont(juce::Font(12.0f));
        addAndMakeVisible(memoryLabel);

        clearButton.setButtonText("Clear Cache");
        clearButton.onClick = [this] { clearAll(); };
        addAndMakeVisible(clearButton);

        updateList();
    }

    void updateList()
    {
        listContent.removeAllChildren();
        
        // Let's query cache snapshot
        auto snapshot = processor.getAudioCache().snapshot();
        
        // Let's build lists of manual clips and shader-loaded clips.
        std::vector<juce::Component*> rows;

        // 1. Render manual clips
        const auto& manualClips = processor.getManualClips();
        for (int i = 0; i < static_cast<int>(manualClips.size()); ++i)
        {
            const auto& mClip = manualClips[static_cast<size_t>(i)];
            
            // Find its cache status if path is not empty
            waviate::audio::WaviateAudioCacheEntryInfo cacheInfo;
            bool foundInCache = false;
            if (mClip.path.isNotEmpty())
            {
                for (const auto& info : snapshot)
                {
                    if (info.location == mClip.path.toStdString())
                    {
                        cacheInfo = info;
                        foundInCache = true;
                        break;
                    }
                }
            }

            auto* row = new ManualClipRow(processor, i, mClip.path, mClip.name, cacheInfo, foundInCache,
                [this] { updateList(); if (onRecompileNeeded) onRecompileNeeded(); });
            listContent.addAndMakeVisible(row);
            rows.push_back(row);
        }

        // 2. Render shader clips
        for (const auto& info : snapshot)
        {
            if (! info.isManual)
            {
                auto* row = new ShaderClipRow(processor, info, [this] { updateList(); });
                listContent.addAndMakeVisible(row);
                rows.push_back(row);
            }
        }

        // Position rows
        int y = 0;
        const int rowHeight = 60;
        for (auto* row : rows)
        {
            row->setBounds(0, y, getWidth() - 16, rowHeight);
            y += rowHeight + 4;
        }

        listContent.setSize(getWidth() - 16, y);

        // Update memory usage label
        const double mbs = static_cast<double>(processor.getAudioCache().loadedBytes()) / (1024.0 * 1024.0);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << mbs << " MB";
        memoryLabel.setText("Memory: " + ss.str(), juce::dontSendNotification);
        
        resized();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(8);
        
        auto header = bounds.removeFromTop(24);
        titleLabel.setBounds(header.removeFromLeft(bounds.getWidth() - 32));
        addButton.setBounds(header.removeFromRight(24));

        auto footer = bounds.removeFromBottom(60);
        memoryLabel.setBounds(footer.removeFromTop(24));
        clearButton.setBounds(footer.removeFromBottom(28));

        bounds.removeFromBottom(8);
        viewport.setBounds(bounds);
    }

private:
    void addManualClip()
    {
        // Add clip to manualClips list in processor
        juce::String defaultName = "Clip_" + juce::String(static_cast<int>(processor.getManualClipCount() + 1));
        processor.addManualClip("", defaultName);
        updateList();
    }

    void clearAll()
    {
        processor.clearAllAudioClips();
        updateList();
        if (onRecompileNeeded)
            onRecompileNeeded();
    }

    class ManualClipRow : public juce::Component
    {
    public:
        ManualClipRow(WaviateScriptAudioProcessor& processor,
                      int index,
                      const juce::String& path,
                      const juce::String& name,
                      const waviate::audio::WaviateAudioCacheEntryInfo& cacheInfo,
                      bool inCache,
                      std::function<void()> onUpdate)
            : processor(processor), index(index), path(path), name(name),
              cacheInfo(cacheInfo), inCache(inCache), onUpdate(onUpdate)
        {
            nameInput.setText(name, juce::dontSendNotification);
            nameInput.onTextChange = [this] {
                if (this->index >= 0 && static_cast<size_t>(this->index) < this->processor.getManualClipCount()) {
                    this->processor.setManualClipName(static_cast<size_t>(this->index), nameInput.getText());
                    this->onUpdate();
                }
            };
            addAndMakeVisible(nameInput);

            if (path.isEmpty())
            {
                selectButton.setButtonText("Select File...");
                selectButton.onClick = [this] { chooseFile(); };
                addAndMakeVisible(selectButton);

#ifdef WAV_SCRIPT_PREMIUM
                urlButton.setButtonText("Enter URL...");
                urlButton.onClick = [this] { enterUrl(); };
                addAndMakeVisible(urlButton);
#endif
            }
            else
            {
                juce::File file(path);
                pathLabel.setText(file.getFileName(), juce::dontSendNotification);
                pathLabel.setFont(juce::Font(10.0f));
                addAndMakeVisible(pathLabel);

                juce::String statusText = "Pending";
                juce::Colour statusColour = juce::Colours::orange;
                
                if (this->inCache)
                {
                    if (this->cacheInfo.state == waviate::audio::WaviateAudioLoadState::Loading)
                    {
                        statusText = "Loading...";
                    }
                    else if (this->cacheInfo.state == waviate::audio::WaviateAudioLoadState::Ready)
                    {
                        statusText = "Ready";
                        statusColour = juce::Colours::lightgreen;
                        pathLabel.setTooltip(juce::String("Channels: ") + juce::String(this->cacheInfo.channelCount) 
                            + "\nSample Rate: " + juce::String(this->cacheInfo.sampleRate) + " Hz"
                            + "\nFrames: " + juce::String(this->cacheInfo.frameCount));
                    }
                    else if (this->cacheInfo.state == waviate::audio::WaviateAudioLoadState::Failed)
                    {
                        statusText = "Failed";
                        statusColour = juce::Colours::red;
                        pathLabel.setTooltip(this->cacheInfo.errorMessage);
                    }
                }
                statusLabel.setText(statusText, juce::dontSendNotification);
                statusLabel.setFont(juce::Font(10.0f, juce::Font::italic));
                statusLabel.setColour(juce::Label::textColourId, statusColour);
                addAndMakeVisible(statusLabel);
            }

            deleteButton.setButtonText("X");
            deleteButton.onClick = [this] { removeClip(); };
            addAndMakeVisible(deleteButton);
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            auto deleteArea = bounds.removeFromRight(20);
            deleteButton.setBounds(deleteArea.reduced(2));

            auto top = bounds.removeFromTop(28);
            nameInput.setBounds(top.reduced(2));

            if (selectButton.isVisible())
            {
#ifdef WAV_SCRIPT_PREMIUM
                auto halfWidth = bounds.getWidth() / 2;
                selectButton.setBounds(bounds.removeFromLeft(halfWidth).reduced(2));
                urlButton.setBounds(bounds.reduced(2));
#else
                selectButton.setBounds(bounds.reduced(2));
#endif
            }
            else
            {
                auto halfWidth = bounds.getWidth() / 2;
                pathLabel.setBounds(bounds.removeFromLeft(halfWidth).reduced(2));
                statusLabel.setBounds(bounds.reduced(2));
            }
        }

    private:
        void chooseFile()
        {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Select Audio File for Clip",
                juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                "*.wav;*.aif;*.mp3"
            );

            fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this](const juce::FileChooser& chooser) {
                    auto result = chooser.getResult();
                    if (result.existsAsFile())
                    {
                        juce::String filePath = result.getFullPathName();
                        if (index >= 0 && static_cast<size_t>(index) < processor.getManualClipCount()) {
                            processor.setManualClipPath(static_cast<size_t>(index), filePath);
                            onUpdate();
                        }
                    }
                });
        }

#ifdef WAV_SCRIPT_PREMIUM
        void enterUrl()
        {
            juce::AlertWindow::showAsync(
                juce::MessageBoxOptions()
                    .withIconType(juce::MessageBoxIconType::QuestionIcon)
                    .withTitle("Enter Audio URL")
                    .withMessage("Please enter the direct URL to an audio file (http/https):")
                    .withButton("OK")
                    .withButton("Cancel"),
                [this](int buttonIndex) {
                    if (buttonIndex == 1) // OK
                    {
                        // To get the text, we actually need to use a custom AlertWindow or just TextEditor.
                        // For simplicity, let's just use NativeMessageBox if possible, 
                        // but since juce::AlertWindow::showAsync with textbox is complex,
                        // let's create a full juce::AlertWindow here.
                    }
                });
            
            // Re-implementing with custom AlertWindow to get text
            auto* alert = new juce::AlertWindow("Enter Audio URL", "Please enter the direct URL to an audio file (http/https):", juce::MessageBoxIconType::QuestionIcon);
            alert->addTextEditor("url", "https://", "URL");
            alert->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
            alert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

            alert->enterModalState(true, juce::ModalCallbackFunction::create([this, alert](int result) {
                if (result == 1)
                {
                    juce::String urlStr = alert->getTextEditorContents("url");
                    if (urlStr.isNotEmpty() && index >= 0 && static_cast<size_t>(index) < processor.getManualClipCount()) {
                        processor.setManualClipPath(static_cast<size_t>(index), urlStr);
                        onUpdate();
                    }
                }
                delete alert;
            }));
        }
#endif

        void removeClip()
        {
            if (index >= 0 && static_cast<size_t>(index) < processor.getManualClipCount()) {
                processor.removeManualClipAt(static_cast<size_t>(index));
            }
            onUpdate();
        }

        WaviateScriptAudioProcessor& processor;
        int index;
        juce::String path;
        juce::String name;
        waviate::audio::WaviateAudioCacheEntryInfo cacheInfo;
        bool inCache;
        std::function<void()> onUpdate;

        juce::TextEditor nameInput;
        juce::TextButton selectButton;
#ifdef WAV_SCRIPT_PREMIUM
        juce::TextButton urlButton;
#endif
        juce::Label pathLabel;
        juce::Label statusLabel;
        juce::TextButton deleteButton;

        std::unique_ptr<juce::FileChooser> fileChooser;
    };

    class ShaderClipRow : public juce::Component
    {
    public:
        ShaderClipRow(WaviateScriptAudioProcessor& processor,
                      const waviate::audio::WaviateAudioCacheEntryInfo& info,
                      std::function<void()> onUpdate)
            : processor(processor), info(info), onUpdate(onUpdate)
        {
            juce::File file(info.location);
            nameLabel.setText(file.getFileName() + " (Shader)", juce::dontSendNotification);
            nameLabel.setFont(juce::Font(12.0f, juce::Font::bold));
            addAndMakeVisible(nameLabel);

            pathLabel.setText(info.location, juce::dontSendNotification);
            pathLabel.setFont(juce::Font(9.0f));
            addAndMakeVisible(pathLabel);

            juce::String statusText = "Pending";
            juce::Colour statusColour = juce::Colours::orange;

            if (info.state == waviate::audio::WaviateAudioLoadState::Loading)
            {
                statusText = "Loading...";
            }
            else if (info.state == waviate::audio::WaviateAudioLoadState::Ready)
            {
                statusText = "Ready";
                statusColour = juce::Colours::lightgreen;
                nameLabel.setTooltip(juce::String("Channels: ") + juce::String(info.channelCount) 
                    + "\nSample Rate: " + juce::String(info.sampleRate) + " Hz"
                    + "\nFrames: " + juce::String(info.frameCount));
            }
            else if (info.state == waviate::audio::WaviateAudioLoadState::Failed)
            {
                statusText = "Failed";
                statusColour = juce::Colours::red;
                nameLabel.setTooltip(info.errorMessage);
            }

            statusLabel.setText(statusText, juce::dontSendNotification);
            statusLabel.setFont(juce::Font(10.0f, juce::Font::italic));
            statusLabel.setColour(juce::Label::textColourId, statusColour);
            addAndMakeVisible(statusLabel);

            deleteButton.setButtonText("X");
            deleteButton.onClick = [this] { removeClip(); };
            addAndMakeVisible(deleteButton);
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            auto deleteArea = bounds.removeFromRight(20);
            deleteButton.setBounds(deleteArea.reduced(2));

            auto top = bounds.removeFromTop(24);
            nameLabel.setBounds(top.reduced(2));

            auto halfWidth = bounds.getWidth() / 2;
            pathLabel.setBounds(bounds.removeFromLeft(halfWidth).reduced(2));
            statusLabel.setBounds(bounds.reduced(2));
        }

    private:
        void removeClip()
        {
            processor.getAudioCache().removeManualClip(info.location);
            onUpdate();
        }

        WaviateScriptAudioProcessor& processor;
        waviate::audio::WaviateAudioCacheEntryInfo info;
        std::function<void()> onUpdate;

        juce::Label nameLabel;
        juce::Label pathLabel;
        juce::Label statusLabel;
        juce::TextButton deleteButton;
    };

    WaviateScriptAudioProcessor& processor;
    std::function<void()> onRecompileNeeded;

    juce::Label titleLabel;
    juce::TextButton addButton;
    juce::Viewport viewport;
    juce::Component listContent;
    juce::Label memoryLabel;
    juce::TextButton clearButton;
};
