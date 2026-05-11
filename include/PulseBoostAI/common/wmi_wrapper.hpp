#pragma once

#include <string>
#include <optional>

namespace pulseboost {

/// Wraps Windows Management Instrumentation (WMI) COM calls securely.
class WmiWrapper {
public:
    WmiWrapper();
    ~WmiWrapper();

    // Disable copy/move semantics for RAII COM object
    WmiWrapper(const WmiWrapper &) = delete;
    WmiWrapper &operator=(const WmiWrapper &) = delete;
    WmiWrapper(WmiWrapper &&) = delete;
    WmiWrapper &operator=(WmiWrapper &&) = delete;

    /// Connects to a WMI namespace (default: ROOT\CIMV2).
    /// Safe to call multiple times (caches connection).
    bool connect(const std::wstring &wmiNamespace = L"ROOT\\CIMV2");

    /// Executes a WQL query and returns a String property from the first result.
    std::optional<std::wstring> querySingleString(const std::wstring &query, const std::wstring &propertyName);

    /// Executes a WMI class method (e.g. creating a restore point).
    bool executeMethod(const std::wstring &className, const std::wstring &methodName, const std::wstring &parameterName, const std::wstring &parameterValue);

private:
    struct Impl;
    Impl* pImpl;
};

}  // namespace pulseboost
