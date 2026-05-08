#include "../TestSupport/WaviateUnitTest.h"

#include "../../Source/WaviateShaderSerializer.h"

WAVIATE_TEST_CASE(ShaderSerializerSmokeTest, "Shader Serializer Smoke", "Serialization")
{
    WAVIATE_TEST("serialize and deserialize without mutating the source");
    const juce::String source = "float SampleProcess(const WaviateSample& wav) { return wav.getIncomingSample(); }\n";
    WaviateShaderSerializer::Options options;
    options.minifySource = false;
    options.compressPayload = false;
    options.formatOnDeserialize = false;
    options.prettyPrintJson = false;

    const auto serialized = WaviateShaderSerializer::serialize(source, options);
    WAVIATE_EXPECT_MSG(serialized.succeeded, serialized.errorMessage);

    if (!serialized)
        return;

    const auto deserialized = WaviateShaderSerializer::deserialize(serialized.serialized, options);
    WAVIATE_EXPECT_MSG(deserialized.succeeded, deserialized.errorMessage);

    if (deserialized)
        WAVIATE_EXPECT_EQUALS(deserialized.source, source);
}
