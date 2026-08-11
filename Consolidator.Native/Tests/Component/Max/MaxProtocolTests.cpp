#include "Max/Protocol/AtomCommandDecoder.h"
#include "Max/Protocol/AtomPathCodec.h"
#include "Max/Protocol/AtomResponseEncoder.h"
#include "Max/Protocol/AtomValueCodec.h"
#include "Max/Protocol/MaxProtocolAdapter.h"
#include "Support/TestFramework.h"

#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

using namespace consolidator;
using namespace consolidator::max;
using namespace c74::min;

namespace
{

atoms ReadFrame(const char* source, const char* request, int count,
                std::initializer_list<atom> pathAtoms)
{
    atoms frame{
        atom{kProtocolVersion}, atom{source}, atom{request}, atom{count}};
    frame.emplace_back("query");
    frame.insert(frame.end(), pathAtoms.begin(), pathAtoms.end());
    return frame;
}

std::string Text(const atom& value)
{
    return static_cast<std::string>(value);
}

} // namespace

TEST_CASE("Command decoder uses explicit query and entry framing")
{
    AtomCommandDecoder decoder;
    const atoms args{
        atom{kProtocolVersion}, atom{"ui"}, atom{"10"}, atom{2},
        atom{"query"}, atom{"compressor"},
        atom{"query"}, atom{"equalizer"}};

    const auto result = decoder.Decode(
        symbol("read"), args, core::InstanceId{7}, core::RequestId{21});

    EXPECT_TRUE(result.command.has_value());
    EXPECT_FALSE(result.error.has_value());
    const auto& command = std::get<core::ReadStateCommand>(*result.command);
    EXPECT_EQ(command.queries.size, 2U);
    EXPECT_EQ(command.queries.entries[0].path.GetDeviceId(),
              dsp::DeviceId::Compressor);
    EXPECT_EQ(command.queries.entries[1].path.GetDeviceId(),
              dsp::DeviceId::Equalizer);
}

TEST_CASE("Command decoder rejects unknown tokens inside an explicit path")
{
    AtomCommandDecoder decoder;
    const atoms args = ReadFrame(
        "ui", "10", 1, {atom{"compressor"}, atom{"unknown"}});

    const auto result = decoder.Decode(
        symbol("read"), args, core::InstanceId{7}, core::RequestId{21});

    EXPECT_FALSE(result.command.has_value());
    EXPECT_TRUE(result.error.has_value());
    EXPECT_EQ(result.error->code, "invalid_path");
}

TEST_CASE("Command decoder rejects non-canonical wire IDs")
{
    AtomCommandDecoder decoder;
    const auto result = decoder.Decode(
        symbol("read"),
        ReadFrame("ui", "01", 1, {atom{"compressor"}}),
        core::InstanceId{7},
        core::RequestId{21});

    EXPECT_FALSE(result.command.has_value());
    EXPECT_TRUE(result.error.has_value());
    EXPECT_EQ(result.error->code, "malformed");
}

TEST_CASE("Command decoder accepts an empty read batch as a full snapshot")
{
    AtomCommandDecoder decoder;
    const atoms args{
        atom{kProtocolVersion}, atom{"ui"}, atom{"10"}, atom{0}};

    const auto result = decoder.Decode(
        symbol("read"), args, core::InstanceId{7}, core::RequestId{21});

    EXPECT_TRUE(result.command.has_value());
    EXPECT_FALSE(result.error.has_value());
    const auto& command = std::get<core::ReadStateCommand>(*result.command);
    EXPECT_EQ(command.queries.size, 0U);
}

TEST_CASE("Command decoder accepts an empty write batch")
{
    AtomCommandDecoder decoder;
    const atoms args{
        atom{kProtocolVersion}, atom{"ui"}, atom{"10"}, atom{0}};

    const auto result = decoder.Decode(
        symbol("write"), args, core::InstanceId{7}, core::RequestId{21});

    EXPECT_TRUE(result.command.has_value());
    EXPECT_FALSE(result.error.has_value());
    const auto& command = std::get<core::WriteStateCommand>(*result.command);
    EXPECT_EQ(command.entries.size, 0U);
}

TEST_CASE("Command decoder rejects an empty source")
{
    AtomCommandDecoder decoder;
    const atoms args{
        atom{kProtocolVersion}, atom{""}, atom{"10"}, atom{0}};

    const auto result = decoder.Decode(
        symbol("read"), args, core::InstanceId{7}, core::RequestId{21});

    EXPECT_FALSE(result.command.has_value());
    EXPECT_TRUE(result.error.has_value());
    EXPECT_EQ(result.error->code, "unknown_source");
}

TEST_CASE("Path codec preserves semantic topology and DSP paths")
{
    AtomPathCodec codec;
    const atoms input{
        atom{"equalizer"}, atom{"bank"}, atom{2}, atom{"filter"}, atom{3},
        atom{"gain"}};
    std::size_t position = 0;
    const auto decoded = codec.Decode(
        input, position, input.size(), core::InstanceId{4});

    EXPECT_TRUE(decoded.has_value());
    EXPECT_EQ(position, input.size());
    EXPECT_TRUE(decoded->deviceId.has_value());
    EXPECT_TRUE(decoded->parameterId.has_value());
    EXPECT_EQ(*decoded->deviceId, dsp::DeviceId::Equalizer);
    EXPECT_EQ(*decoded->parameterId, dsp::ParameterId::Gain);
    EXPECT_EQ(decoded->depth, 2U);

    const auto groupPath = core::StatePath::BankGroup(
        core::InstanceId{4}, dsp::BankId::Bank2);
    atoms encoded;
    codec.Encode(encoded, groupPath);
    EXPECT_EQ(Text(encoded[0]), "bank");
    EXPECT_EQ(static_cast<int>(encoded[1]), 3);
    EXPECT_EQ(Text(encoded[2]), "group");
}

TEST_CASE("Value codec uses strict typed values and wire symbols")
{
    AtomValueCodec codec;
    atoms encoded;
    codec.Encode(encoded, core::StateValue{core::InstanceId{42}});

    EXPECT_EQ(encoded.size(), 1U);
    EXPECT_EQ(encoded[0].a_type, c74::max::A_SYM);
    EXPECT_EQ(Text(encoded[0]), "42");

    const auto markerPath = core::StatePath::DspMarker(
        dsp::DeviceId::Compressor, core::StateMarkerId::Bypass);
    const auto boolean = codec.Decode(atom{1}, markerPath);
    const auto symbolic = codec.Decode(atom{"true"}, markerPath);
    EXPECT_TRUE(boolean.has_value());
    EXPECT_FALSE(symbolic.has_value());
}

TEST_CASE("Adapter isolates correlation by source and encodes multipart responses")
{
    MaxProtocolAdapter adapter;
    const auto first = adapter.Decode(
        symbol("read"),
        ReadFrame("ui", "10", 1, {atom{"compressor"}}),
        core::InstanceId{7});
    const auto second = adapter.Decode(
        symbol("read"),
        ReadFrame("other", "10", 1, {atom{"compressor"}}),
        core::InstanceId{8});
    const auto duplicate = adapter.Decode(
        symbol("read"),
        ReadFrame("ui", "10", 1, {atom{"compressor"}}),
        core::InstanceId{7});

    EXPECT_TRUE(first.command.has_value());
    EXPECT_TRUE(second.command.has_value());
    EXPECT_FALSE(duplicate.command.has_value());
    EXPECT_EQ(duplicate.error->code, "duplicate_request");

    const auto firstRequestId = std::visit(
        [](const auto& command) { return command.requestId; }, *first.command);
    core::ActionResponse response{
        firstRequestId, core::InstanceId{7}, core::ActionStatus::Accepted};
    std::vector<symbol> selectors;
    std::vector<atoms> frames;
    std::optional<DecodeResult> reentrantRequest;
    adapter.EncodeResponse(
        response,
        [&](symbol selector, const atoms& values)
        {
            selectors.push_back(selector);
            frames.push_back(values);
            if (selector == symbol("action_done"))
            {
                reentrantRequest = adapter.Decode(
                    symbol("read"),
                    ReadFrame("ui", "10", 1, {atom{"compressor"}}),
                    core::InstanceId{7});
            }
        });

    EXPECT_EQ(selectors.size(), 1U);
    EXPECT_EQ(Text(atom{selectors[0]}), "action_done");
    EXPECT_EQ(static_cast<int>(frames[0][0]), kProtocolVersion);
    EXPECT_EQ(Text(frames[0][1]), "ui");
    EXPECT_EQ(Text(frames[0][2]), "10");
    EXPECT_EQ(Text(frames[0][3]), "7");
    EXPECT_EQ(Text(frames[0][4]), "accepted");
    EXPECT_TRUE(reentrantRequest.has_value());
    EXPECT_TRUE(reentrantRequest->command.has_value());
}

TEST_CASE("Adapter encodes complete state response framing")
{
    MaxProtocolAdapter adapter;
    const auto decoded = adapter.Decode(
        symbol("read"),
        ReadFrame("ui", "12", 1, {atom{"compressor"}}),
        core::InstanceId{7});
    EXPECT_TRUE(decoded.command.has_value());

    const auto requestId = std::visit(
        [](const auto& command) { return command.requestId; }, *decoded.command);
    core::StateResponse response{requestId, core::InstanceId{7}, {}, true};
    const auto fallbackPath = core::StatePath::Device(
                                  dsp::DeviceId::Compressor)
                                  .WithParameter(dsp::ParameterId::Threshold);
    const auto targetPath = fallbackPath.WithInstance(core::InstanceId{11});
    (void)response.entries.TryAppend(
        core::StateEntry{fallbackPath, core::StateValue{1.0f}});
    (void)response.entries.TryAppend(
        core::StateEntry{targetPath, core::StateValue{2.0f}});
    response.entries.entries[0].status = core::StateWriteStatus::Unchanged;
    response.entries.entries[0].physicalMinimum = -60.0f;
    response.entries.entries[0].physicalMaximum = 12.0f;
    response.entries.entries[0].minimum = -30.0f;
    response.entries.entries[0].maximum = 6.0f;

    std::vector<symbol> selectors;
    std::vector<atoms> frames;
    adapter.EncodeResponse(
        response,
        [&](symbol selector, const atoms& frame)
        {
            selectors.push_back(selector);
            frames.push_back(frame);
        });

    EXPECT_EQ(selectors.size(), 4U);
    EXPECT_EQ(Text(atom{selectors[0]}), "state_begin");
    EXPECT_EQ(Text(atom{selectors[1]}), "state_entry");
    EXPECT_EQ(Text(atom{selectors[2]}), "state_entry");
    EXPECT_EQ(Text(atom{selectors[3]}), "state_done");

    EXPECT_EQ(static_cast<int>(frames[0][0]), kProtocolVersion);
    EXPECT_EQ(Text(frames[0][2]), "12");
    EXPECT_EQ(Text(frames[0][3]), "7");
    EXPECT_EQ(static_cast<int>(frames[0][4]), 1);
    EXPECT_EQ(static_cast<int>(frames[0][5]), 2);

    EXPECT_EQ(Text(frames[1][3]), "7");
    EXPECT_EQ(static_cast<int>(frames[1][4]), 0);
    EXPECT_EQ(Text(frames[1][8]), "unchanged");
    EXPECT_EQ(static_cast<float>(frames[1][9]), -60.0f);
    EXPECT_EQ(static_cast<float>(frames[1][10]), 12.0f);
    EXPECT_EQ(static_cast<float>(frames[1][11]), -30.0f);
    EXPECT_EQ(static_cast<float>(frames[1][12]), 6.0f);

    EXPECT_EQ(Text(frames[2][3]), "11");
    EXPECT_EQ(static_cast<int>(frames[2][4]), 1);
    EXPECT_EQ(Text(frames[3][2]), "12");
    EXPECT_EQ(Text(frames[3][3]), "7");
}

TEST_MAIN()
