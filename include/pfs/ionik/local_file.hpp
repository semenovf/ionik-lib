////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022-2023 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.03.27 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "file.hpp"
#include "file_provider.hpp"
#include "pfs/filesystem.hpp"

namespace ionik {

using local_file_handle = int;
using local_file_provider = file_provider<local_file_handle, pfs::filesystem::path>;
using local_file = file<local_file_provider>;

} // namespace ionik

