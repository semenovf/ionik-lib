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
using filesize_t = std::int64_t;

template <typename HandleType, typename FilePath>
class file_provider
{
public:
    using filepath_type = FilePath;
    using filesize_type = filesize_t;
    using handle_type   = HandleType;

public:
    static handle_type invalid () noexcept;
    static bool is_invalid (handle_type const & h) noexcept;
    static handle_type open_read_only (filepath_type const & path, error * perr);
    static handle_type open_write_only (filepath_type const & path, truncate_enum trunc, error * perr);
    static void close (handle_type & h);
    static filesize_type offset (handle_type const & h);
    static void set_pos (handle_type & h, filesize_type offset, error * perr);
    static filesize_type read (handle_type & h, char * buffer, filesize_type len, error * perr);
    static filesize_type write (handle_type & h, char const * buffer, filesize_type len, error * perr);
};

} // namespace ionik
