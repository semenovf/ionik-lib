////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.03.06 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/fmt.hpp"
#include "pfs/ionik/video/capture_device.hpp"

int main ()
{
    auto capture_devices = ionik::video::fetch_capture_devices();

    for (auto const & dev: capture_devices) {
        fmt::println("name: {}", dev.readable_name);

        for (auto const & item : dev.data)
            fmt::println("{}: {}", item.first, item.second);
    }

    return EXIT_SUCCESS;
}
