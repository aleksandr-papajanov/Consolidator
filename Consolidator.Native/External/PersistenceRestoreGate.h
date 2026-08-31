#pragma once

namespace consolidator::max
{

class PersistenceRestoreGate
{
public:
    void BeginLocalChangeNotification()
    {
        localChangeNotificationActive_ = true;
    }

    void EndLocalChangeNotification()
    {
        localChangeNotificationActive_ = false;
    }

    [[nodiscard]] bool ShouldRestore() const
    {
        return !localChangeNotificationActive_;
    }

private:
    bool localChangeNotificationActive_{};
};

} // namespace consolidator::max
