////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.23 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/pdh_provider.hpp"
#include <pfs/assert.hpp>
#include <pfs/i18n.hpp>
#include <pfs/windows.hpp>
#include <pdhmsg.h>
#include <array>
#include <string>
#include <utility>

namespace ionik {
namespace metrics {

#define COUNTER_ITEM(name, hcounter) \
    std::pair<std::wstring, PDH_HCOUNTER *> { name, reinterpret_cast<PDH_HCOUNTER *>(& this->hcounter) }

static std::wstring make_counter_path (LPWSTR object_name, LPWSTR instance_name, LPWSTR counter_name)
{
    wchar_t path_buffer [PDH_MAX_COUNTER_PATH];

    PDH_COUNTER_PATH_ELEMENTS_W path_elements {
          NULL // szMachineName
        , object_name
        , instance_name
        , NULL // szParentInstance
        , 0    // dwInstanceIndex
        , counter_name
    };

    DWORD buffer_size = _countof(path_buffer);

    auto rc = PdhMakeCounterPathW(& path_elements, path_buffer, & buffer_size, 0);

    PFS__TERMINATE(rc == ERROR_SUCCESS, "Make counter path failure");

    return std::wstring(path_buffer, buffer_size);
}

pdh_provider::pdh_provider (error * perr)
{
    std::string exception_cause;

    DWORD_PTR user_data = reinterpret_cast<DWORD_PTR>(this);

    // Open a query object.
    // Performance data is collected from a real-time data source.
    //                       v
    auto rc = PdhOpenQueryW(NULL, user_data, & _hquery);

    if (rc == ERROR_SUCCESS) {
        std::array<std::pair<std::wstring, PDH_HCOUNTER *>, 1> counters {
            // Can also may use "\\Processor Information(*)\\% Processor Time" 
            // and get individual CPU values with PdhGetFormattedCounterArray()
              COUNTER_ITEM(L"\\Processor Information(_Total)\\% Processor Time", _processor_time)
            //, COUNTER_ITEM(L"\\Memory\\Available Bytes", _mem_available)
        };

        for (auto const & item : counters) {
            auto rc1 = PdhValidatePathW(item.first.c_str());

            if (rc1 != ERROR_SUCCESS) {
                pfs::throw_or(perr, error {
                      errc::unsupported
                    , tr::f_("perfomance counter is not valid: {}", pfs::windows::utf8_encode(item.first.c_str()))
                });
            }

            rc = PdhAddEnglishCounterW(_hquery, item.first.c_str(), user_data, item.second);

            if (rc != ERROR_SUCCESS) {
                exception_cause = "PdhAddEnglishCounter";
                break;
            }
        }
    } else {
        exception_cause = "PdhOpenQueryW";
    }

    if (rc != ERROR_SUCCESS) {
        pfs::throw_or(perr, error {
              pfs::get_last_system_error()
            , exception_cause
        });

        return;
    }
}

pdh_provider::~pdh_provider ()
{
    if (_hquery != INVALID_HANDLE_VALUE) {
        PdhCloseQuery(_hquery);
        _hquery = INVALID_HANDLE_VALUE;
    }
}

inline double to_double (PDH_HCOUNTER hcounter)
{
    PDH_FMT_COUNTERVALUE result;
    PdhGetFormattedCounterValue(hcounter, PDH_FMT_DOUBLE, nullptr, & result);
    return result.doubleValue;
}

inline std::int64_t to_integer (PDH_HCOUNTER hcounter)
{
    PDH_FMT_COUNTERVALUE result;
    PdhGetFormattedCounterValue(hcounter, PDH_FMT_LARGE, nullptr, & result);
    return result.largeValue;
}

bool pdh_provider::query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
    , void * user_data_ptr, error * perr)
{
    auto rc = PdhCollectQueryData(_hquery);

    if (rc != ERROR_SUCCESS) {
        if (rc == PDH_NO_DATA)
            return true;

        pfs::throw_or(perr, error {
              pfs::get_last_system_error()
            , "PdhCollectQueryData"
        });

        return false;
    }

    if (f != nullptr) {
        (void)(!f("ProcessorTime", to_double(_processor_time), user_data_ptr)
            //&& !f("mem_avail", to_integer(_mem_available), user_data_ptr)
           // && !f("", , user_data_ptr)
        );
    }

    return true;
}

}} // namespace ionic::metrics
