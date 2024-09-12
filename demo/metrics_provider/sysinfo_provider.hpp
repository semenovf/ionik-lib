////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "ionik/error.hpp"

namespace ionik {
namespace metrics {

class sysinfo_provider
{
public:
    sysinfo_provider ();

public:
    bool query (error * perr = nullptr);
};

}} // namespace ionik::metrics
