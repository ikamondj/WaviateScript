#include "../TestSupport/WaviateUnitTest.h"
#include "CompileTestHelpers.h"

#include <cmath>

namespace
{
constexpr float epsilon = 0.0001f;
}

WAVIATE_TEST_CASE(CompilePipelineSmokeTest, "Compile Pipeline Smoke", "Compile")
{
    using namespace waviate::tests::compile;

    WAVIATE_TEST("compile inline sample shader and execute it");
    const auto inlineResult = compileSource("inline_passthrough", R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    return wav.getIncomingSample();
}
)wlsl");
    expectCompileSuccess(*this, inlineResult);
    if (inlineResult)
        WAVIATE_EXPECT(std::abs(invokeSample(inlineResult, { .incomingSample = 0.375f }) - 0.375f) <= epsilon);

    WAVIATE_TEST("compile fixture file through the same pipeline");
    const auto fixtureResult = compileFixture("sample_passthrough.wlsl");
    expectCompileSuccess(*this, fixtureResult);
    if (fixtureResult)
        WAVIATE_EXPECT(std::abs(invokeSample(fixtureResult, { .incomingSample = -0.125f }) + 0.125f) <= epsilon);

    WAVIATE_TEST("capture compile failures cleanly");
    const auto failureResult = compileSource("broken_shader", "float SampleProcess(const WaviateSample& wav) {");
    expectCompileFailure(*this, failureResult);
}
