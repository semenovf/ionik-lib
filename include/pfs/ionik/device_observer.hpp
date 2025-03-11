////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2022.08.21 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "error.hpp"
#include "exports.hpp"
#include <chrono>
#include <functional>
#include <system_error>
#include <utility>
#include <vector>

namespace ionik {

struct device_observer_rep;

struct device_info
{
    // On Linux:
    // `block`, `hid`, `usb`, ...
    // On Windows:
    // System, Display, USB, HIDClass...
    std::string subsystem; 

    std::string devpath;   
    
    // Empty on Windows
    std::string sysname;
};

// On Windows only one instance allowed.
class device_observer
{
    device_observer_rep * _rep {nullptr};

public:
    static IONIK__EXPORT std::function<void(std::string const &)> on_failure;

    std::function<void(device_info const &)> arrived 
        = [] (device_info const &) {};

    std::function<void(device_info const &)> removed
        = [] (device_info const &) {};

    // Unused in Windows
    std::function<void(device_info const &)> bound
        = [] (device_info const &) {};

    // Unused in Windows
    std::function<void(device_info const & )> unbound
        = [] (device_info const &) {};

private:
    bool init (std::initializer_list<std::string> && subsystems, error & err);
    void deinit ();

private:
    device_observer () = delete;
    device_observer (device_observer const &) = delete;
    device_observer (device_observer &&) = delete;
    device_observer & operator = (device_observer const &) = delete;
    device_observer & operator = (device_observer &&) = delete;

public:
    /**
     * Construct device observer for subsystem.
     *
     * @throws ionik::error (errc::bad_resource)
     */
    IONIK__EXPORT device_observer (std::initializer_list<std::string> subsystems);

    /**
     * Construct device observer for subsystem.
     */
    IONIK__EXPORT device_observer (error & err, std::initializer_list<std::string> subsystems);

    IONIK__EXPORT ~device_observer ();

    // On Windows timeout value ignored
    IONIK__EXPORT void poll (std::chrono::milliseconds timeout);

public: // static
    static IONIK__EXPORT std::vector<std::string> working_device_subsystems ();
};

} // namespace ionik
