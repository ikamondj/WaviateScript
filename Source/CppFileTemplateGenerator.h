/*
  ==============================================================================

    CppFileTemplateGenerator.h
    Created: 27 Feb 2026 1:47:53am
    Author:  ikamo

  ==============================================================================
*/

#pragma once
#include "NewFileTemplateGenerator.h"
class CppFileTemplateGenerator : public NewFileTemplateGenerator {
public:
    std::string getDefaultFileSource() override;
};