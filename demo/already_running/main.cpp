////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.12.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/ionik/already_running.hpp"
#include "pfs/log.hpp"
#include <string>
#include <thread>

static std::string MAGIC_NAME {"e577d357-3076-41aa-8b10-9ead450ece15"};
static char const * TAG = "already_running";

int main (int /*argc*/, char * /*argv*/[])
{
    ionik::already_running already_running{MAGIC_NAME};

    if (already_running()) {
        LOGD(TAG, "Application already running");
        return EXIT_SUCCESS;
    } else {
        LOGD(TAG, "First application instance");
    }

    while (true)
        std::this_thread::sleep_for(std::chrono::seconds{1});

    return EXIT_SUCCESS;
}
