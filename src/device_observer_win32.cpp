////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2022.08.21 Initial version
////////////////////////////////////////////////////////////////////////////////
#include "pfs/ionik/device_observer.hpp"
#include "pfs/ionik/error.hpp"
#include "pfs/i18n.hpp"
#include "pfs/string_view.hpp"
#include "pfs/windows.hpp"
#include <initguid.h>
#include <devpkey.h>
#include <setupapi.h>
#include <dbt.h>
#include <algorithm>
#include <atomic>

//
// Registering for device notification
// https://learn.microsoft.com/en-us/windows/win32/devio/registering-for-device-notification
//
namespace ionik {

std::function<void(std::string const &)> device_observer::on_failure
    = [] (std::string const &) {};

struct device_observer_rep
{
    HWND hwnd;
    std::vector<HDEVNOTIFY> dev_notifs;
};

struct device_class_info {
    std::string device_enumero;
    std::string device_class;
    GUID device_class_guid;
};

struct subsystem {
    std::string name;
    GUID guid;
};

// https://learn.microsoft.com/en-us/windows-hardware/drivers/install/guid-devinterface-usb-device
// https://learn.microsoft.com/en-us/windows-hardware/drivers/install/guid-devinterface-hid
// ...
static std::vector<subsystem> __subsystems = {
      { "usb"      , GUID{0xA5DCBF10, 0x6530, 0x11D2, {0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED}}}
    , { "hid"      , GUID{0x4d1e55b2, 0xf16f, 0x11cf, {0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30}}}
    , { "disk"     , GUID{0x53F56307, 0xB6BF, 0x11D0, {0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B}}}
    , { "partition", GUID{0x53F5630A, 0xB6BF, 0x11D0, {0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B}}}
    , { "volume"   , GUID{0x53F5630D, 0xB6BF, 0x11D0, {0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B}}}
    , { "wpd"      , GUID{0x6AC27878, 0xA6FA, 0x4155, {0xBA, 0x85, 0xF9, 0x8F, 0x49, 0x1D, 0x4F, 0x33}}}
};

static std::vector<device_class_info> __device_classes;

static std::string to_string (GUID const & guid)
{
    return fmt::format("{:08X}-{:04X}-{:04X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}"
        , guid.Data1, guid.Data2, guid.Data3
        , guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3]
        , guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

static std::wstring generate_window_class_name ()
{
    static std::atomic_int __wnd_counter = 0;
    auto name = fmt::format("ionik_window_{}"
        , __wnd_counter.fetch_add(1, std::memory_order_relaxed));
    return pfs::windows::utf8_decode(name.c_str());
}

LRESULT notify_window_proc (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT ret = 0L;

    switch (message) {
        case WM_DEVICECHANGE: {
            auto ptr = reinterpret_cast<device_observer *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

            if (!ptr) {
                throw error(errc::system_error), tr::_("Unable to fetch device observer pointer");
            }

            PDEV_BROADCAST_HDR hdr = reinterpret_cast<PDEV_BROADCAST_HDR>(lparam);

            if (hdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                PDEV_BROADCAST_DEVICEINTERFACE pdev = reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(hdr);
                auto devpath = pfs::windows::utf8_encode(pdev->dbcc_name);

                auto subs_pos = std::find_if(__subsystems.cbegin(), __subsystems.cend()
                    , [pdev] (subsystem const & subs) {
                        return IsEqualGUID(subs.guid, pdev->dbcc_classguid) != FALSE;
                    });

                std::string subsystem = "?";

                if (subs_pos != __subsystems.cend())
                    subsystem = subs_pos->name;
                else
                    subsystem = to_string(pdev->dbcc_classguid);

                switch (wparam) {
                    case DBT_DEVICEARRIVAL:
                        ptr->arrived(device_info{std::move(subsystem), devpath, ""});
                        break;

                    case DBT_DEVICEREMOVECOMPLETE:
                        ptr->removed(device_info{std::move(subsystem), devpath, ""});
                        break;
                }
            }
            break;
        }

        default:
            return DefWindowProc(hwnd, message, wparam, lparam);
    }

    return ret;
}

static void collect_device_classes ()
{
    // Retrieve a list of all present devices
    auto dev_list_handle = SetupDiGetClassDevs(nullptr, nullptr, nullptr
        , DIGCF_PRESENT | DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);

    if (dev_list_handle == INVALID_HANDLE_VALUE) {
        device_observer::on_failure(fmt::format("{} ({})"
            , pfs::windows::utf8_error(GetLastError()), GetLastError()));
        return;
    }

    SP_DEVINFO_DATA dev_info_data;

    ZeroMemory(& dev_info_data, sizeof(SP_DEVINFO_DATA));
    dev_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD device_index = 0
            ; SetupDiEnumDeviceInfo(dev_list_handle, device_index, & dev_info_data)
            ; device_index++) {
        DEVPROPTYPE dev_prop_type;
        WCHAR buffer[512];
        DWORD required_size;

        device_class_info dci {"", "", GUID{0, 0, 0, {0,0,0,0,0,0,0,0}}};

        auto success = SetupDiGetDeviceProperty(dev_list_handle, & dev_info_data
            , & DEVPKEY_Device_ClassGuid, & dev_prop_type , reinterpret_cast<PBYTE>(& dci.device_class_guid)
            , sizeof(dci.device_class_guid), & required_size, 0);

        success = success && SetupDiGetDeviceProperty(dev_list_handle, & dev_info_data
            , & DEVPKEY_Device_EnumeratorName, & dev_prop_type , reinterpret_cast<PBYTE>(& buffer)
            , sizeof(buffer), & required_size, 0);

        dci.device_enumero = pfs::windows::utf8_encode(buffer);

        success = success && SetupDiGetDeviceProperty(dev_list_handle, & dev_info_data
            , & DEVPKEY_Device_Class, & dev_prop_type , reinterpret_cast<PBYTE>(& buffer)
            , sizeof(buffer), & required_size, 0);

        dci.device_class = pfs::windows::utf8_encode(buffer);

        success = SetupDiGetDeviceProperty(dev_list_handle, & dev_info_data
            , & DEVPKEY_Device_DeviceDesc, & dev_prop_type , reinterpret_cast<PBYTE>(& buffer)
            , sizeof(buffer), & required_size, 0);
        
        //success = SetupDiGetDeviceProperty(dev_list_handle, & dev_info_data
        //    , & DEVPKEY_Device_FriendlyName, & dev_prop_type , reinterpret_cast<PBYTE>(& buffer)
        //    , sizeof(buffer), & required_size, 0);

        //LOGD("", "desc: {}, enum={}, class={}, class_guid={}"
        //    , pfs::windows::utf8_encode(buffer)
        //    , dci.device_enumero
        //    , dci.device_class
        //    , to_string(dci.device_class_guid));

        if (!success) { // || PropType != DEVPROP_TYPE_GUID) {
            auto errn = GetLastError();

            if (errn == ERROR_NOT_FOUND) {
                // This device has an unknown device setup class.
            } else {
                device_observer::on_failure(fmt::format("{} ({})"
                    , pfs::windows::utf8_error(errn), errn));
            }

            continue;
        }   

        if (required_size > sizeof(buffer)) {
            device_observer::on_failure(fmt::format("Buffer size to store the property value too small: required {} bytes"
                , required_size));
            continue;
        }

        auto const * device_class_guid_ptr = & dci.device_class_guid;

        auto pos = std::find_if(__device_classes.cbegin(), __device_classes.cend()
            , [device_class_guid_ptr] (device_class_info const & dci) {
                return dci.device_class_guid == *device_class_guid_ptr;
            });

        if (pos == __device_classes.cend())
            __device_classes.push_back(std::move(dci));
    }

    if (dev_list_handle)
        SetupDiDestroyDeviceInfoList(dev_list_handle);

}

device_observer::device_observer (std::initializer_list<std::string> subsystems)
{
    auto res = init(std::move(subsystems));

    if (res.first)
        throw pfs::error{res.first, res.second};
}

device_observer::device_observer (std::error_code & ec, std::initializer_list<std::string> subsystems)
{
    auto res = init(std::move(subsystems));

    if (res.first)
        ec = res.first;
}

device_observer::~device_observer ()
{
    deinit();
}

std::pair<std::error_code, std::string> device_observer::init (
    std::initializer_list<std::string> && subsystems)
{
    _rep = new device_observer_rep {
          HWND{0}
        , std::vector<HDEVNOTIFY>{}
    };

    // Create notify window

    auto window_class_name = generate_window_class_name();
    WNDCLASSEX wx;
    ZeroMemory(& wx, sizeof(wx));

    wx.cbSize = sizeof(WNDCLASSEX);
    wx.style = CS_HREDRAW | CS_VREDRAW;
    wx.lpfnWndProc = reinterpret_cast<WNDPROC>(notify_window_proc);
    wx.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wx.lpszClassName = window_class_name.c_str();
    wx.hInstance = reinterpret_cast<HINSTANCE>(GetModuleHandle(nullptr));

    if (RegisterClassEx(& wx)) {
        _rep->hwnd = CreateWindowW(window_class_name.c_str()
            , window_class_name.c_str()
            , WS_ICONIC, 0, 0, CW_USEDEFAULT, 0, HWND_MESSAGE
            , nullptr, wx.hInstance, nullptr);

        if (!_rep->hwnd) {
            throw error {errc::system_error, pfs::system_error_text()};
        }
    }

    SetWindowLongPtrW(_rep->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    for (auto const & devtype: subsystems) {
        DEV_BROADCAST_DEVICEINTERFACE notification_filter;
        ZeroMemory(& notification_filter, sizeof(notification_filter));
        notification_filter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
        notification_filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

        auto subs_pos = std::find_if(__subsystems.cbegin(), __subsystems.cend()
            , [& devtype] (subsystem const & subs) {
                return subs.name == devtype;
        });

        if (subs_pos != __subsystems.cend()) {
            notification_filter.dbcc_classguid = subs_pos->guid;
        } else {
            device_observer::on_failure(fmt::format("Unsupported subsystem: {}, ignored", devtype));
            continue;
        }

        auto dev_notif = RegisterDeviceNotification(_rep->hwnd
            , & notification_filter
            , DEVICE_NOTIFY_WINDOW_HANDLE);

        _rep->dev_notifs.push_back(dev_notif);
    }

    return std::make_pair(std::error_code{}, std::string{});
}

void device_observer::deinit ()
{
    for (auto dev_notif: _rep->dev_notifs) {
        UnregisterDeviceNotification(dev_notif);
    }

    // Destroy window
    if (_rep->hwnd) {
        DestroyWindow(_rep->hwnd);
        _rep->hwnd = nullptr;
    }

    delete _rep;
}

//
// https://stackoverflow.com/questions/10866311/getmessage-with-a-timeout
//
static BOOL GetMessageWithTimeout (MSG * msg, HWND hwnd, std::chrono::milliseconds timeout)
{
    BOOL res;
    UINT_PTR timerId = SetTimer(hwnd, NULL, static_cast<UINT>(timeout.count()), NULL);
    res = GetMessage(msg, hwnd, 0, 0);
    KillTimer(NULL, timerId);

    if (!res)
        return FALSE;

    if (msg->message == WM_TIMER && msg->hwnd == hwnd && msg->wParam == timerId)
        return FALSE; //TIMEOUT! You could call SetLastError() or something...

    return TRUE;
}

void device_observer::poll (std::chrono::milliseconds timeout)
{
    MSG msg;

    // GetMessage is blocking call
    // auto success = GetMessage(& msg, _rep->hwnd, 0, 0);
    //
    // PeekMessage returns immediately
    // auto success = PeekMessage(& msg, _rep->hwnd, 0, 0, PM_REMOVE);

    auto success = GetMessageWithTimeout(& msg, _rep->hwnd, timeout);

    if (success != 0) {
        TranslateMessage(& msg);
        DispatchMessage(& msg);
    }
}

std::vector<std::string> device_observer::working_device_subsystems ()
{
    std::vector<std::string> result;

    for (auto const & subs: __subsystems)
        result.emplace_back(subs.name);

    return result;
}

// Unused yet
static std::vector<std::string> working_device_classes ()
{
    if (__device_classes.empty())
        collect_device_classes();

    std::vector<std::string> result;

    for (auto const & dci: __device_classes)
        result.emplace_back(dci.device_class);

    return result;
}

} // namespace ionik

