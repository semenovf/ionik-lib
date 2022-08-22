////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2022.08.21 Initial version
////////////////////////////////////////////////////////////////////////////////
#include "device_observer.hpp"
#include <windows.h>

namespace ionik {

struct device_observer_rep
{
    HWND hwnd;
    HDEVNOTIFY dev_notif;
    GUID guid;
};

} // namespace ionik

