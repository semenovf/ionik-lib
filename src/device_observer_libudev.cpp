////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2022.08.21 Initial version
////////////////////////////////////////////////////////////////////////////////
#include "device_observer.hpp"
#include "error.hpp"
#include <pfs/i18n.hpp>
#include <map>
#include <fcntl.h>
#include <libudev.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>

namespace ionik {

static constexpr int MAX_EVENTS = 32;

struct device_observer_rep
{
    udev * u;
    udev_monitor * m;
    int md;           // file descriptor used by this monitor
    int ed;           // epoll descriptor
};

std::function<void(std::string const &)> device_observer::on_failure
    = [] (std::string const &) {};

bool device_observer::init (std::initializer_list<std::string> && subsystems, error & err)
{
    _rep = new device_observer_rep;
    _rep->md = -1;
    _rep->ed = -1;
    _rep->u = udev_new();

    if (!_rep->u) {
        err = error {tr::_("create udev context failure")};
        return false;
    }

    _rep->m = udev_monitor_new_from_netlink(_rep->u, "udev");

    if (!_rep->m) {
        err = error{tr::_("create udev monitor object failure")};
        return false;
    }

    for (auto const & devtype: subsystems) {
        int errn = udev_monitor_filter_add_match_subsystem_devtype(_rep->m
            , devtype.c_str(), nullptr);

        if (errn < 0) {
            err = error {tr::_("modify monitor filter failure")};
            return false;
        }
    }

    // Start monitoring
    int errn = udev_monitor_enable_receiving(_rep->m);

    if (errn < 0) {
        err = error {tr::_("start monitoring failure")};
        return false;
    }

    _rep->md = udev_monitor_get_fd(_rep->m);

    if (_rep->md < 0) {
        err = error {tr::_("monitor file descriptor failure")};
        return false;
    }

    errn = fcntl(_rep->md, F_SETFD, fcntl(_rep->md, F_GETFD, 0) | O_NONBLOCK);

    if (errn < 0) {
        err = error {tr::_("set nonblocking to observer file descriptor failure")};
        return false;
    }

    // In the initial epoll_create() implementation, the size argument informed
    // the kernel of the number of file descriptors that the caller expected
    // to add to the epoll instance.  The kernel used this information  as a
    // hint for the amount of space to initially allocate in internal data
    // structures describing events.  (If necessary, the kernel would allocate
    // more space if the caller's usage exceeded the hint given in size.)
    // Nowadays, this hint is no longer required (the kernel dynamically sizes
    // the required data structures without needing the hint), but size must
    // still be greater than zero, in order to ensure backward compatibility
    // when new epoll applications are run on older kernels.
    _rep->ed = epoll_create(1);

    if (_rep->ed < 0) {
        err = error {tr::f_("epoll_create failure: {}", pfs::system_error_text())};
        return false;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.fd = _rep->md;

    errn = epoll_ctl(_rep->ed, EPOLL_CTL_ADD, _rep->md, & ev);

    if (errn < 0) {
        err = error {tr::f_("epoll_ctl failure: {}", pfs::system_error_text())};
        return false;
    }

    return true;
}

void device_observer::deinit ()
{
    PFS__ASSERT(_rep, "");
    PFS__ASSERT(_rep->u, "");
    PFS__ASSERT(_rep->m, "");

    if (_rep->ed > 0) {
        ::close(_rep->ed);
        _rep->ed = -1;
    }

    udev_monitor_filter_remove(_rep->m);
    udev_monitor_unref(_rep->m);
    _rep->m = nullptr;
    _rep->md = -1;

    udev_unref(_rep->u);
    _rep->u = nullptr;

    delete _rep;
}

device_observer::device_observer (error & err, std::initializer_list<std::string> subsystems)
{
    init(std::move(subsystems), err);
}

device_observer::device_observer (std::initializer_list<std::string> subsystems)
{
    error err;
    auto success = init(std::move(subsystems), err);

    if (!success)
        throw err;
}

device_observer::~device_observer ()
{
    deinit();
}

void device_observer::poll (std::chrono::milliseconds timeout)
{
    struct epoll_event events[MAX_EVENTS];
    int nfds = epoll_wait(_rep->ed, events, MAX_EVENTS, timeout.count());

    for (int i = 0; i < nfds; i++) {
        udev_device * dev = udev_monitor_receive_device(_rep->m);

        if (dev) {
            auto subsystem = udev_device_get_subsystem(dev);
            auto devpath   = udev_device_get_devpath(dev);
            auto sysname   = udev_device_get_sysname(dev);

            auto action = udev_device_get_action(dev);

            if (std::strcmp(action, "add") == 0) {
                if (arrived)
                    arrived(device_info{subsystem, devpath, sysname});
            } else if (std::strcmp(action, "remove") == 0) {
                if (removed)
                    removed(device_info{subsystem, devpath, sysname});
            } else if (std::strcmp(action, "bind") == 0) {
                if (bound)
                    bound(device_info{subsystem, devpath, sysname});
            } else if (std::strcmp(action, "unbind") == 0) {
                if (unbound)
                    unbound(device_info{subsystem, devpath, sysname});
            }

            udev_device_unref(dev);
        }
    }
}

std::vector<std::string> device_observer::working_device_subsystems ()
{
    auto u = udev_new();

    if (!u)
        return std::vector<std::string>{};

    std::vector<std::string> result;

    auto enu = udev_enumerate_new(u);
    int errn = udev_enumerate_scan_devices(enu);
    //int errn = udev_enumerate_scan_subsystems(enu);

    if (errn >= 0) {
        std::map<std::string, bool> subs;

        for (auto entry = udev_enumerate_get_list_entry(enu)
                ; entry
                ; entry = udev_list_entry_get_next(entry)) {

            auto name = udev_list_entry_get_name(entry);
            auto device = udev_device_new_from_syspath(u, name);
            auto subsystem = udev_device_get_subsystem(device);

            subs.emplace(std::string{subsystem}, true);
        }

        for (auto const & item: subs)
            result.push_back(item.first);

        udev_enumerate_unref(enu);
    }

    udev_unref(u);
    return result;
}

} // namespace ionik
