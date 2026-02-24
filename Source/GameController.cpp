/*
  ==============================================================================

    GameController.cpp
    Created: 23 Feb 2026 9:59:45pm
    Author:  ikamo

  ==============================================================================
*/

#include "GameController.h"
#include "PluginProcessor.h"
#include "LockFreeFifo.h"
#include "GameControllerInput.h"

void GameControllerInterface::timerCallback()
{
    //TODO poll for controller events in SDL 2
    GameControllerEvent event;
    processCue.push(event);
}

GameControllerInterface::GameControllerInterface(SpscEventQueue<GameControllerEvent, 10, true>& proc) : processCue(proc)
{
    startTimer(4);
    //TODO initialize SDL2
}

std::optional<GameControllerEvent> GameControllerInterface::getInputEvent()
{
    GameControllerEvent event;
    bool opt = processCue.popOne(event);
    return opt ? std::make_optional(event) : std::nullopt;
}
