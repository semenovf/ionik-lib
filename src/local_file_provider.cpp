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
#include "pfs/numeric_cast.hpp"
#include "pfs/ionik/error.hpp"
#include "pfs/ionik/file_provider.hpp"
#include <cassert>

#if _MSC_VER
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <fcntl.h>
#   include <io.h>

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

// NOTE `pfs::numeric_cast` can throw `std::overflow_error` or `std::underflow_error`

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
filesize_t file_provider_t::size (filepath_t const & path, error * perr)
{
    if (!fs::exists(path)) {
        pfs::throw_or(perr, make_error_code(std::errc::no_such_file_or_directory)
            , fs::utf8_encode(path));
        return 0;
    }

    return pfs::numeric_cast<filesize_t>(fs::file_size(path));
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
handle_t file_provider_t::open_read_only (filepath_t const & path, error * perr)
{
    if (!fs::exists(path)) {
        pfs::throw_or(perr
            , make_error_code(std::errc::no_such_file_or_directory)
            , fs::utf8_encode(path));
        return INVALID_FILE_HANDLE;
    }

    if (!fs::is_regular_file(path)) {
        if (fs::is_symlink(path)) {
            std::error_code ec;
            fs::path tmp {path};

            while (!ec && fs::is_symlink(tmp))
                tmp = fs::read_symlink(tmp, ec);

            if (ec) {
                pfs::throw_or(perr, ec, tr::f_("open read only failure: {}", fs::utf8_encode(path)));
                return INVALID_FILE_HANDLE;
            }
        }  else {
            pfs::throw_or(perr, make_error_code(std::errc::invalid_argument)
                , tr::f_("expected regular file: {}", fs::utf8_encode(path)));
            return INVALID_FILE_HANDLE;
        }
    }

#   if _MSC_VER
    handle_t h;
    // _sopen_s(& h, fs::utf8_encode(path).c_str(), O_RDONLY, _SH_DENYNO, 0);
    _wsopen_s(& h, path.c_str(), O_RDONLY | O_BINARY, _SH_DENYNO, 0);
#   else
    handle_t h = ::open(fs::utf8_encode(path).c_str(), O_RDONLY);
#   endif

    if (h < 0) {
        pfs::throw_or(perr, pfs::get_last_system_error(), tr::f_("open read only file: {}", path));
        return INVALID_FILE_HANDLE;
    }

    return h;
}

template <>
handle_t file_provider_t::open_write_only (filepath_t const & path, truncate_enum trunc
    , filesize_type initial_size, error * perr)
{
    int oflags = O_WRONLY | O_CREAT;

    if (trunc == truncate_enum::on && initial_size == 0)
        oflags |= O_TRUNC;

#if _MSC_VER
    oflags |= O_BINARY;

    handle_t h;
    // _sopen_s(& h, fs::utf8_encode(path).c_str(), oflags, _SH_DENYWR, S_IRUSR | S_IWUSR);
    _wsopen_s(& h, path.c_str(), oflags, _SH_DENYWR, S_IRUSR | S_IWUSR);
#else
    handle_t h = ::open(fs::utf8_encode(path).c_str(), oflags, S_IRUSR | S_IWUSR);
#endif

    if (h < 0) {
        pfs::throw_or(perr, pfs::get_last_system_error(), tr::f_("open write only file failure: {}", path));
        return INVALID_FILE_HANDLE;
    }

    if (trunc == truncate_enum::on && initial_size > 0) {
        std::error_code ec;

#if _MSC_VER
        LARGE_INTEGER large_initial_size;
        large_initial_size.QuadPart = static_cast<LONGLONG>(initial_size);

        if (large_initial_size.QuadPart >= 0) {
            auto hh = reinterpret_cast<HANDLE>(_get_osfhandle(h));
            auto success = SetFilePointerEx(hh, large_initial_size, NULL, FILE_BEGIN) == 0
                || SetEndOfFile(hh) == 0;

            if (!success)
                ec = pfs::get_last_system_error();
        } else {
            ec = std::make_error_code(std::errc::file_too_large);
        }
#else
        auto rc = ::truncate(fs::utf8_encode(path).c_str(), static_cast<off_t>(initial_size));

        if (rc != 0)
            ec = pfs::get_last_system_error();
#endif

        if (ec) {
            pfs::throw_or(perr, tr::f_("resize file failure while open write only file: {}"
                , fs::utf8_encode(path)));

            file_provider_t::close(h);
            return INVALID_FILE_HANDLE;
        }
    }

    return h;
}

template <>
std::pair<filesize_t, bool> file_provider_t::offset (handle_t const & h, error * perr)
{
#if _MSC_VER
    auto n = _lseek(h, 0, SEEK_CUR);
#else
    auto n = ::lseek(h, 0, SEEK_CUR);
#endif

    if (n < 0) {
        pfs::throw_or(perr, pfs::get_last_system_error(), tr::_("get file position"));
        return std::make_pair(0, false);
    }

    return std::make_pair(pfs::numeric_cast<filesize_t>(n), true);
}

template <>
bool file_provider_t::set_pos (handle_t & h, filesize_t pos, error * perr)
{
    // FIXME Need to handle the situation when pos > std::numeric_limits<off_t>::max()

    // NOTE lseek() allows the file offset to be set beyond the end of the file
    // (but this does not change the size of the file).
#if _MSC_VER
    auto offset_value = _lseek(h, pfs::numeric_cast<long>(pos), SEEK_SET);
#else
    auto offset_value = ::lseek(h, pfs::numeric_cast<off_t>(pos), SEEK_SET);
#endif

    if (offset_value < 0) {
        pfs::throw_or(perr, pfs::get_last_system_error(), tr::_("set file position"));
        return false;
    }

    return true;
}

template <>
std::pair<filesize_t, bool> file_provider_t::read (handle_t & h, char * buffer
    , filesize_t len, error * perr)
{
#if _MSC_VER
    auto n = _read(h, buffer, pfs::numeric_cast<unsigned int>(len));
#else
    auto n = ::read(h, buffer, pfs::numeric_cast<std::size_t>(len));
#endif

    if (n < 0) {
        pfs::throw_or(perr, pfs::get_last_system_error(), tr::_("read from file"));
        return std::make_pair(0, false);
    }

    return std::make_pair(pfs::numeric_cast<filesize_t>(n), true);
}

template <>
std::pair<filesize_t, bool> file_provider_t::write (handle_t & h, char const * buffer
    , filesize_t len, error * perr)
{
#if _MSC_VER
    auto n = _write(h, buffer, pfs::numeric_cast<unsigned int>(len));
#else
    auto n = ::write(h, buffer, pfs::numeric_cast<std::size_t>(len));
#endif

    if (n < 0) {
        pfs::throw_or(perr, pfs::get_last_system_error(), tr::_("write into file"));
        return std::make_pair(0, false);
    }

    return std::make_pair(pfs::numeric_cast<filesize_t>(n), true);
}

} // namespace ionik
