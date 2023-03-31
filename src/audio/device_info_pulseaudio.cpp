////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file was part of `multimedia-lib` until 2023.03.31.
// This file is part of `ionik-lib`.
//
// Changelog:
//      2021.08.03 Initial version (multimedia-lib).
//      2023.03.31 Initial version (ionik-lib).
////////////////////////////////////////////////////////////////////////////////
#include "pfs/ionik/audio/device.hpp"
#include <pulse/pulseaudio.h>
#include <vector>

namespace ionik {
namespace audio {

class context_notifier final
{
    pa_context_state_t _state {PA_CONTEXT_UNCONNECTED};

public:
    bool connecting () const noexcept
    {
        bool result {false};

        switch (_state) {
            case PA_CONTEXT_UNCONNECTED:  // The context hasn't been connected yet.
            case PA_CONTEXT_CONNECTING:   // A connection is being established.
            case PA_CONTEXT_AUTHORIZING:  // The client is authorizing itself to the daemon.
            case PA_CONTEXT_SETTING_NAME: // The client is passing its application name to the daemon.
                result = true;
                break;
            default:
                break;
        }

        return result;
    }

    // The connection is established, the context is ready to execute operations.
    bool ready () const noexcept
    {
        return _state == PA_CONTEXT_READY;
    }

    // The connection failed or was disconnected.
    bool disconnected () const noexcept
    {
        return _state == PA_CONTEXT_FAILED;
    }

    // The connection was terminated cleanly.
    bool terminated () const noexcept
    {
        return _state == PA_CONTEXT_TERMINATED;
    }

    static void callback (pa_context * ctx, void * userdata)
    {
        auto notifier = reinterpret_cast<context_notifier *>(userdata);
        notifier->_state = pa_context_get_state(ctx);
    }
};

class server_info final
{
    std::string _user_name;
    std::string _host_name;
    std::string _server_version;
    std::string _server_name;
    std::string _default_sink_name;
    std::string _default_source_name;

public:
    static void callback (pa_context * c, pa_server_info const * i, void * userdata)
    {
        auto info = reinterpret_cast<server_info *>(userdata);

        info->_user_name = i->user_name;
        info->_host_name = i->host_name;
        info->_server_version = i->server_version;
        info->_server_name = i->server_name;
        info->_default_sink_name = i->default_sink_name;
        info->_default_source_name = i->default_source_name;
    }

    // User name of the daemon process
    std::string const & user_name () const noexcept
    {
        return _user_name;
    }

    // Host name the daemon is running on
    std::string const & host_name () const noexcept
    {
        return _host_name;
    }

    // Version string of the daemon
    std::string const & server_version () const noexcept
    {
        return _server_version;
    }

    // Server package name (usually "pulseaudio")
    std::string const & server_name () const noexcept
    {
        return _server_name;
    }

    // Name of default sink
    std::string const & default_sink_name () const noexcept
    {
        return _default_sink_name;
    }

    // Name of default source
    std::string const & default_source_name () const noexcept
    {
        return _default_source_name;
    }
};

class device_info_helper final
{
public:
    std::vector<device_info> * _device_infos_ptr {nullptr};
    device_info * _device_info_ptr {nullptr};

public:
    device_info_helper (std::vector<device_info> & device_infos)
        : _device_infos_ptr(& device_infos)
    {}

    device_info_helper (device_info & di)
        : _device_info_ptr(& di)
    {}

    template <typename NativeInfo>
    static void callback (pa_context * c
        , NativeInfo const * i
        , int eol
        , void * userdata)
    {
        // We're at the end of the list.
        if (eol > 0)
            return;

        auto info = reinterpret_cast<device_info_helper *>(userdata);

        if (info->_device_infos_ptr) {
            info->_device_infos_ptr->emplace_back();
            auto & di = info->_device_infos_ptr->back();
            di.name = i->name;
            di.readable_name = i->description;
        }

        if (info->_device_info_ptr) {
            info->_device_info_ptr->name = i->name;
            info->_device_info_ptr->readable_name = i->description;
        }
    }
};

class session final
{
    pa_mainloop *     _mainloop {nullptr};
    pa_mainloop_api * _mainloop_api {nullptr};
    pa_context *      _context {nullptr};
    context_notifier  _context_notifier;

public:
    ~session ()
    {
        end();
    }

    pa_context * context ()
    {
        return _context;
    }

    bool begin ()
    {
        // Create a mainloop API and connection to the default server
        _mainloop = pa_mainloop_new();
        _mainloop_api = pa_mainloop_get_api(_mainloop);

        pa_proplist * proplist {nullptr};

        _context = pa_context_new_with_proplist(_mainloop_api
            , "pfs::multimedia"
            , proplist);

        auto rc = pa_context_connect(_context
            , nullptr             // Connect to default server
            , PA_CONTEXT_NOFLAGS
            , nullptr);

        // Connection error
        if (rc < 0)
            return false;

        pa_context_set_state_callback(_context
            , context_notifier::callback
            , & _context_notifier);

        return true;
    }

    template <typename Operation>
    bool process_operation (Operation && operation)
    {
        bool error {false};
        bool done {false};
        int step = 0;
        pa_operation * op {nullptr};

        while (!done) {
            if (_context_notifier.connecting()) {
                pa_mainloop_iterate(_mainloop, 1, nullptr);
                continue;
            }

            if (_context_notifier.disconnected()
                    || _context_notifier.terminated()) {
                done = true;
                continue;
            }

            if (_context_notifier.ready()) {
                switch (step) {
                    case 0: {
                        op = operation();

                        if (!op) {
                            done = true;
                            error = true;
                            continue;
                        }

                        step++;
                        break;
                    }

                    case 1: {
                        auto state = pa_operation_get_state(op);

                        switch (state) {
                            case PA_OPERATION_RUNNING: // The operation is still running
                            default:
                                break;

                            case PA_OPERATION_DONE:   // The operation has completed
                                pa_operation_unref(op);
                                done = true;
                                continue;

                            case PA_OPERATION_CANCELLED: // The operation has been cancelled.
                                // Operations may get cancelled by the application,
                                // or as a result of the context getting disconnected
                                // while the operation is pending
                                done = true;
                                error = true;
                                continue;
                        }
                    }
                }
            }

            // Iterate the main loop and go again.  The second argument is whether
            // or not the iteration should block until something is ready to be
            // done.  Set it to zero for non-blocking.
            auto rc = pa_mainloop_iterate(_mainloop, 1, nullptr);

            // Main loop iteration error
            if (rc < 0) {
                done = true;
                error = true;
                continue;
            }
        }

        return error != true;
    }

    void end ()
    {
        if (_context) {
            pa_context_disconnect(_context);
            pa_context_unref(_context);
            _context = nullptr;
        }

        if (_mainloop) {
            pa_mainloop_free(_mainloop);
            _mainloop = nullptr;
            _mainloop_api = nullptr;
        }
    }
};

// Default source/input device
device_info default_input_device ()
{
    session sess;
    device_info result;

    if (sess.begin()) {
        server_info srv_info;

        auto rc = sess.process_operation([& sess, & srv_info] () -> pa_operation * {
            return pa_context_get_server_info(sess.context()
                , server_info::callback
                , & srv_info);
        });

        if (rc == true) {
            device_info_helper src_info {result};

            rc = sess.process_operation([& sess, & srv_info, & src_info] () -> pa_operation * {
                return pa_context_get_source_info_by_name(sess.context()
                    , srv_info.default_source_name().c_str()
                    , device_info_helper::callback<pa_source_info>
                    , & src_info);
            });
        }

        sess.end();
    }

    return result;
}

// Default sink/ouput device
device_info default_output_device ()
{
    session sess;
    device_info result;

    if (sess.begin()) {
        server_info srv_info;

        auto rc = sess.process_operation([& sess, & srv_info] () -> pa_operation * {
            return pa_context_get_server_info(sess.context()
                , server_info::callback
                , & srv_info);
        });

        if (rc == true) {
            device_info_helper sink_info {result};

            rc = sess.process_operation([& sess, & srv_info, & sink_info] () -> pa_operation * {
                return pa_context_get_sink_info_by_name(sess.context()
                    , srv_info.default_sink_name().c_str()
                    , device_info_helper::callback<pa_sink_info>
                    , & sink_info);
            });
        }

        sess.end();
    }

    return result;
}

std::vector<device_info> fetch_devices (device_mode mode)
{
    std::vector<device_info> result;
    session sess;

    if (sess.begin()) {
        if (mode == device_mode::input) {
            device_info_helper src_info {result};

            auto rc = sess.process_operation([& sess, & src_info] () -> pa_operation * {
                return pa_context_get_source_info_list(sess.context()
                    , device_info_helper::callback<pa_source_info>
                    , & src_info);
            });
        }

        if (mode == device_mode::output) {
            device_info_helper sink_info {result};

            auto rc = sess.process_operation([& sess, & sink_info] () -> pa_operation * {
                return pa_context_get_sink_info_list(sess.context()
                    , device_info_helper::callback<pa_sink_info>
                    , & sink_info);
            });
        }

        sess.end();
    }

    return result;
}

}} // namespace ionik::audio
