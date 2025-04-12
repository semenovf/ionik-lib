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
#include "pfs/ionik/exports.hpp"
#include <cstdint>
#include <utility>

namespace ionik {

enum class truncate_enum: std::int8_t { off, on };
using filesize_t = std::uint64_t;

template <typename HandleType, typename FilePath>
class file_provider
{
public:
    using filepath_type = FilePath;
    using filesize_type = filesize_t;
    using handle_type   = HandleType;
    using offset_result_type = std::pair<filesize_type, bool>;
    using read_result_type = std::pair<filesize_type, bool>;
    using write_result_type = std::pair<filesize_type, bool>;

public:
    static IONIK__EXPORT handle_type invalid () noexcept;
    static IONIK__EXPORT bool is_invalid (handle_type const & h) noexcept;
    static IONIK__EXPORT filesize_type size (filepath_type const & path, error * perr);
    static IONIK__EXPORT handle_type open_read_only (filepath_type const & path, error * perr);
    static IONIK__EXPORT handle_type open_write_only (filepath_type const & path, truncate_enum trunc
        , filesize_type initial_size, error * perr);
    static IONIK__EXPORT void close (handle_type & h);
    static IONIK__EXPORT offset_result_type offset (handle_type const & h, error * perr);
    static IONIK__EXPORT bool set_pos (handle_type & h, filesize_type offset, error * perr);

    /**
     * Read data from file into buffer
     *
     * @return { > 0,  true } on read successful;
     *         {   0,  true } if no data available (end of file);
     **        {   0, false } on read failure, @e *perr set to @c ionik::error if specified and not @c null.
     */
    static IONIK__EXPORT read_result_type read (handle_type & h, char * buffer, filesize_type len, error * perr);
    static IONIK__EXPORT write_result_type write (handle_type & h, char const * buffer, filesize_type len, error * perr);
};

} // namespace ionik
