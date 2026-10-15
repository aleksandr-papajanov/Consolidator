#include "Features/Tracks/Controllers/GroupEditor.h"

#include <memory>
#include <string>
#include <utility>

namespace consolidator::vst3::features::tracks::controllers
{

GroupEditor::GroupEditor(SendCommand send) : send_(std::move(send)) {}

void GroupEditor::Write(
    std::vector<std::pair<std::uint64_t, int>> banks, shared::protocol::ProtocolValue group, Completion completion)
{
    if (banks.empty())
    {
        completion(true);
        return;
    }
    auto operation = std::make_shared<Operation>(Operation{ std::move(completion), banks.size() });
    for (const auto& selection : banks)
    {
        send_("write", { std::string("topology"), std::to_string(selection.first), std::string("0"),
            std::int64_t{ 1 }, std::string("entry"), std::string("bank"), static_cast<std::int64_t>(selection.second),
            std::string("group"), std::string("value"), group, std::string("copy") },
            [operation](bool accepted)
            {
                operation->failed = operation->failed || !accepted;
                if (--operation->remaining == 0)
                {
                    operation->completed(!operation->failed);
                }
            });
    }
}

} // namespace consolidator::vst3::features::tracks::controllers
