////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.12 Initial version (proc_provider.hpp).
//      2024.09.27 Initial version (moved from proc_provider.hpp).
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "counter.hpp"
#include <pfs/ionik/error.hpp>
#include <pfs/string_view.hpp>
#include <string>
#include <vector>

namespace ionik {
namespace metrics {

class proc_self_status_provider
{
public:
    using string_view = pfs::string_view;

    struct record_view
    {
        string_view key;
        std::vector<string_view> values;
    };

private:
    std::string _content;

private:
    bool read_all (error * perr);
    bool parse_record (string_view::const_iterator & pos, string_view::const_iterator last
        , record_view & rec, error * perr = nullptr);

public:
    proc_self_status_provider ();

public:
    /**
     * Сonsistently visits records with the function call @a f with a signature
     * bool (string_view const & key, std::vector<string_view> const & values).
     * If @a f return @c true the loop breaks.
     */
    template <typename F>
    bool query_all (F && f, error * perr = nullptr)
    {
        if (!read_all(perr))
            return false;

        string_view content_view {_content};
        auto pos = content_view.cbegin();
        auto last = content_view.cend();

        record_view rec;

        while (parse_record(pos, last, rec)) {
            if (f(rec.key, rec.values))
                break;
        }

        return true;
    }

    /**
     * Supported keys:
     *      * VmSize - the virtual memory usage of the entire process, in bytes (it is the sum of VmLib, VmExe, VmData, and VmStk);
     *      * VmPeak - the peak virtual memory size, in bytes;
     *      * VmRSS  - resident set size, in bytes;
     *      * VmSwap - the swap memory used, in bytes.
     */
    bool query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , void * user_data_ptr, error * perr = nullptr);
};

}} // namespace ionik::metrics
