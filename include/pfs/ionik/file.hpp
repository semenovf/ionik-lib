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
#include <type_traits>

namespace ionik {

template <typename FileProvider>
class file
{
public:
    using filepath_type = typename FileProvider::filepath_type;
    using filesize_type = typename FileProvider::filesize_type;
    using handle_type   = typename FileProvider::handle_type;
    using offset_result_type = typename FileProvider::offset_result_type;
    using read_result_type   = typename FileProvider::read_result_type;
    using write_result_type  = typename FileProvider::write_result_type;

    static_assert(std::is_unsigned<filesize_type>::value, "File size must be unsigned");

private:
    handle_type _h {FileProvider::invalid()};
    filesize_type _size {0};

private:
    file (handle_type h, filesize_type size): _h(h), _size(size) {}

public:
    file () {}

    file (file const & f) = delete;
    file & operator = (file const & f) = delete;

    file (file && f)
    {
        _h = f._h;
        _size = f._size;
        f._h = FileProvider::invalid();
        f._size = 0;
    }

    file & operator = (file && f)
    {
        if (this != & f) {
            close();
            _h = f._h;
            _size = f._size;
            f._h = FileProvider::invalid();
            f._size = 0;
        }

        return *this;
    }

    ~file ()
    {
        close();
    }

    operator bool () const noexcept
    {
        return _h != FileProvider::invalid();
    }

    handle_type native () const noexcept
    {
        return _h;
    }

    void close () noexcept
    {
        if (!FileProvider::is_invalid(_h))
            FileProvider::close(_h);

        _h = FileProvider::invalid();
    }

    offset_result_type offset (error * perr = nullptr) const
    {
        return FileProvider::offset(_h, perr);
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
     */
    read_result_type read (char * buffer, filesize_type len, error * perr = nullptr)
    {
        return FileProvider::read(_h, buffer, len, perr);
    }

    template <typename T>
    inline read_result_type read (T & value, error * perr = nullptr)
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

        while (n.first > 0) {
            result.append(buffer, n.first);

            n = FileProvider::read(_h, buffer, 512, perr);
        }

        return n.second ? result : std::string{};
    }

    /**
     * @brief Write buffer to file.
     */
    write_result_type write (char const * buffer, filesize_type len, error * perr = nullptr)
    {
        return FileProvider::write(_h, buffer, len, perr);
    }

    /**
     * @brief Write value to file.
     */
    template <typename T>
    inline write_result_type write (T const & value, error * perr = nullptr)
    {
        return write(reinterpret_cast<char const *>(& value), sizeof(T), perr);
    }

    /**
     * Set file position by @a offset.
     */
    bool set_pos (filesize_type pos, error * perr = nullptr)
    {
        if (pos >= _size) {
            pfs::throw_or(perr, make_error_code(std::errc::invalid_argument)
                , tr::f_("new file position is out of bounds"));
            return false;
        }

        return FileProvider::set_pos(_h, pos, perr);
    }

    /**
     * Read @a bytes from current file position.
     */
    bool skip (filesize_type bytes, error * perr = nullptr)
    {
        auto off_res = offset(perr);

        if (!off_res.second)
            return false;

        return set_pos(off_res.first + bytes, perr);
    }

public: // static
   /**
    * @brief Open file for reading.
    *
    * @return Input file handle.
    */
    static file open_read_only (filepath_type const & path, error * perr = nullptr)
    {
        return file {
              FileProvider::open_read_only(path, perr)
            , FileProvider::size(path, perr)
        };
    }

    /**
    * @brief Open file for writing.
    *
    * @param path File path to open.
    * @param trunc Truncation flag.
    * @param initial_size Initial size. If @a trunc is @c on then the file will be resized to the
    *        specified size.
    *
    * @return Output file handle.
    */
    static file open_write_only (filepath_type const & path, truncate_enum trunc
        , filesize_type initial_size = 0, error * perr = nullptr)
    {
        return file {
              FileProvider::open_write_only(path, trunc, initial_size, perr)
            , trunc == truncate_enum::on ? 0 : FileProvider::size(path, perr)
        };
    }

    static file open_write_only (filepath_type const & path, error * perr = nullptr)
    {
        return open_write_only(path, truncate_enum::off, 0, perr);
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
            return n.second;
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
