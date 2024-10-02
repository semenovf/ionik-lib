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

struct counter_item 
{
    std::wstring name;
    PDH_HCOUNTER * hcounter;
    int alt_index; // Alternative item index
    bool processed;    // Set when counter is processed
};

#define COUNTER_ITEM(name, hcounter, altindex) \
    counter_item { name, reinterpret_cast<PDH_HCOUNTER *>(& this->hcounter), altindex, false }

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
        std::array<counter_item, 2> counters {
            // Can also may use "\\Processor Information(*)\\% Processor Time" 
            // and get individual CPU values with PdhGetFormattedCounterArray()
            
/*0*/         COUNTER_ITEM(L"\\Processor Information(_Total)\\% Processor Utility", _processor_time, 2)
/*1*/       , COUNTER_ITEM(L"\\Processor Information(_Total)\\% Processor Time", _processor_time, -1)

            //, COUNTER_ITEM(L"\\Memory\\Available Bytes", _mem_available)
        };

        for (auto & item : counters) {
            if (item.processed)
                continue;

            item.processed = true;

            PDH_STATUS rc1 = PdhValidatePathW(item.name.c_str());

            if (rc1 != ERROR_SUCCESS) {
                if (rc1 == PDH_CSTATUS_NO_COUNTER) {
                    if (item.alt_index >= 0)
                        continue;
                }

                pfs::throw_or(perr, error {
                      errc::unsupported
                    , tr::f_("perfomance counter is not valid: {}", pfs::windows::utf8_encode(item.name.c_str()))
                });
            }

            //
            // Mark all alternatives as processed
            //
            auto alt_index = item.alt_index;

            while (alt_index >= 0) {
                counters[alt_index].processed = true;
                PFS__TERMINATE(counters[alt_index].alt_index == -1 || alt_index < counters[alt_index].alt_index
                    , "inconsistent counters array");
                alt_index = counters[alt_index].alt_index;
            }

            rc = PdhAddEnglishCounterW(_hquery, item.name.c_str(), user_data, item.hcounter);

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
