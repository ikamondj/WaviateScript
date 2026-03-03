/*
  ==============================================================================

    RustFileTemplateGenerator.h
    Created: 27 Feb 2026 1:48:01am
    Author:  ikamo

  ==============================================================================
*/

#pragma once
#ifndef WAV_SCRIPT_PREMIUM
#error "Rust support is only available in the premium version of Waviate Script."
#endif
#include "NewFileTemplateGenerator.h"
class RustFileTemplateGenerator : public NewFileTemplateGenerator {
public:
    std::string getDefaultFileSource() override;
};