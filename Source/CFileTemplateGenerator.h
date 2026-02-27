/*
  ==============================================================================

    CFileTemplateGenerator.h
    Created: 27 Feb 2026 1:47:46am
    Author:  ikamo

  ==============================================================================
*/

#pragma once
#include "NewFileTemplateGenerator.h"
class CfileTemplateGenerator : public NewFileTemplateGenerator {
public:
    std::string getDefaultFileSource() override;
};