/*
  ==============================================================================

    GameController.h
    Created: 23 Feb 2026 9:59:45pm
    Author:  ikamo

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <optional>


struct GameControllerEvent;

class GameControllerInterface : juce::Timer {
    // Inherited via Timer
    void timerCallback() override;
    class SpscEventQueue<GameControllerEvent, 10, true>& processCue;
public:
    GameControllerInterface(SpscEventQueue<GameControllerEvent, 10, true> & proc);
    std::optional<GameControllerEvent> getInputEvent();
};