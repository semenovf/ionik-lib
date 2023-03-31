////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file was part of `multimedia-lib` until 2023.03.31.
// This file is part of `ionik-lib`.
//
// Changelog:
//      2021.08.07 Initial version (multimedia-lib).
//      2023.03.31 Initial version (ionik-lib).
////////////////////////////////////////////////////////////////////////////////
#include "pfs/ionik/audio/device.hpp"
#include <mmdeviceapi.h>
#include <strmif.h> // ICreateDevEnum
#include <functiondiscoverykeys_devpkey.h> // PROPERTYKEY
#include <vector>
#include <cwchar>

namespace ionik {
namespace audio {

// Avoid warning:
// warning C4996: 'wcsrtombs': This function or variable may be unsafe.
//                 Consider using wcsrtombs_s instead. To disable
//                 deprecation, use _CRT_SECURE_NO_WARNINGS. See online
//                 help for details.
static std::string convert_wide (LPCWSTR wstr)
{
    std::mbstate_t state = std::mbstate_t();
    std::size_t len = 1 + std::wcsrtombs(nullptr, & wstr, 0, & state);

    std::string result(len, '\x0');
    std::wcsrtombs(& result[0], & wstr, len, & state);
    return result;
}

class IMMDeviceEnumerator_initializer
{
    HRESULT hrCoInit {S_FALSE};
    IMMDeviceEnumerator ** ppEnumerator {nullptr};

private:
    void finalize ()
    {
        if (ppEnumerator && *ppEnumerator) {
            (*ppEnumerator)->Release();
            *ppEnumerator = nullptr;
            ppEnumerator = nullptr;
        }

        if (SUCCEEDED(hrCoInit)) {
            CoUninitialize();
            hrCoInit = S_FALSE;
        }
    }

public:
    IMMDeviceEnumerator_initializer (IMMDeviceEnumerator ** pp)
        : ppEnumerator(pp)
    {
        if (pp) {
            bool success {false};

            // Mandatory call to call CoCreateInstance() later
            hrCoInit = CoInitialize(nullptr);

            if (SUCCEEDED(hrCoInit)) {
                // https://docs.microsoft.com/en-gb/windows/win32/coreaudio/mmdevice-api
                const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
                const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);

                auto hr = CoCreateInstance(CLSID_MMDeviceEnumerator
                    , nullptr
                    , CLSCTX_ALL // CLSCTX_INPROC_SERVER
                    , IID_IMMDeviceEnumerator
                    , reinterpret_cast<void **>(ppEnumerator));

                if (SUCCEEDED(hr)) {
                    success  = true;
                } else {
                    // Possible errors:
                    // REGDB_E_CLASSNOTREG - A specified class is not registered in the
                    //     registration database. Also can indicate that the type of server
                    //     you requested in the CLSCTX enumeration is not registered or the
                    //     values for the server types in the registry are corrupt.
                    // E_NOINTERFACE - The specified class does not implement the requested
                    //     interface, or the controlling IUnknown does not expose the
                    //     requested interface.

                    // There might be error handling here
                    ;
                }
            }

            if (!success)
                finalize();
        }
    }

    ~IMMDeviceEnumerator_initializer ()
    {
        finalize();
    }
};

class IMMDeviceCollection_initializer
{
    IMMDeviceCollection ** ppEndpoints {nullptr};

private:
    void finalize ()
    {
        if (ppEndpoints && *ppEndpoints) {
            (*ppEndpoints)->Release();
            *ppEndpoints = nullptr;
            ppEndpoints = nullptr;
        }
    }

public:
    IMMDeviceCollection_initializer (IMMDeviceCollection ** pp
        , IMMDeviceEnumerator * pEnumerator
        , device_mode mode)
        : ppEndpoints(pp)
    {
        if (pEnumerator && pp) {
            bool success {false};

            EDataFlow data_flow = (mode == device_mode::output)
                ? eRender
                : eCapture;

            // https://docs.microsoft.com/en-us/windows/win32/api/mmdeviceapi/nf-mmdeviceapi-immdeviceenumerator-enumaudioendpoints
            auto hr = pEnumerator->EnumAudioEndpoints(data_flow
                , DEVICE_STATE_ACTIVE
                , ppEndpoints);

            if (SUCCEEDED(hr)) {
                success = true;
            } else {
                // There might be error handling here
                // NOTE! But there is no apparent reason for the error.
                ;
            }

            if (!success)
                finalize();
        }
    }

    ~IMMDeviceCollection_initializer ()
    {
        finalize();
    }
};

class IMMDevice_guard
{
    IMMDevice ** ppDevice {nullptr};

public:
    IMMDevice_guard (IMMDevice ** pp)
        : ppDevice(pp)
    {}

    ~IMMDevice_guard ()
    {
        if (ppDevice && *ppDevice) {
            (*ppDevice)->Release();
            *ppDevice = nullptr;
            ppDevice = nullptr;
        }
    }
};

static bool device_info_helper (IMMDevice * pEndpoint, device_info & di)
{
    bool result = false;

    if (pEndpoint) {
        IPropertyStore * pProps = nullptr;

        // Get the endpoint ID string.
        // https://docs.microsoft.com/en-us/windows/win32/api/mmdeviceapi/nf-mmdeviceapi-immdevice-getid
        LPWSTR pwszID = nullptr;
        auto hr = pEndpoint->GetId(& pwszID);

        if (SUCCEEDED(hr)) {
            hr = pEndpoint->OpenPropertyStore(STGM_READ, & pProps);

            if (SUCCEEDED(hr)) {
                PROPVARIANT varName;
                // Initialize container for property value.
                PropVariantInit(& varName);

                // Get the endpoint's friendly-name property.
                hr = pProps->GetValue(PKEY_Device_FriendlyName, & varName);

                if (SUCCEEDED(hr)) {
                    di.name = convert_wide(pwszID);
                    di.readable_name = convert_wide(varName.pwszVal);
                    PropVariantClear(& varName);

                    result = true;
                } else { // pProps->GetValue()
                    // There might be error handling here
                    ;
                }

                if (pProps) {
                    pProps->Release();
                    pProps = nullptr;
                }
            } else { // pEndpoint->OpenPropertyStore()
                // There might be error handling here
                // NOTE! But there is no apparent reason for the error.
                ;
            }
        } else { // pEndpoint->GetId()
            // There might be error handling here
            ;
        }

        if (pwszID) {
            CoTaskMemFree(pwszID);
            pwszID = nullptr;
        }
    } else {
        // There might be error handling here
        // E_INVALIDARG - Parameter 'device_index' is not a valid device number.
        ;
    }

    if (pEndpoint) {
        pEndpoint->Release();
        pEndpoint = nullptr;
    }

    return result;
}

// Default source/input device
static device_info default_device_helper (device_mode mode)
{
    device_info result;
    IMMDeviceEnumerator * pEnumerator = nullptr;
    IMMDeviceEnumerator_initializer enumerator_initializer {& pEnumerator};

    if (pEnumerator) {
        IMMDevice * pEndpoint = nullptr;

        EDataFlow data_flow = (mode == device_mode::output)
            ? eRender
            : eCapture;

        // https://docs.microsoft.com/en-us/windows/win32/api/mmdeviceapi/nf-mmdeviceapi-immdeviceenumerator-getdefaultaudioendpoint
        auto hr = pEnumerator->GetDefaultAudioEndpoint(data_flow, eConsole, & pEndpoint);
        IMMDevice_guard endpoint_guard {& pEndpoint};

        if (SUCCEEDED(hr) && pEndpoint) {
            if (!device_info_helper(pEndpoint, result)) {
                result = device_info{};
            }
        }
    }

    return result;
}

// Default source/input device
device_info default_input_device ()
{
    return default_device_helper(device_mode::input);
}

// Default sink/ouput device
device_info default_output_device ()
{
    return default_device_helper(device_mode::output);
}

// https://docs.microsoft.com/en-us/windows/win32/coreaudio/device-properties
std::vector<device_info> fetch_devices (device_mode mode)
{
    std::vector<device_info> result;
    IMMDeviceEnumerator * pEnumerator = nullptr;
    IMMDeviceEnumerator_initializer enumerator_initializer {& pEnumerator};

    if (pEnumerator) {
        IMMDeviceCollection * pEndpoints = nullptr;
        IMMDeviceCollection_initializer endpoints_initializer {& pEndpoints, pEnumerator, mode};

        if (pEndpoints) {
            // https://docs.microsoft.com/en-us/windows/win32/api/mmdeviceapi/nf-mmdeviceapi-immdevicecollection-getcount
            UINT count = 0;
            auto hr = pEndpoints->GetCount(& count);

            if (SUCCEEDED(hr)) {
                for (UINT device_index = 0; device_index < count; device_index++) {
                    IMMDevice * pEndpoint = nullptr;

                    // https://docs.microsoft.com/en-us/windows/win32/api/mmdeviceapi/nf-mmdeviceapi-immdevicecollection-item
                    hr = pEndpoints->Item(static_cast<UINT>(device_index), & pEndpoint);
                    IMMDevice_guard endpoint_guard {& pEndpoint};

                    if (SUCCEEDED(hr) && pEndpoint) {
                        device_info di;

                        if (device_info_helper(pEndpoint, di)) {
                            result.push_back(std::move(di));
                        }
                    } else {
                        // There might be error handling here
                        // E_INVALIDARG - Parameter 'device_index' is not a valid device number.
                        ;
                    }
                }
            } else { // pEndpoints->GetCount()
                // There might be error handling here
                // NOTE! But there is no apparent reason for the error.
                ;
            }
        }
    }

    return result;
}

}} // namespace ionik::audio
