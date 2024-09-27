////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.12 Initial version (proc_provider.hpp).
//      2024.09.27 Initial version (moved from proc_provider.hpp).
////////////////////////////////////////////////////////////////////////////////
#include "proc_reader.hpp"
#include "ionik/local_file.hpp"

namespace ionik {
namespace metrics {

proc_reader::proc_reader (pfs::filesystem::path const & path, error * perr)
{
    auto proc_file = local_file::open_read_only(path, perr);

    if (!proc_file)
        return;

    _content = proc_file.read_all(perr);
}

}} // namespace ionik::metrics
