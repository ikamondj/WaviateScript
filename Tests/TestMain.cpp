#include <juce_core/juce_core.h>

#include <iostream>
#include <vector>

namespace
{
class ConsoleLogger final : public juce::Logger
{
    void logMessage(const juce::String& message) override
    {
        std::cout << message << std::endl;

       #if JUCE_WINDOWS
        juce::Logger::outputDebugString(message);
       #endif
    }
};

class ConsoleUnitTestRunner final : public juce::UnitTestRunner
{
    void logMessage(const juce::String& message) override
    {
        juce::Logger::writeToLog(message);
    }
};
} // namespace

int main(int argc, char** argv)
{
    constexpr auto helpOption = "--help|-h";
    constexpr auto listOption = "--list-categories|-l";
    constexpr auto categoryOption = "--category|-c";
    constexpr auto nameOption = "--name|-n";
    constexpr auto seedOption = "--seed|-s";

    juce::ArgumentList args(argc, argv);
    if (args.containsOption(helpOption))
    {
        std::cout << argv[0]
                  << " [" << helpOption << "]"
                  << " [" << listOption << "]"
                  << " [" << categoryOption << "=category]"
                  << " [" << nameOption << "=name]"
                  << " [" << seedOption << "=seed]"
                  << std::endl;
        return 0;
    }

    if (args.containsOption(listOption))
    {
        for (const auto& category : juce::UnitTest::getAllCategories())
            std::cout << category << std::endl;

        return 0;
    }

    ConsoleLogger logger;
    juce::Logger::setCurrentLogger(&logger);

    const juce::ScopeGuard cleanup([] {
        juce::Logger::setCurrentLogger(nullptr);
    });

    ConsoleUnitTestRunner runner;
    const auto seed = [&]() -> juce::int64
    {
        if (args.containsOption(seedOption))
        {
            const auto seedValue = args.getValueForOption(seedOption);
            return seedValue.startsWith("0x") ? seedValue.getHexValue64() : seedValue.getLargeIntValue();
        }

        return juce::Random::getSystemRandom().nextInt64();
    }();

    if (args.containsOption(categoryOption))
        runner.runTestsInCategory(args.getValueForOption(categoryOption), seed);
    else if (args.containsOption(nameOption))
        runner.runTestsWithName(args.getValueForOption(nameOption), seed);
    else
        runner.runAllTests(seed);

    std::vector<juce::String> failures;
    for (int index = 0; index < runner.getNumResults(); ++index)
    {
        const auto* result = runner.getResult(index);
        if (result->failures <= 0)
            continue;

        const auto testName = result->unitTestName + " / " + result->subcategoryName;
        const auto summary = juce::String(result->failures) + " failure" + (result->failures > 1 ? "s" : "");
        failures.push_back(testName + ": " + summary);
    }

    if (!failures.empty())
    {
        juce::Logger::writeToLog("Test failure summary:");
        for (const auto& failure : failures)
            juce::Logger::writeToLog("  " + failure);

        return 1;
    }

    juce::Logger::writeToLog("All tests completed successfully");
    return 0;
}
