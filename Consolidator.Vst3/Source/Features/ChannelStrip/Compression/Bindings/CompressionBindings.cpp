#include "Features/ChannelStrip/Compression/Bindings/CompressionBindings.h"

#include <algorithm>
#include <stdexcept>

namespace consolidator::vst3::features::channelstrip::compression::bindings
{

void RefreshAvailability(shared::controls::parameter::bindings::ParameterBindings& parameters)
{
    const auto& models = parameters.Models();
    const auto type = std::find_if(models.begin(), models.end(), [](const auto& model)
    {
        return model->State().Path() == "compression.type";
    });
    if (type == models.end())
    {
        throw std::logic_error("Channel strip has no compression type binding.");
    }
    const bool timingAvailable = (*type)->ChoiceValue() < 2;
    for (const auto& model : models)
    {
        const auto& path = model->State().Path();
        if (path == "compression.attack" || path == "compression.release" || path == "compression.ratio")
        {
            model->State().SetAvailable(timingAvailable);
        }
    }
}

} // namespace consolidator::vst3::features::channelstrip::compression::bindings
