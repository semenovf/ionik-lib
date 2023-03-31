////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2023 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.03.27 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/i18n.hpp"
#include "pfs/filesystem.hpp"
#include "pfs/ionik/error.hpp"
#include "pfs/ionik/file_provider.hpp"
#include <cassert>

#if _MSC_VER
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <fcntl.h>

#   ifndef S_IRUSR
#       define S_IRUSR _S_IREAD
#   endif

#   ifndef S_IWUSR
#       define S_IWUSR _S_IWRITE
#   endif
#else // _MSC_VER
#   include <sys/types.h>
#   include <fcntl.h>
#   include <unistd.h>
#endif // !_MSC_VER

namespace ionik {

static constexpr int INVALID_FILE_HANDLE = -1;
namespace fs = pfs::filesystem;
using file_provider_t = file_provider<int, pfs::filesystem::path>;
using filepath_t = file_provider_t::filepath_type;
using filesize_t = file_provider_t::filesize_type;
using handle_t = file_provider_t::handle_type;

template <>
handle_t file_provider_t::invalid () noexcept
{
    return -1;
}

template <>
bool file_provider_t::is_invalid (handle_type const & h) noexcept
{
    return h < 0;
}

template <>
handle_t file_provider_t::open_read_only (filepath_t const & path, error * perr)
{
    if (!fs::exists(path)) {
        error err {
              make_error_code(std::errc::no_such_file_or_directory)
            , fs::utf8_encode(path)
        };

        if (*perr) {
            *perr = err;
            return INVALID_FILE_HANDLE;
        } else {
            throw err;
        }
    }

#   if _MSC_VER
    handle_t h;
    _sopen_s(& h, fs::utf8_encode(path).c_str(), O_RDONLY, _SH_DENYNO, 0);
#   else
    handle_t h = ::open(fs::utf8_encode(path).c_str(), O_RDONLY);
#   endif

    if (h < 0) {
        error err {
              std::error_code(errno, std::generic_category())
            , tr::f_("open read only file: {}", path)
        };

        if (perr) {
            *perr = err;
            return INVALID_FILE_HANDLE;
        } else {
            throw err;
        }
    }

    return h;
}

template <>
handle_t file_provider_t::open_write_only (filepath_t const & path, truncate_enum trunc, error * perr)
{
    int oflags = O_WRONLY | O_CREAT;

    if (trunc == truncate_enum::on)
        oflags |= O_TRUNC;

#   if _MSC_VER
    handle_t h;
    _sopen_s(& h, fs::utf8_encode(path).c_str(), oflags, _SH_DENYWR, S_IRUSR | S_IWUSR);
#   else
    handle_t h = ::open(fs::utf8_encode(path).c_str(), oflags, S_IRUSR | S_IWUSR);
#   endif

    if (h < 0) {
        error err {
              std::error_code(errno, std::generic_category())
            , tr::f_("open write only file: {}", path)
        };

        if (perr) {
            *perr = err;
            return INVALID_FILE_HANDLE;
        } else {
            throw err;
        }
    }

    return h;
}

template <>
void file_provider_t::close (handle_t & h)
{
#if _MSC_VER
    _close(h);
#else
    ::close(h);
#endif
}

template <>
filesize_t file_provider_t::offset (handle_t const & h)
{
#if _MSC_VER
    return static_cast<filesize_t>(_lseek(h, 0, SEEK_CUR));
#else
    return static_cast<filesize_t>(::lseek(h, 0, SEEK_CUR));
#endif
}

template <>
void file_provider_t::set_pos (handle_t & h, filesize_t offset, error * perr)
{
#if _MSC_VER
    auto pos = _lseek(h, static_cast<long>(offset), SEEK_SET);
#else
    auto pos = ::lseek(h, offset, SEEK_SET);
#endif

    if (pos < 0) {
        error err {
              std::error_code(errno, std::generic_category())
            , tr::_("set file position")
        };

        if (perr) {
            *perr = err;
            return;
        } else {
            throw err;
        }
    }
}

template <>
filesize_t file_provider_t::read (handle_t & h, char * buffer, filesize_t len, error * perr)
{
    assert(len >= 0);

#if _MSC_VER
    auto n = _read(h, buffer, static_cast<unsigned int>(len));
#else
    auto n = ::read(h, buffer, len);
#endif

    if (n < 0) {
        error err {
              std::error_code(errno, std::generic_category())
            , tr::_("read from file")
        };

        if (perr) {
            *perr = err;
            return -1;
        } else {
            throw err;
        }
    }

    return n;
}

template <>
filesize_t file_provider_t::write (handle_t & h, char const * buffer, filesize_t len, error * perr)
{
    assert(len >= 0);

#if _MSC_VER
    auto n = _write(h, buffer, static_cast<unsigned int>(len));
#else
    auto n = ::write(h, buffer, len);
#endif

    if (n < 0) {
        error err {
              std::error_code(errno, std::generic_category())
            , tr::_("write into file")
        };

        if (perr) {
            *perr = err;
            return -1;
        } else {
            throw err;
        }
    }

    return n;
}

} // namespace ionik
