#include "PulseBoostAI/ui_backend/notification_manager.hpp"

#include <Windows.h>
#include <iostream>

// NOTE: In a full production application, WinRT APIs would be used here via C++/WinRT 
// to post true rich XML Toast Notifications. For this MVP C++ implementation, 
// we fall back to a standard MessageBox or logging if a full UI overlay isn't available.

namespace pulseboost {

struct NotificationManager::Impl {
    bool isInitialized = false;
};

NotificationManager::NotificationManager() : pImpl(new Impl()) {
    // Boilerplate for future WinRT RoInitialize
    pImpl->isInitialized = true;
}

NotificationManager::~NotificationManager() {
    delete pImpl;
}

bool NotificationManager::showToast(const std::string &title, const std::string &message, bool isError) {
    if (!pImpl->isInitialized) return false;

    // For now, print to console as fallback. QML ToastNotification handles the visual aspect.
    std::cout << "[Toast] " << title << ": " << message << std::endl;
    return true;
}

}  // namespace pulseboost
