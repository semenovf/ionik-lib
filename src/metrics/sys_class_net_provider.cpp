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

    std::error_code ec;

    fs::path const dir = PFS__LITERAL_PATH("/sys/class/net");

    if (!fs::exists(dir, ec)) {
        if (ec) {
            pfs::throw_or(perr, error {ec, tr::f_("check path failure: {}", dir)});
        } else {
            pfs::throw_or(perr, error {
                  std::make_error_code(std::errc::no_such_file_or_directory)
                , fs::utf8_encode(dir)
            });
        }

        return;
    }

    if (!fs::is_directory(dir)) {
        if (ec) {
            pfs::throw_or(perr, error {ec, tr::f_("check directory failure: {}", dir)});
        } else {
            pfs::throw_or(perr, error {
                  std::make_error_code(std::errc::not_a_directory)
                , fs::utf8_encode(dir)
            });
        }

        return;
    }

    _recent_checkpoint = time_point_type::clock::now();

    read_all(perr);
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
        pfs::throw_or(perr, error {
              pfs::errc::unexpected_error
            , tr::f_("invalid content in: {}", path)
        });

        return -1;
    }

    std::error_code ec;
    auto n = pfs::to_integer<std::int64_t>(first, last, ec);

    if (ec) {
        pfs::throw_or(perr, error {
              pfs::errc::unexpected_error
            , tr::f_("invalid content in: {}", path)
        });

        return -1;
    }

    return n;
}

bool sys_class_net_provider::read_all (error * perr)
{
    error err;

    fs::path const root_dir = PFS__LITERAL_PATH("/sys/class/net") / fs::utf8_decode(_iface)
        / PFS__LITERAL_PATH("statistics");

    auto rx_bytes = read_integer(root_dir / PFS__LITERAL_PATH("rx_bytes"), perr);

    if (rx_bytes < 0)
        return false;

    auto tx_bytes = read_integer(root_dir / PFS__LITERAL_PATH("tx_bytes"), perr);

    if (tx_bytes < 0)
        return false;

    auto now = time_point_type::clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now - _recent_checkpoint).count();

    auto rx_speed = (_recent_data.rx_speed + static_cast<double>(rx_bytes - _recent_data.rx_bytes) * millis / 1000) / 2;
    auto tx_speed = (_recent_data.tx_speed + static_cast<double>(tx_bytes - _recent_data.tx_bytes) * millis / 1000) / 2;

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
            pfs::throw_or(perr, error {ec, tr::f_("check path failure: {}", dir)});
        } else {
            pfs::throw_or(perr, error {
                  std::make_error_code(std::errc::no_such_file_or_directory)
                , fs::utf8_encode(dir)
            });
        }

        return std::vector<std::string>{};
    }

    if (!fs::is_directory(dir)) {
        if (ec) {
            pfs::throw_or(perr, error {ec, tr::f_("check directory failure: {}", dir)});
        } else {
            pfs::throw_or(perr, error {
                  std::make_error_code(std::errc::not_a_directory)
                , fs::utf8_encode(dir)
            });
        }

        return std::vector<std::string>{};
    }

    // Entries can be symbolic links
    for (auto const & dir_entry : fs::directory_iterator{dir})
        result.push_back(fs::utf8_encode(dir_entry.path().filename()));

    return result;
}


}} // namespace ionik::metrics

