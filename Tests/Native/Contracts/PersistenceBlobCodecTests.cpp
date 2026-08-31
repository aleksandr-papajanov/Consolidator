#include "TestSupport.h"

#include <limits>
#include <string>
#include <vector>

#include "PersistenceBlobCodec.h"

namespace consolidator::tests
{

bool RunPersistenceBlobCodecTests()
{
    using consolidator::max::PersistenceBlobCodec;

    std::string payload = "{\"schema\":1}";
    payload.push_back('\0');
    payload.push_back(static_cast<char>(0xFF));
    const auto encoded = PersistenceBlobCodec::Encode(payload);

    auto succeeded = true;
    succeeded &= Expect(
        encoded.has_value(),
        "Persistence payload was not encoded.");
    if (!encoded)
    {
        return false;
    }

    succeeded &= Expect(
        encoded->size() == 2 + (payload.size() + 5) / 6,
        "Persistence payload did not use the compact atom representation.");
    const auto decoded = PersistenceBlobCodec::Decode(*encoded);
    succeeded &= Expect(
        decoded.has_value() && *decoded == payload,
        "Persistence payload did not survive an atom round trip.");

    auto malformedMagic = *encoded;
    malformedMagic[0] = 0;
    succeeded &= Expect(
        !PersistenceBlobCodec::Decode(malformedMagic),
        "Persistence payload with invalid magic was accepted.");

    auto truncated = *encoded;
    truncated.pop_back();
    succeeded &= Expect(
        !PersistenceBlobCodec::Decode(truncated),
        "Truncated persistence payload was accepted.");

    auto invalidChunk = *encoded;
    invalidChunk.back() = 281474976710656.0;
    succeeded &= Expect(
        !PersistenceBlobCodec::Decode(invalidChunk),
        "Out-of-range persistence atom was accepted.");

    auto nonCanonicalTail = *encoded;
    nonCanonicalTail.back() += 1099511627776.0;
    succeeded &= Expect(
        !PersistenceBlobCodec::Decode(nonCanonicalTail),
        "Persistence payload with hidden trailing bytes was accepted.");

    auto fractional = *encoded;
    fractional.back() += 0.5;
    succeeded &= Expect(
        !PersistenceBlobCodec::Decode(fractional),
        "Persistence payload with a fractional atom was accepted.");

    auto nonFinite = *encoded;
    nonFinite.back() = std::numeric_limits<double>::quiet_NaN();
    succeeded &= Expect(
        !PersistenceBlobCodec::Decode(nonFinite),
        "Persistence payload with a non-finite atom was accepted.");

    const std::vector<double> tooManyAtoms(0x10000, 0.0);
    succeeded &= Expect(
        !PersistenceBlobCodec::Decode(tooManyAtoms),
        "Persistence payload exceeded the Max attribute atom limit.");

    const std::string maximumSized(
        PersistenceBlobCodec::MaximumPayloadBytes,
        'x');
    const auto maximumEncoded = PersistenceBlobCodec::Encode(maximumSized);
    succeeded &= Expect(
        maximumEncoded.has_value() && maximumEncoded->size() <= 0xFFFF,
        "Maximum persistence payload exceeded the Max attribute atom limit.");

    const std::string oversized(
        PersistenceBlobCodec::MaximumPayloadBytes + 1,
        'x');
    succeeded &= Expect(
        !PersistenceBlobCodec::Encode(oversized),
        "Oversized persistence payload was accepted.");
    return succeeded;
}

} // namespace consolidator::tests
