////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.07.01 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "filesystem_monitor/monitor.hpp"

#if IONIK__HAS_INOTIFY
#   include "filesystem_monitor/backend/inotify.hpp"
#elif _MSC_VER
#   include "filesystem_monitor/backend/win32.hpp"
#else
#   error "Filesystem monitor not implemented for this platform"
#endif

namespace ionik {
namespace filesystem_monitor {

#if IONIK__HAS_INOTIFY
    using monitor_rep = backend::inotify;
#elif _MSC_VER
    using monitor_rep = backend::win32;
#endif

using monitor_t = monitor<monitor_rep>;

}} // namespace ionik::filesystem_monitor
