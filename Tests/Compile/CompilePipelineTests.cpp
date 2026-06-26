#include "../TestSupport/WaviateUnitTest.h"
#include "CompileTestHelpers.h"
#include "WaviateSafety.h"

#include <cmath>

namespace
{
constexpr float epsilon = 0.0001f;

bool nearlyEqual(const float lhs, const float rhs, const float tolerance = epsilon)
{
    return std::abs(lhs - rhs) <= tolerance;
}
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

WAVIATE_TEST_CASE(CompilePipelineCoreFunctionTest, "Compile Pipeline Core Functions", "Compile")
{
    using namespace waviate::tests::compile;

    WAVIATE_TEST("basic waviate core waveform helpers return expected values");
    const auto waveformResult = compileSource("waveform_helpers", R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    return wav.sine(0.25f)
         + wav.saw(0.75f)
         + wav.square(0.10f)
         + wav.pulse(0.20f, 0.30f);
}
)wlsl");
    expectCompileSuccess(*this, waveformResult);
    if (waveformResult)
        WAVIATE_EXPECT(nearlyEqual(invokeSample(waveformResult), 3.5f));

    WAVIATE_TEST("sample rate and time conversion helpers return expected values");
    const auto timingResult = compileSource("timing_helpers", R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    return wav.samplesToSeconds(24000ULL) + static_cast<float>(wav.secondsToSamples(0.5f));
}
)wlsl");
    expectCompileSuccess(*this, timingResult);
    if (timingResult)
        WAVIATE_EXPECT(nearlyEqual(invokeSample(timingResult, { .sampleRate = 48000.0f }), 24000.5f));
}

WAVIATE_TEST_CASE(CompilePipelineDeterminismTest, "Compile Pipeline Determinism", "Compile")
{
    using namespace waviate::tests::compile;

    WAVIATE_TEST("perlin output is deterministic across invocations and recompiles");
    const auto perlinSource = juce::String(R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    return wav.perlin(12.345f) + wav.perlin(6.1725f);
}
)wlsl");
    const auto perlinFirst = compileSource("perlin_determinism_a", perlinSource);
    const auto perlinSecond = compileSource("perlin_determinism_b", perlinSource);
    expectCompileSuccess(*this, perlinFirst);
    expectCompileSuccess(*this, perlinSecond);
    if (perlinFirst && perlinSecond)
    {
        const auto firstRun = invokeSample(perlinFirst);
        const auto secondRun = invokeSample(perlinFirst);
        const auto recompiledRun = invokeSample(perlinSecond);
        WAVIATE_EXPECT(nearlyEqual(firstRun, secondRun, 0.000001f));
        WAVIATE_EXPECT(nearlyEqual(firstRun, recompiledRun, 0.000001f));
    }

    WAVIATE_TEST("voronoi output is deterministic across invocations and recompiles");
    const auto voronoiSource = juce::String(R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    return wav.voronoi(4.250f) + wav.voronoi(9.875f);
}
)wlsl");
    const auto voronoiFirst = compileSource("voronoi_determinism_a", voronoiSource);
    const auto voronoiSecond = compileSource("voronoi_determinism_b", voronoiSource);
    expectCompileSuccess(*this, voronoiFirst);
    expectCompileSuccess(*this, voronoiSecond);
    if (voronoiFirst && voronoiSecond)
    {
        const auto firstRun = invokeSample(voronoiFirst);
        const auto secondRun = invokeSample(voronoiFirst);
        const auto recompiledRun = invokeSample(voronoiSecond);
        WAVIATE_EXPECT(nearlyEqual(firstRun, secondRun, 0.000001f));
        WAVIATE_EXPECT(nearlyEqual(firstRun, recompiledRun, 0.000001f));
    }
}

WAVIATE_TEST_CASE(CompilePipelineSampleInputFlowTest, "Compile Pipeline Sample Input Flow", "Compile")
{
    using namespace waviate::tests::compile;

    WAVIATE_TEST("waviate sample exposes input metadata and current buffers");
    const auto metadataResult = compileSource("sample_metadata_flow", R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    const float metadata = static_cast<float>(wav.getChannel())
        + static_cast<float>(wav.getSampleInBlock()) * 10.0f
        + static_cast<float>(wav.getBlockSize()) * 100.0f
        + static_cast<float>(wav.getInputChannelCount()) * 1000.0f
        + static_cast<float>(wav.getChannelCount()) * 10000.0f
        + wav.getSampleRate() * 0.001f
        + static_cast<float>(wav.getSamplesSinceAppStart()) * 0.01f;

    return metadata + wav.getIncomingSample() + wav.getCurrentSample();
}
)wlsl");
    expectCompileSuccess(*this, metadataResult);
    if (metadataResult)
    {
        SampleInvocation invocation;
        invocation.incomingSample = 0.25f;
        invocation.currentSample = 0.75f;
        invocation.channel = 1;
        invocation.sampleInBlock = 2;
        invocation.blockSize = 4;
        invocation.inputChannelCount = 2;
        invocation.outputChannelCount = 3;
        invocation.sampleRate = 48000.0f;
        invocation.samplesSinceAppStart = 123;
        WAVIATE_EXPECT(nearlyEqual(invokeSample(metadataResult, invocation), 32470.23f + 1.0f));
    }

    WAVIATE_TEST("waviate sample supports indexed channel reads and current sample writes");
    const auto bufferFlowResult = compileSource("sample_buffer_flow", R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    const float mixed = wav.getIncomingSample(0, 1)
        + wav.getIncomingSample(1, 2)
        + wav.getCurrentSample(2, 0);
    wav.setCurrentSample(mixed);
    return wav.getCurrentSample();
}
)wlsl");
    expectCompileSuccess(*this, bufferFlowResult);
    if (bufferFlowResult)
    {
        SampleInvocation invocation;
        invocation.channel = 2;
        invocation.sampleInBlock = 0;
        invocation.blockSize = 3;
        invocation.inputChannels = {
            { 0.10f, 0.25f, 0.30f },
            { 0.40f, 0.50f, 0.75f }
        };
        invocation.outputChannels = {
            { 1.00f, 1.00f, 1.00f },
            { 2.00f, 2.00f, 2.00f },
            { 3.50f, 3.50f, 3.50f }
        };

        const auto execution = executeSample(bufferFlowResult, invocation);
        WAVIATE_EXPECT(nearlyEqual(execution.returnValue, 4.5f));
        WAVIATE_EXPECT(nearlyEqual(execution.selectedOutputSample, 4.5f));
        WAVIATE_EXPECT(nearlyEqual(execution.outputChannels[2][0], 4.5f));
    }

    WAVIATE_TEST("waviate sample exposes midi state deterministically");
    const auto midiResult = compileSource("sample_midi_flow", R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    const float noteState = wav.isMidiNoteOn(64) ? 1.0f : 0.0f;
    const float ccValue = static_cast<float>(wav.getMidiCCValue(7));
    wav.setCurrentSample(noteState + ccValue);
    return wav.getCurrentSample();
}
)wlsl");
    expectCompileSuccess(*this, midiResult);
    if (midiResult)
    {
        SampleInvocation invocation;
        invocation.midiNoteOn[64] = 1;
        invocation.midiCcValue[7] = 99;
        WAVIATE_EXPECT(nearlyEqual(invokeSample(midiResult, invocation), 100.0f));
    }
}

WAVIATE_TEST_CASE(CompilePipelineAudioLoadPipelineTest, "Compile Pipeline Audio Load Pipeline", "Compile")
{
    using namespace waviate::tests::compile;
    using waviate::audio::WaviateAudioCache;
    using waviate::audio::WaviateAudioLoadRequest;

    WAVIATE_TEST("loadAudio returns a silent pending clip and queues the exact location once");
    const auto pendingResult = compileSource("audio_load_pending", R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    auto clip = loadAudio("memory://kick");
    return static_cast<float>(clip.length()) + clip.read(0.5f) + clip.readSample(0);
}
)wlsl");
    expectCompileSuccess(*this, pendingResult);
    if (pendingResult)
    {
        WaviateAudioCache cache;
        SampleInvocation invocation;
        invocation.audioCache = &cache;

        WAVIATE_EXPECT(nearlyEqual(invokeSample(pendingResult, invocation), 1.0f));

        WaviateAudioLoadRequest request;
        WAVIATE_EXPECT(cache.popPendingRequest(request));
        WAVIATE_EXPECT(request.location == "memory://kick");
        WAVIATE_EXPECT(! cache.popPendingRequest(request));
    }

    WAVIATE_TEST("loadAudio reads cached audio after the runtime cache is populated");
    const auto loadedResult = compileSource("audio_load_ready", R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    auto clip = loadAudio("memory://kick");
    return static_cast<float>(clip.length()) + clip.read(0.5f) + clip.readSample(0);
}
)wlsl");
    expectCompileSuccess(*this, loadedResult);
    if (loadedResult)
    {
        WaviateAudioCache cache;
        SampleInvocation invocation;
        invocation.audioCache = &cache;

        static_cast<void>(invokeSample(loadedResult, invocation));

        WaviateAudioLoadRequest request;
        WAVIATE_EXPECT(cache.popPendingRequest(request));
        WAVIATE_EXPECT(cache.storeLoadedAudio("memory://kick", { 0.0f, 1.0f, 0.0f }, 1, 48000.0f));

        WAVIATE_EXPECT(nearlyEqual(invokeSample(loadedResult, invocation), 4.0f));
        WAVIATE_EXPECT(cache.pendingRequestCount() == 0);
    }
}

WAVIATE_TEST_CASE(CompilePipelineFuelMeteringTest, "Compile Pipeline Fuel Metering", "Compile")
{
    using namespace waviate::tests::compile;

    WAVIATE_TEST("normal shaders expose fuel runtime and run under a block budget");
    const auto simpleResult = compileSource("fuel_simple", R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    return wav.getIncomingSample() * 0.5f;
}
)wlsl");
    expectCompileSuccess(*this, simpleResult);
    if (simpleResult)
    {
        WAVIATE_EXPECT(simpleResult.runtime.hasFuelMetering());
        simpleResult.runtime.beginBlock(waviate::compile::calculateFuelBudget(
            waviate::compile::FuelLimitPreset::Minimal,
            1,
            1));

        WAVIATE_EXPECT(nearlyEqual(invokeSample(simpleResult, { .incomingSample = 0.75f }), 0.375f));
        WAVIATE_EXPECT(! simpleResult.runtime.isFuelExhausted());
    }

    WAVIATE_TEST("large loops trip fuel metering and return fallback output");
    const auto loopResult = compileSource("fuel_loop", R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    float x = wav.getIncomingSample() * 0.1f;
    for (int i = 0; i < 2000; ++i)
    {
        x = 3.9f * x * (1.0f - x);
    }
    return x;
}
)wlsl");
    expectCompileSuccess(*this, loopResult);
    if (loopResult)
    {
        WAVIATE_EXPECT(loopResult.runtime.hasFuelMetering());
        loopResult.runtime.beginBlock(64);

        WAVIATE_EXPECT(nearlyEqual(invokeSample(loopResult, { .incomingSample = 0.25f }), 0.0f));
        WAVIATE_EXPECT(loopResult.runtime.isFuelExhausted());
        WAVIATE_EXPECT(loopResult.runtime.fuelRemaining() == 0);

        // Reset fallback fuel state so it doesn't affect other things
        waviate::safety::resetFallbackFuelState();
    }
}

WAVIATE_TEST_CASE(CompilePipelineArenaFacadeTest, "Compile Pipeline Arena Facade", "Compile")
{
    using namespace waviate::tests::compile;

    WAVIATE_TEST("waviate facade containers allocate through the ephemeral arena");
    const auto arenaResult = compileSource("arena_facade", R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    auto values = wav.newArray<float>(4);
    values.set(0, 1.25f);
    values.set(1, 0.75f);

    auto numbers = wav.newVector<float>();
    numbers.push(values.get(0));
    numbers.push(values.get(1));

    auto name = wav.newString("wa");
    name.append('v');

    auto gains = wav.newMap<int, float>();
    gains.insert(7, numbers.get(0) + numbers.get(1));

    return gains.get(7, 0.0f) + static_cast<float>(name.size());
}
)wlsl");
    expectCompileSuccess(*this, arenaResult);
    if (arenaResult)
        WAVIATE_EXPECT(nearlyEqual(invokeSample(arenaResult), 5.0f));

    WAVIATE_TEST("arena exhaustion traps and returns fallback output");
    const auto exhaustedResult = compileSource("arena_exhaustion", R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    auto tooLarge = wav.newArray<float>(40000000);
    return tooLarge.valid() ? 1.0f : 2.0f;
}
)wlsl");
    expectCompileSuccess(*this, exhaustedResult);
    if (exhaustedResult)
    {
        exhaustedResult.runtime.beginBlock(1000000);
        WAVIATE_EXPECT(nearlyEqual(invokeSample(exhaustedResult), 0.0f));
        WAVIATE_EXPECT(exhaustedResult.runtime.isFuelExhausted());
        waviate::safety::resetFallbackFuelState();
    }
}

WAVIATE_TEST_CASE(CompilePipelineSafetyRejectionTest, "Compile Pipeline Safety Rejection", "Compile")
{
    using namespace waviate::tests::compile;

    WAVIATE_TEST("unallowlisted external symbols are rejected even with harmless names");
    const auto externalResult = compileSource("unsafe_external_symbol", R"wlsl(
extern "C" float host_escape();

float SampleProcess(const WaviateSample& wav)
{
    return host_escape();
}
)wlsl");
    expectCompileFailure(*this, externalResult, "external function");

    WAVIATE_TEST("inline assembly is rejected");
    const auto asmResult = compileSource("unsafe_inline_asm", R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    asm volatile("");
    return 0.0f;
}
)wlsl");
    expectCompileFailure(*this, asmResult, "asm");

    WAVIATE_TEST("arbitrary function pointer address calls are rejected");
    const auto pointerResult = compileSource("unsafe_function_pointer_address", R"wlsl(
using HostCall = float (*)();

float SampleProcess(const WaviateSample& wav)
{
    HostCall call = (HostCall)0x12345678ULL;
    return call();
}
)wlsl");
    expectCompileFailure(*this, pointerResult, "function-pointer");

    WAVIATE_TEST("raw allocation syntax is rejected");
    const auto allocationResult = compileSource("unsafe_raw_new", R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    int* value = new int(3);
    return static_cast<float>(*value);
}
)wlsl");
    expectCompileFailure(*this, allocationResult, "new");

    WAVIATE_TEST("mutable global storage is rejected");
    const auto globalResult = compileSource("unsafe_mutable_global", R"wlsl(
float persistent = 1.0f;

float SampleProcess(const WaviateSample& wav)
{
    persistent += 1.0f;
    return persistent;
}
)wlsl");
    expectCompileFailure(*this, globalResult, "global");

    WAVIATE_TEST("dynamic stack allocation is rejected");
    const auto stackResult = compileSource("unsafe_dynamic_stack", R"wlsl(
float SampleProcess(const WaviateSample& wav)
{
    int count = wav.getBlockSize();
    float values[count];
    values[0] = 1.0f;
    return values[0];
}
)wlsl");
    expectCompileFailure(*this, stackResult, "stack");
}
