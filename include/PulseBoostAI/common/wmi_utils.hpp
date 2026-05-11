#pragma once

#include <Windows.h>
#include <Wbemidl.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

class WmiSession {
public:
    WmiSession();
    ~WmiSession();

    WmiSession(const WmiSession &) = delete;
    WmiSession &operator=(const WmiSession &) = delete;

    bool isReady() const;
    std::vector<std::unordered_map<std::string, std::string>> query(const std::wstring &wql,
                                                                    const std::vector<std::wstring> &fields) const;

private:
    IWbemLocator *locator_ = nullptr;
    IWbemServices *services_ = nullptr;
    bool ready_ = false;
    bool initializedCom_ = false;
};

inline WmiSession::WmiSession() {
    const HRESULT initHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    initializedCom_ = SUCCEEDED(initHr);
    if (!initializedCom_ && initHr != RPC_E_CHANGED_MODE) {
        return;
    }

    const HRESULT securityHr =
        CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
                             nullptr, EOAC_NONE, nullptr);
    if (FAILED(securityHr) && securityHr != RPC_E_TOO_LATE) {
        return;
    }

    if (FAILED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator,
                                reinterpret_cast<void **>(&locator_)))) {
        return;
    }

    BSTR resource = SysAllocString(L"ROOT\\CIMV2");
    const HRESULT connectHr =
        locator_->ConnectServer(resource, nullptr, nullptr, nullptr, 0, nullptr, nullptr, &services_);
    SysFreeString(resource);
    if (FAILED(connectHr)) {
        return;
    }

    const HRESULT blanketHr =
        CoSetProxyBlanket(services_, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
                          RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
    if (FAILED(blanketHr)) {
        return;
    }

    ready_ = true;
}

inline WmiSession::~WmiSession() {
    if (services_ != nullptr) {
        services_->Release();
    }
    if (locator_ != nullptr) {
        locator_->Release();
    }
    if (initializedCom_) {
        CoUninitialize();
    }
}

inline bool WmiSession::isReady() const {
    return ready_;
}

inline std::vector<std::unordered_map<std::string, std::string>> WmiSession::query(
    const std::wstring &wql, const std::vector<std::wstring> &fields) const {
    std::vector<std::unordered_map<std::string, std::string>> rows;
    if (!ready_) {
        return rows;
    }

    IEnumWbemClassObject *enumerator = nullptr;
    BSTR language = SysAllocString(L"WQL");
    BSTR queryText = SysAllocString(wql.c_str());
    const HRESULT queryHr =
        services_->ExecQuery(language, queryText, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr,
                             &enumerator);
    SysFreeString(language);
    SysFreeString(queryText);
    if (FAILED(queryHr) || enumerator == nullptr) {
        return rows;
    }

    IWbemClassObject *object = nullptr;
    ULONG returned = 0;
    while (enumerator->Next(WBEM_INFINITE, 1, &object, &returned) == WBEM_S_NO_ERROR) {
        std::unordered_map<std::string, std::string> row;
        for (const auto &field : fields) {
            VARIANT value;
            VariantInit(&value);
            if (SUCCEEDED(object->Get(field.c_str(), 0, &value, nullptr, nullptr))) {
                switch (value.vt) {
                case VT_BSTR:
                    row[fromWide(field)] = value.bstrVal == nullptr ? std::string {} : fromWide(std::wstring(value.bstrVal));
                    break;
                case VT_I4:
                case VT_INT:
                    row[fromWide(field)] = std::to_string(value.intVal);
                    break;
                case VT_UI4:
                    row[fromWide(field)] = std::to_string(value.uintVal);
                    break;
                case VT_BOOL:
                    row[fromWide(field)] = value.boolVal == VARIANT_TRUE ? std::string("true") : std::string("false");
                    break;
                default:
                    break;
                }
            }
            VariantClear(&value);
        }
        rows.push_back(std::move(row));
        object->Release();
        object = nullptr;
    }

    enumerator->Release();
    return rows;
}

}  // namespace pulseboost
