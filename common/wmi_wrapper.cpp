#include "PulseBoostAI/common/wmi_wrapper.hpp"

#include <Windows.h>
#include <Wbemidl.h>
#include <comdef.h>

#include <iostream>

#pragma comment(lib, "wbemuuid.lib")

namespace pulseboost {

struct WmiWrapper::Impl {
    IWbemLocator* locator = nullptr;
    IWbemServices* services = nullptr;
    bool comInitialized = false;

    ~Impl() {
        if (services) { services->Release(); }
        if (locator)  { locator->Release(); }
        if (comInitialized) { CoUninitialize(); }
    }
};

WmiWrapper::WmiWrapper() : pImpl(new Impl()) {
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    pImpl->comInitialized = SUCCEEDED(hr);

    if (pImpl->comInitialized) {
        // Set general COM security levels
        CoInitializeSecurity(
            NULL, -1, NULL, NULL,
            RPC_C_AUTHN_LEVEL_DEFAULT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL, EOAC_NONE, NULL
        );
    }
}

WmiWrapper::~WmiWrapper() {
    delete pImpl;
}

bool WmiWrapper::connect(const std::wstring &wmiNamespace) {
    if (!pImpl->comInitialized) return false;
    if (pImpl->services) return true; // Already connected

    HRESULT hr = CoCreateInstance(
        CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pImpl->locator
    );

    if (FAILED(hr)) return false;

    hr = pImpl->locator->ConnectServer(
        _bstr_t(wmiNamespace.c_str()), NULL, NULL, 0, NULL, 0, 0, &pImpl->services
    );

    if (FAILED(hr)) {
        pImpl->locator->Release();
        pImpl->locator = nullptr;
        return false;
    }

    hr = CoSetProxyBlanket(
        pImpl->services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE
    );

    if (FAILED(hr)) {
        pImpl->services->Release();
        pImpl->services = nullptr;
        pImpl->locator->Release();
        pImpl->locator = nullptr;
        return false;
    }

    return true;
}

std::optional<std::wstring> WmiWrapper::querySingleString(const std::wstring &query, const std::wstring &propertyName) {
    if (!pImpl->services) return std::nullopt;

    IEnumWbemClassObject* enumerator = nullptr;
    HRESULT hr = pImpl->services->ExecQuery(
        bstr_t("WQL"), bstr_t(query.c_str()),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &enumerator
    );

    if (FAILED(hr)) return std::nullopt;

    IWbemClassObject* classObject = nullptr;
    ULONG returnCount = 0;
    std::optional<std::wstring> result = std::nullopt;

    while (enumerator) {
        hr = enumerator->Next(WBEM_INFINITE, 1, &classObject, &returnCount);
        if (returnCount == 0) break;

        VARIANT vtProp;
        hr = classObject->Get(propertyName.c_str(), 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) {
            result = std::wstring(vtProp.bstrVal, SysStringLen(vtProp.bstrVal));
        }

        VariantClear(&vtProp);
        classObject->Release();
        break; // Only want the first result
    }

    enumerator->Release();
    return result;
}

bool WmiWrapper::executeMethod(const std::wstring &className, const std::wstring &methodName, const std::wstring &parameterName, const std::wstring &parameterValue) {
    if (!pImpl->services) return false;

    IWbemClassObject* classObject = nullptr;
    HRESULT hr = pImpl->services->GetObject(
        _bstr_t(className.c_str()),
        0,
        nullptr,
        &classObject,
        nullptr
    );
    if (FAILED(hr) || classObject == nullptr) {
        return false;
    }

    IWbemClassObject* inParamsClass = nullptr;
    hr = classObject->GetMethod(_bstr_t(methodName.c_str()), 0, &inParamsClass, nullptr);
    if (FAILED(hr)) {
        classObject->Release();
        return false;
    }

    IWbemClassObject* inParamsInstance = nullptr;
    if (inParamsClass != nullptr) {
        hr = inParamsClass->SpawnInstance(0, &inParamsInstance);
        if (FAILED(hr)) {
            inParamsClass->Release();
            classObject->Release();
            return false;
        }

        if (!parameterName.empty()) {
            VARIANT variantValue;
            VariantInit(&variantValue);
            variantValue.vt = VT_BSTR;
            variantValue.bstrVal = SysAllocString(parameterValue.c_str());
            hr = inParamsInstance->Put(
                _bstr_t(parameterName.c_str()),
                0,
                &variantValue,
                0
            );
            VariantClear(&variantValue);
            if (FAILED(hr)) {
                inParamsInstance->Release();
                inParamsClass->Release();
                classObject->Release();
                return false;
            }
        }
    }

    IWbemClassObject* outParams = nullptr;
    hr = pImpl->services->ExecMethod(
        _bstr_t(className.c_str()),
        _bstr_t(methodName.c_str()),
        0,
        nullptr,
        inParamsInstance,
        &outParams,
        nullptr
    );

    if (outParams != nullptr) {
        outParams->Release();
    }
    if (inParamsInstance != nullptr) {
        inParamsInstance->Release();
    }
    if (inParamsClass != nullptr) {
        inParamsClass->Release();
    }
    classObject->Release();

    return SUCCEEDED(hr);
}

}  // namespace pulseboost
