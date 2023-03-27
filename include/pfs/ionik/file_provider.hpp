////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2023 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.03.27 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/ionik/error.hpp"

namespace ionik {

enum class truncate_enum: std::int8_t { off, on };

template <typename FilePath>
class file_provider
{
public:
    using filepath_type = FilePath;
    using filesize_type = std::int64_t;
    using handle_type   = int;

public:
    static constexpr handle_type const INVALID_FILE_HANDLE = -1;

public:
    static handle_type open_read_only (filepath_type const & path, error * perr);
    static handle_type open_write_only (filepath_type const & path, truncate_enum trunc, error * perr);
    static void close (handle_type h);
    static filesize_type offset (handle_type h);
    static bool set_pos (handle_type h, filesize_type offset, error * perr);
    static filesize_type read (handle_type h, char * buffer, filesize_type len, error * perr);
    static filesize_type write (handle_type h, char const * buffer, filesize_type len, error * perr);
};

} // namespace ionik
