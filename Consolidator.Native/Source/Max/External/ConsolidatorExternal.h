#pragma once

#include <atomic>

#include "Core/Instance/ConsolidatorInstance.h"
#include "../Protocol/MaxProtocolAdapter.h"
#include "c74_min_api.h"

namespace consolidator::max
{

class ConsolidatorExternal : public c74::min::object<ConsolidatorExternal>,
                             public c74::min::vector_operator<>
{
public:
    MIN_DESCRIPTION { "Thin Max protocol adapter for Consolidator Core." };
    MIN_TAGS { "audio" };
    MIN_AUTHOR { "Consolidator" };

    c74::min::inlet<> controlInput { this, "Protocol commands." };
    c74::min::inlet<> mainInputLeft {
        this, "Main input left.", "signal" };
    c74::min::inlet<> mainInputRight {
        this, "Main input right.", "signal" };
    c74::min::inlet<> referenceInputLeft {
        this, "Reference input left.", "signal" };
    c74::min::inlet<> referenceInputRight {
        this, "Reference input right.", "signal" };

    c74::min::outlet<> controlOutput { this, "Protocol responses." };
    c74::min::outlet<> mainOutputLeft {
        this, "Main output left.", "signal" };
    c74::min::outlet<> mainOutputRight {
        this, "Main output right.", "signal" };
    c74::min::outlet<> referenceOutputLeft {
        this, "Reference output left.", "signal" };
    c74::min::outlet<> referenceOutputRight {
        this, "Reference output right.", "signal" };

    c74::min::message<> read { this, "read", "Read state through the protocol.",
        MIN_FUNCTION { HandleProtocolMessage(c74::min::symbol("read"), args); return {}; } };
    c74::min::message<> write { this, "write", "Write state through the protocol.",
        MIN_FUNCTION { HandleProtocolMessage(c74::min::symbol("write"), args); return {}; } };
    c74::min::message<> reset { this, "reset", "Reset DSP through the protocol.",
        MIN_FUNCTION { HandleProtocolMessage(c74::min::symbol("reset"), args); return {}; } };
    c74::min::message<> dspsetup { this, "dspsetup",
        "Prepare DSP for the host sample rate.",
        MIN_FUNCTION
        {
            instance_.Prepare(static_cast<double>(args[0]));
            return {};
        } };

    ConsolidatorExternal();
    ~ConsolidatorExternal();

    void operator()(c74::min::audio_bundle input,
                    c74::min::audio_bundle output) override;

private:
    void HandleProtocolMessage(c74::min::symbol selector,
                               const c74::min::atoms& args);
    void NotifyResponseAvailable();
    void DrainResponses();
    void EmitProtocolError(const ProtocolError& error);

    core::ConsolidatorInstance instance_;
    MaxProtocolAdapter protocol_;
    c74::min::queue<> responseQueue_;
    std::atomic_bool responseDispatchPending_{false};
    std::atomic_bool acceptingResponses_{true};
};

} // namespace consolidator::max
