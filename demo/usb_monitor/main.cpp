////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2022.08.21 Initial version
////////////////////////////////////////////////////////////////////////////////
#include "pfs/ionik/device_observer.hpp"
#include "pfs/fmt.hpp"
#include "pfs/log.hpp"
#include <cstdlib>
#include <cstring>

static constexpr char const * TAG = "ionik-lib";

#if IONIK__DEVICE_OBSERVER_DISABLED
int main (int /*argc*/, char * /*argv*/[])
{
    fmt::println(stderr,
        "ATTENTION!\n"
        "Device observer feature disabled.\n"
        "See warning(s) while build library.");

    return EXIT_SUCCESS;
}

#else // !IONIK__DEVICE_OBSERVER_DISABLED
int main (int argc, char * argv[])
{
    ionik::device_observer::on_failure = [] (std::string const & msg) {
        LOGE(TAG, "{}", msg);
    };

    if (argc < 2) {
        fmt::print(stderr, "Too few arguments\n");
        fmt::print(stdout, "Usage: {} --subs | device_subsystem\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (std::strcmp("--subs", argv[1]) == 0) {
        auto subs = ionik::device_observer::working_device_subsystems();

        for (std::size_t i = 0, n = subs.size(); i < n; i++)
            fmt::print("{:3}. {}\n", i, subs[i]);

        return EXIT_SUCCESS;
    }

    ionik::device_observer d {argv[1]};

    d.arrived = [] (ionik::device_info const & di) {
        fmt::print("Device arrived: subsystem={}; devpath={}; sysname={}\n"
            , di.subsystem, di.devpath, di.sysname);
    };

    d.removed = [] (ionik::device_info const & di) {
        fmt::print("Device removed: subsystem={}; devpath={}; sysname={}\n"
            , di.subsystem, di.devpath, di.sysname);
    };

    while (true) {
        d.poll(std::chrono::milliseconds{100});
    }

    return EXIT_SUCCESS;
}
#endif // IONIK__DEVICE_OBSERVER_DISABLED
