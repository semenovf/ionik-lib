////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.12.15 Initial version (https://learn.microsoft.com/en-us/windows/win32/services/svcconfig-cpp).
////////////////////////////////////////////////////////////////////////////////
#include "utils.hpp"
#include <pfs/argvapi.hpp>
#include <pfs/expected.hpp>
#include <pfs/scope.hpp>
#include <pfs/string_view.hpp>
#include <pfs/windows.hpp>
#include <pfs/ionik/error.hpp>
#include <cstdlib>

#pragma comment(lib, "advapi32.lib")

namespace fs = pfs::filesystem;
using result_type = pfs::expected<bool, std::string>;

result_type WINAPI do_query_service (std::wstring service_name);
result_type WINAPI do_enable_service (std::wstring service_name, bool enable);
result_type WINAPI do_delete_service (std::wstring service_name);

void print_usage (fs::path const & program_name)
{
    fmt::println(tr::_("Description:"));
    fmt::println(tr::_("    Command-line tool that configures a service.\n"));
    fmt::println(tr::_("Usage:"));
    fmt::println(tr::f_("    {} COMMAND SERVICE_NAME\n", program_name));
    fmt::println(tr::_("Commands:"));
    fmt::println(tr::_("    query"));
    fmt::println(tr::_("    disable"));
    fmt::println(tr::_("    enable"));
    fmt::println(tr::_("    delete"));
    fmt::println(tr::_("    describe"));
}

int __cdecl wmain (int argc, WCHAR * argv[])
{
    auto command_line = pfs::make_argvapi(argc, argv);

    if( argc != 3 ) {
        print_error(tr::_("Incorrect number of arguments\n"));
        print_usage(command_line.program_name());
        return EXIT_FAILURE;
    }

    auto it = command_line.begin();
    auto command_name = it.next().arg();
    auto service_name = it.next().arg();

    result_type res;
    bool print_usage_on_error = false;

    if (command_name == L"query") {
        res = do_query_service(to_string(service_name));
    } else if (command_name == L"disable") {
        res = do_enable_service(to_string(service_name), false);
    } else if (command_name == L"enable") {
        res = do_enable_service(to_string(service_name), true);
    } else if (command_name == L"delete") {
        res = do_delete_service(to_string(service_name));
    } else if (command_name == L"describe") {
        //res = do_enable_service(to_string(service_name));
    } else {
        res = pfs::make_unexpected(tr::f_("Unknown command: {}", pfs::windows::utf8_encode(
            command_name.begin(), command_name.size())));
    }

    if (!res) {
        fmt::println(stderr, res.error());

        if (print_usage_on_error)
            print_usage(command_line.program_name());

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * Retrieves and displays the current service configuration.
 */
result_type WINAPI do_query_service (std::wstring service_name)
{
    // Get a handle to the SCM database. 
    auto sc_manager = make_manager();

    if (!sc_manager) {
        return pfs::make_unexpected(tr::f_("OpenSCManager: {}", pfs::get_last_system_error().message()));
    }

    // Get a handle to the service.
    auto sc_service = make_service(& *sc_manager, service_name.c_str(), SERVICE_QUERY_CONFIG);

    if (!sc_service) {
        return pfs::make_unexpected(tr::f_("OpenService: {}", pfs::get_last_system_error().message()));
    }

    // Get the configuration information.
    DWORD dwBytesNeeded = 0;
    DWORD cbBufSize = 0; 
    LPQUERY_SERVICE_CONFIGW lpsc; 
    auto success = QueryServiceConfigW(& *sc_service, nullptr, 0, & dwBytesNeeded) == TRUE;

    if (!success) {
        auto dwError = GetLastError();

        if (dwError == ERROR_INSUFFICIENT_BUFFER) {
            cbBufSize = dwBytesNeeded;
            lpsc = static_cast<LPQUERY_SERVICE_CONFIGW>(LocalAlloc(LMEM_FIXED, cbBufSize));

            if (lpsc == nullptr)
                return pfs::make_unexpected(tr::f_("LocalAlloc", pfs::get_last_system_error().message()));

        } else {
            return pfs::make_unexpected(tr::f_("QueryServiceConfig: {}"
                , pfs::get_last_system_error().message()));
        }
    }

    auto lpsc_guard = pfs::make_scope_exit([lpsc] { LocalFree(lpsc); });

    success = QueryServiceConfigW(& *sc_service, lpsc, cbBufSize, & dwBytesNeeded) == TRUE;

    if (!success) {
        return pfs::make_unexpected(tr::f_("QueryServiceConfig: {}"
            , pfs::get_last_system_error().message()));
    }

    LPSERVICE_DESCRIPTIONW lpsd;
    success = QueryServiceConfig2W(& *sc_service, SERVICE_CONFIG_DESCRIPTION, nullptr, 0, & dwBytesNeeded) != FALSE;
        
    if (!success) {
        auto dwError = GetLastError();

        if (dwError == ERROR_INSUFFICIENT_BUFFER) {
            cbBufSize = dwBytesNeeded;
            lpsd = static_cast<LPSERVICE_DESCRIPTIONW>(LocalAlloc(LMEM_FIXED, cbBufSize));

            if (lpsd == nullptr)
                return pfs::make_unexpected(tr::f_("LocalAlloc", pfs::get_last_system_error().message()));
        } else {
            return pfs::make_unexpected(tr::f_("QueryServiceConfig2: {}"
                , pfs::get_last_system_error().message()));
        }
    }

    auto lpsd_guard = pfs::make_scope_exit([lpsd] { LocalFree(lpsd); });

    success = QueryServiceConfig2W(& *sc_service, SERVICE_CONFIG_DESCRIPTION, (LPBYTE)lpsd
        , cbBufSize, & dwBytesNeeded) != FALSE;

    if (!success) {
        return pfs::make_unexpected(tr::f_("QueryServiceConfig2: {}"
            , pfs::get_last_system_error().message()));
    }

    // Print the configuration information.

    fmt::println(tr::f_("{} configuration:"), pfs::windows::utf8_encode(service_name));
    fmt::println(tr::f_("  Type: 0x{:x}"), lpsc->dwServiceType);
    fmt::println(tr::f_("  Start Type: 0x{:x}"), lpsc->dwStartType);
    fmt::println(tr::f_("  Error Control: 0x{:x}"), lpsc->dwErrorControl);
    fmt::println(tr::f_("  Binary path: {}"), pfs::windows::utf8_encode(lpsc->lpBinaryPathName));
    fmt::println(tr::f_("  Account: {}"), pfs::windows::utf8_encode(lpsc->lpServiceStartName));

    if (lpsd->lpDescription != nullptr && lpsd->lpDescription[0] != 0)
        fmt::println(tr::f_("  Description: {}"), pfs::windows::utf8_encode(lpsd->lpDescription));

    if (lpsc->lpLoadOrderGroup != NULL && lpsc->lpLoadOrderGroup[0] != 0)
        fmt::println(tr::f_("  Load order group: {}"), pfs::windows::utf8_encode(lpsc->lpLoadOrderGroup));

    if (lpsc->dwTagId != 0)
        fmt::println(tr::f_("  Tag ID: {}", lpsc->dwTagId));
 
    if (lpsc->lpDependencies != NULL && lpsc->lpDependencies[0] != 0)
        fmt::println(tr::f_("  Dependencies: {}", pfs::windows::utf8_encode(lpsc->lpDependencies)));

    return true;
}

/**
 * Enables/Disables the service.
 */ 
result_type WINAPI do_enable_service (std::wstring service_name, bool enable)
{
    // Get a handle to the SCM database. 
    auto sc_manager = make_manager();

    if (!sc_manager)
        return pfs::make_unexpected(tr::f_("OpenSCManager: {}", pfs::get_last_system_error().message()));

    // Get a handle to the service.
    auto sc_service = make_service(& *sc_manager, service_name.c_str(), SERVICE_CHANGE_CONFIG);

    if (!sc_service)
        return pfs::make_unexpected(tr::f_("OpenService: {}", pfs::get_last_system_error().message()));

    auto success = ChangeServiceConfigW(& *sc_service, // handle of service 
        SERVICE_NO_CHANGE, // service type: no change 
        enable ? SERVICE_DEMAND_START : SERVICE_DISABLED,  // service start type 
        SERVICE_NO_CHANGE, // error control: no change 
        NULL,              // binary path: no change 
        NULL,              // load order group: no change 
        NULL,              // tag ID: no change 
        NULL,              // dependencies: no change 
        NULL,              // account name: no change 
        NULL,              // password: no change 
        NULL) == TRUE;     // display name: no change

    // Change the service start type.
    if (!success) {
        return pfs::make_unexpected(tr::f_("ChangeServiceConfig: {}"
            , pfs::get_last_system_error().message()));
    } 

    fmt::println(tr::f_("Service {} successfully", (enable ? tr::_("enabled") : tr::_("disabled")))); 

    return true;
}

/**
 * Deletes a service from the SCM database.
 */
result_type WINAPI do_delete_service (std::wstring service_name)
{
    // Get a handle to the SCM database. 
    auto sc_manager = make_manager();

    if (!sc_manager)
        return pfs::make_unexpected(tr::f_("OpenSCManager: {}", pfs::get_last_system_error().message()));

    // Get a handle to the service.
    auto sc_service = make_service(& *sc_manager, service_name.c_str(), DELETE);

    // Delete the service.
    if (!sc_service)
        return pfs::make_unexpected(tr::f_("OpenService: {}", pfs::get_last_system_error().message()));

    auto success = DeleteService(& *sc_service) == TRUE;

    if (!success)
        return pfs::make_unexpected(tr::f_("DeleteService: {}", pfs::get_last_system_error().message()));

    fmt::println(tr::_("Service deleted successfully")) ;

    return true;
}

#if __FIXME__
//
// Purpose: 
//   Updates the service description to "This is a test description".
//
// Parameters:
//   None
// 
// Return value:
//   None
//
VOID __stdcall DoUpdateSvcDesc()
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    SERVICE_DESCRIPTION sd;
    LPTSTR szDesc = TEXT("This is a test description");

    // Get a handle to the SCM database. 

    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (NULL == schSCManager) 
    {
        printf("OpenSCManager failed (%d)\n", GetLastError());
        return;
    }

    // Get a handle to the service.

    schService = OpenService( 
        schSCManager,            // SCM database 
        szSvcName,               // name of service 
        SERVICE_CHANGE_CONFIG);  // need change config access 

    if (schService == NULL)
    { 
        printf("OpenService failed (%d)\n", GetLastError()); 
        CloseServiceHandle(schSCManager);
        return;
    }    

    // Change the service description.

    sd.lpDescription = szDesc;

    if( !ChangeServiceConfig2(
        schService,                 // handle to service
        SERVICE_CONFIG_DESCRIPTION, // change: description
        &sd) )                      // new description
    {
        printf("ChangeServiceConfig2 failed\n");
    }
    else printf("Service description updated successfully.\n");

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}


#endif // __FIXME__