/*
  ==============================================================================

    RustFileTemplateGenerator.h
    Created: 27 Feb 2026 1:48:01am
    Author:  ikamo

  ==============================================================================
*/

#pragma once
#include "NewFileTemplateGenerator.h"
class RustFileTemplateGenerator : public NewFileTemplateGenerator {
public:
    std::string getDefaultFileSource() override;
};