////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.12.16 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include <pfs/i18n.hpp>
#include <windows.h>
#include <memory>
#include <type_traits>

struct sc_handle_deleter
{
    void operator () (std::remove_pointer_t<SC_HANDLE> * sch) const
    {
        CloseServiceHandle(sch);
    }
};

using unique_sc_handle = std::unique_ptr<std::remove_pointer_t<SC_HANDLE>, sc_handle_deleter>;

inline void print_error (std::string const & msg)
{
    fmt::println(stderr, tr::f_("ERROR: {}", msg));
}

inline unique_sc_handle
make_manager (LPCWSTR machine_name = nullptr // default is local computer
    , LPCWSTR database_name = nullptr // default is ServicesActive database 
    , DWORD desired_access = SC_MANAGER_ALL_ACCESS)
{
    auto sc_manager = OpenSCManagerW(machine_name, database_name, desired_access);
    return unique_sc_handle(sc_manager, sc_handle_deleter{});
}

inline unique_sc_handle
make_service (SC_HANDLE sc_manager, LPCWSTR service_name, DWORD desired_access)
{
    auto sc_service = OpenServiceW(sc_manager, service_name, desired_access);
    return unique_sc_handle(sc_service, sc_handle_deleter{});
}
