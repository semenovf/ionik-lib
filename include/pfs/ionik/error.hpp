////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022-2025 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2022.08.21 Initial version
//      2025.03.11 Refactored.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/error.hpp"

namespace ionik {

class error: public pfs::error
{
public:
    using pfs::error::error;
};

} // namespace ionik
