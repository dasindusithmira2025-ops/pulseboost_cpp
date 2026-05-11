#pragma once

#include <string>

namespace pulseboost {

/// Handles pushing native Windows 10/11 Toast Notifications to the Action Center.
class NotificationManager {
public:
    NotificationManager();
    ~NotificationManager();

    /// Displays a native Windows toast notification
    bool showToast(const std::string &title, const std::string &message, bool isError = false);

private:
    struct Impl;
    Impl* pImpl;
};

}  // namespace pulseboost
