#pragma once

#include <juce_core/juce_core.h>

#define WAVIATE_TEST_CASE(ClassName, DisplayName, Category) \
class ClassName final : public juce::UnitTest { \
public: \
    ClassName() : juce::UnitTest(DisplayName, Category) {} \
    void runTest() override; \
}; \
static ClassName ClassName##_instance; \
void ClassName::runTest()

#define WAVIATE_TEST(Name) beginTest(Name)
#define WAVIATE_EXPECT(Expr) expect((Expr), #Expr)
#define WAVIATE_EXPECT_MSG(Expr, Message) expect((Expr), (Message))
#define WAVIATE_EXPECT_EQUALS(Actual, Expected) expectEquals((Actual), (Expected))
