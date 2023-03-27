////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2023 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2021.10.20 Initial version.
//      2021.11.01 Complete basic version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "error.hpp"
#include "file_provider.hpp"
#include "pfs/i18n.hpp"
#include <string>

namespace ionik {

template <typename FileProvider>
class file
{
public:
    using filepath_type = typename FileProvider::filepath_type;
    using filesize_type = typename FileProvider::filesize_type;
    using handle_type   = typename FileProvider::handle_type;

private:
    handle_type _h {FileProvider::invalid()};

private:
    file (handle_type h): _h(h) {}

public:
    file () {}

    file (file const & f) = delete;
    file & operator = (file const & f) = delete;

    file (file && f)
    {
        _h = f._h;
        f._h = FileProvider::invalid();
    }

    file & operator = (file && f)
    {
        if (this != & f) {
            close();
            _h = f._h;
            f._h = FileProvider::invalid();
        }

        return *this;
    }

    ~file ()
    {
        close();
    }

    operator bool () const noexcept
    {
        return _h >= 0;
    }

    void close () noexcept
    {
        if (!FileProvider::is_invalid(_h))
            FileProvider::close(_h);

        _h = FileProvider::invalid();
    }

    filesize_type offset () const
    {
        return FileProvider::offset(_h);
    }

    /**
     * Read data chunk from file.
     *
     * @param ifh File descriptor.
     * @param buffer Pointer to input buffer.
     * @param len Input buffer size.
     * @param offset Offset from start to read (-1 for start read from current position).
     * @param perr Pointer to store error if not @c null. If @a perr is null
     *        function may throw an error
     *
     * @return Actually read chunk size or -1 on error.
     *
     * @note Limit maximum file size to 2,147,479,552 bytes. For transfer bigger
     *       files use another way/tools (scp, for example).
     */
    filesize_type read (char * buffer, filesize_type len, error * perr = nullptr)
    {
        if (len == 0)
            return 0;

        if (len < 0) {
            error err {
                  make_error_code(std::errc::invalid_argument)
                , tr::_("invalid buffer length")
            };

            if (perr) {
                *perr = err;
                return -1;
            } else {
                throw err;
            }
        }

        return FileProvider::read(_h, buffer, len, perr);
    }

    template <typename T>
    inline filesize_type read (T & value, error * perr = nullptr)
    {
        return read(reinterpret_cast<char *>(& value), sizeof(T), perr);
    }

    /**
     * Read all content from file started from specified @a offset.
     */
    std::string read_all (error * perr = nullptr)
    {
        std::string result;
        char buffer[512];

        auto n = FileProvider::read(_h, buffer, 512, perr);

        while (n > 0) {
            result.append(buffer, n);

            n = FileProvider::read(_h, buffer, 512, perr);
        }

        return n < 0 ? std::string{} : result;
    }

    /**
     * @brief Write buffer to file.
     */
    filesize_type write (char const * buffer, filesize_type len, error * perr = nullptr)
    {
        return FileProvider::write(_h, buffer, len, perr);
    }

    /**
     * @brief Write value to file.
     */
    template <typename T>
    inline filesize_type write (T const & value, error * perr = nullptr)
    {
        return write(reinterpret_cast<char const *>(& value), sizeof(T), perr);
    }

    /**
     * Set file position by @a offset.
     */
    void set_pos (filesize_type offset, error * perr = nullptr)
    {
        FileProvider::set_pos(_h, offset, perr);
    }

public: // static
   /**
    * @brief Open file for reading.
    *
    * @return Input file handle or @c INVALID_FILE_HANDLE on error. In last case
    *         @a ec set to appropriate error code:
    *         - @c std::errc::no_such_file_or_directory if file not found;
    *         - @c std::errc::invalid_argument if @a offset has unsuitable value;
    *         - other POSIX-specific error codes returned by @c ::open and
    *           @c ::lseek calls.
    */
    static file open_read_only (filepath_type const & path, error * perr = nullptr)
    {
        return file{FileProvider::open_read_only(path, perr)};
    }

    /**
    * @brief Open file for writing.
    *
    * @return Output file handle or @c INVALID_FILE_HANDLE on error. In last case
    *         @a ec set to appropriate error code:
    *         - POSIX-specific error codes returned by @c ::open and
    *           @c ::lseek calls.
    */
    static file open_write_only (filepath_type const & path, truncate_enum trunc
        , error * perr = nullptr)
    {
        return file{FileProvider::open_write_only(path, trunc, perr)};
    }


    static file open_write_only (filepath_type const & path, error * perr = nullptr)
    {
        return open_write_only(path, truncate_enum::off, perr);
    }

    /**
     * Rewrite file with content from @a text.
     */
    static bool rewrite (filepath_type const & path, char const * buffer
        , filesize_type count, error * perr = nullptr)
    {
        file f = open_write_only(path, truncate_enum::on, perr);

        if (f) {
            auto n = f.write(buffer, count, perr);
            return n >= 0;
        }

        return false;
    }

    static void rewrite (filepath_type const & path, std::string const & text)
    {
        rewrite(path, text.c_str(), static_cast<filesize_type>(text.size()));
    }

    static std::string read_all (filepath_type const & path, error * perr = nullptr)
    {
        auto f = file::open_read_only(path, perr);

        if (f)
            return f.read_all(perr);

        return std::string{};
    }
};

} // namespace ionik
