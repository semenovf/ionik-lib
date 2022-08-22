////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2022.08.21 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <chrono>
#include <functional>
#include <system_error>
#include <utility>
#include <vector>

namespace ionik {

struct device_observer_rep;

struct device_info
{
    std::string subsystem; // `block`, ...
    std::string devtype;   // `partition`, `disk`, ...
    std::string sysname;   // `sdc`, `sdc1` for `block` subsystem
};

class device_observer
{
    device_observer_rep * _rep {nullptr};

public:
    std::function<void(device_info const & )> arrived;
    std::function<void(device_info const & )> removed;
    std::function<void(device_info const & )> bound;
    std::function<void(device_info const & )> unbound;

private:
    std::pair<std::error_code, std::string> init (
        std::initializer_list<std::string> && subsystems);
    void deinit ();

public:
    /**
     * Construct device observer for subsystem.
     *
     * @throws ionik::error (errc::bad_resource)
     */
    device_observer (std::initializer_list<std::string> subsystems);

    /**
     * Construct device observer for subsystem.
     */
    device_observer (std::error_code & ec, std::initializer_list<std::string> subsystems);

    ~device_observer ();

    void poll (std::chrono::milliseconds timeout);

public: // static
    static std::vector<std::string> working_device_subsystems ();
};

} // namespace ionik
