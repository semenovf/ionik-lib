////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.10.19 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/sys_class_net_provider.hpp"
#include "proc_reader.hpp"
#include <pfs/filesystem.hpp>
#include <pfs/i18n.hpp>
#include <pfs/integer.hpp>
#include <algorithm>
#include <cstring>

namespace fs = pfs::filesystem;

namespace ionik {
namespace metrics {

using string_view = sys_class_net_provider::string_view;

sys_class_net_provider::sys_class_net_provider (std::string iface, std::string readable_name, error * perr)
    : _iface(std::move(iface))
    , _readable_name(std::move(readable_name))
{
    if (_readable_name.empty())
        _readable_name = _iface;

    fs::path const net_dir = PFS__LITERAL_PATH("/sys/class/net");
    fs::path const root_dir = net_dir / pfs::utf8_decode_path(_iface) / PFS__LITERAL_PATH("statistics");
    _rx_bytes_path = root_dir / PFS__LITERAL_PATH("rx_bytes");
    _tx_bytes_path = root_dir / PFS__LITERAL_PATH("tx_bytes");

    for (auto const & p: {net_dir, root_dir, _rx_bytes_path, _tx_bytes_path}) {
        std::error_code ec;

        if (fs::exists(p, ec))
            continue;

        if (ec) {
            pfs::throw_or(perr, ec, tr::f_("check path failure: {}", p));
            return;
        }

        pfs::throw_or(perr, std::make_error_code(std::errc::no_such_file_or_directory)
            , pfs::utf8_encode_path(p));
        return;
    }

    if (!read(_recent_data.rx_bytes, _recent_data.tx_bytes, perr))
        return;

    _recent_checkpoint = time_point_type::clock::now();
}

static std::int64_t read_integer (fs::path const & path, error * perr)
{
    error err;
    proc_reader reader {path, & err};

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return -1;
    }

    auto text = reader.move_content();

    char const * first = text.c_str();
    char const * last = first + text.size();

    while (first != last && *(last - 1) == '\n')
        --last;

    if (first == last) {
        pfs::throw_or(perr, tr::f_("invalid content in: {}", path));
        return -1;
    }

    std::error_code ec;
    auto n = pfs::to_integer<std::int64_t>(first, last, ec);

    if (ec) {
        pfs::throw_or(perr, tr::f_("invalid content in: {}", path));
        return -1;
    }

    return n;
}

bool sys_class_net_provider::read (std::int64_t & rx_bytes, std::int64_t & tx_bytes, error * perr)
{
    rx_bytes = read_integer(_rx_bytes_path, perr);

    if (rx_bytes < 0)
        return false;

    tx_bytes = read_integer(_tx_bytes_path, perr);

    if (tx_bytes < 0)
        return false;

    return true;
}

bool sys_class_net_provider::read_all (error * perr)
{
    std::int64_t rx_bytes = 0;
    std::int64_t tx_bytes = 0;

    if (!read(rx_bytes, tx_bytes, perr))
        return false;

    auto now = time_point_type::clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now - _recent_checkpoint).count();

    if (millis <= 0)
        return false;

    auto rx_speed = static_cast<double>(rx_bytes - _recent_data.rx_bytes) * 1000 / millis;
    auto tx_speed = static_cast<double>(tx_bytes - _recent_data.tx_bytes) * 1000 / millis;

    _recent_data.rx_bytes = rx_bytes;
    _recent_data.tx_bytes = tx_bytes;
    _recent_data.rx_speed = rx_speed;
    _recent_data.tx_speed = tx_speed;
    _recent_data.rx_speed_max = std::max(_recent_data.rx_speed_max, rx_speed);
    _recent_data.tx_speed_max = std::max(_recent_data.tx_speed_max, tx_speed);
    _recent_checkpoint = now;

    return true;
}

bool sys_class_net_provider::query (counter_group & counters, error * perr)
{
    if (!read_all(perr))
        return false;

    std::memcpy(& counters, & _recent_data, sizeof(counters));
    return true;
}

bool sys_class_net_provider::query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
    , void * user_data_ptr, error * perr)
{
    if (f == nullptr)
        return true;

    if (!read_all(perr))
        return false;

    (void)(!f("rx_bytes", counter_t{_recent_data.rx_bytes}, user_data_ptr)
        && !f("tx_bytes", counter_t{_recent_data.tx_bytes}, user_data_ptr)
        && !f("rx_speed", counter_t{_recent_data.rx_speed}, user_data_ptr)
        && !f("tx_speed", counter_t{_recent_data.tx_speed}, user_data_ptr)
        && !f("rx_speed_max", counter_t{_recent_data.rx_speed_max}, user_data_ptr)
        && !f("tx_speed_max", counter_t{_recent_data.tx_speed_max}, user_data_ptr));

    return true;
}

std::vector<std::string> sys_class_net_provider::interfaces (error * perr)
{
    std::vector<std::string> result;
    fs::path const dir {"/sys/class/net"};

    std::error_code ec;

    if (!fs::exists(dir, ec)) {
        if (ec) {
            pfs::throw_or(perr, ec, tr::f_("check path failure: {}", dir));
        } else {
            pfs::throw_or(perr, std::make_error_code(std::errc::no_such_file_or_directory)
                , pfs::utf8_encode_path(dir));
        }

        return std::vector<std::string>{};
    }

    if (!fs::is_directory(dir)) {
        if (ec) {
            pfs::throw_or(perr, ec, tr::f_("check directory failure: {}", dir));
        } else {
            pfs::throw_or(perr, std::make_error_code(std::errc::not_a_directory), pfs::utf8_encode_path(dir));
        }

        return std::vector<std::string>{};
    }

    // Entries can be symbolic links
    for (auto const & dir_entry : fs::directory_iterator{dir})
        result.push_back(pfs::utf8_encode_path(dir_entry.path().filename()));

    return result;
}

}} // namespace ionik::metrics
