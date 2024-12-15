////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.12.15 Initial version (https://learn.microsoft.com/en-us/windows/win32/services/svcconfig-cpp).
////////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <pfs/argvapi.hpp>
#include <pfs/expected.hpp>
#include <pfs/i18n.hpp>
#include <pfs/scope.hpp>
#include <pfs/string_view.hpp>
#include <pfs/windows.hpp>
#include <pfs/ionik/error.hpp>
#include <cstdlib>
#include <memory>
#include <type_traits>

#pragma comment(lib, "advapi32.lib")

//TCHAR szCommand[10];
//TCHAR szSvcName[80];

namespace fs = pfs::filesystem;

pfs::expected<bool, std::string> __stdcall DoQuerySvc(std::wstring service_name);
VOID __stdcall DoUpdateSvcDesc(void);
VOID __stdcall DoDisableSvc(void);
VOID __stdcall DoEnableSvc(void);
VOID __stdcall DoDeleteSvc(void);

void __stdcall print_usage (fs::path const & program_name)
{
    fmt::println(tr::_("Description:"));
    fmt::println(tr::_("    Command-line tool that configures a service.\n"));
    fmt::println(tr::_("Usage:"));
    fmt::println(tr::f_("    {} COMMAND SERVICE_NAME\n", program_name));
    fmt::println(tr::_("Commands:"));
    fmt::println(tr::_("    query"));
    fmt::println(tr::_("    describe"));
    fmt::println(tr::_("    disable"));
    fmt::println(tr::_("    enable"));
    fmt::println(tr::_("    delete"));
}

int __cdecl wmain (int argc, WCHAR * argv[])
{
    auto command_line = pfs::make_argvapi(argc, argv);

    if( argc != 3 ) {
        fmt::println(stderr, tr::_("Incorrect number of arguments"));
        print_usage(command_line.program_name());
        return EXIT_FAILURE;
    }

    auto it = command_line.begin();
    auto command_name = it.next().arg();
    auto service_name = it.next().arg();

    //StringCchCopy(szCommand, 10, argv[1]);
    //StringCchCopy(szSvcName, 80, argv[2]);

    if (command_name == L"query") {
        auto res = DoQuerySvc(to_string(service_name));

        if (!res) {
            fmt::println(stderr, res.error());
            return EXIT_FAILURE;
        }
    }
    //else if (lstrcmpi( szCommand, TEXT("describe")) == 0 )
    //    DoUpdateSvcDesc();
    //else if (lstrcmpi( szCommand, TEXT("disable")) == 0 )
    //    DoDisableSvc();
    //else if (lstrcmpi( szCommand, TEXT("enable")) == 0 )
    //    DoEnableSvc();
    //else if (lstrcmpi( szCommand, TEXT("delete")) == 0 )
    //    DoDeleteSvc();
    else {
        fmt::println(stderr, tr::f_("Unknown command: {}\n", pfs::windows::utf8_encode(
            command_name.begin(), command_name.size())));
        print_usage(command_line.program_name());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * Retrieves and displays the current service configuration.
 */
pfs::expected<bool, std::string> __stdcall DoQuerySvc (std::wstring service_name)
{
    // Get a handle to the SCM database. 
    LPCWSTR machine_name = nullptr;  // Local computer
    LPCWSTR database_name = nullptr; // ServicesActive database 
    auto schSCManager = OpenSCManagerW(machine_name, database_name, SC_MANAGER_ALL_ACCESS);

    if (schSCManager == nullptr) {
        return pfs::make_unexpected(tr::f_("OpenSCManager ФВВ: {}"
            , pfs::get_last_system_error().message()));
    }

    auto schSCManager_guard = pfs::make_scope_exit([schSCManager] {
        CloseServiceHandle(schSCManager);
    });

    // Get a handle to the service.

    SC_HANDLE schService = OpenServiceW(schSCManager, service_name.c_str(), SERVICE_QUERY_CONFIG); // need query config access 

    if (schService == nullptr) {
        return pfs::make_unexpected(tr::f_("OpenServiceW: {}"
            , pfs::get_last_system_error().message()));
    }

    auto schService_guard = pfs::make_scope_exit([schService] {
        CloseServiceHandle(schService);
    });

    // Get the configuration information.
    DWORD dwBytesNeeded;
    DWORD cbBufSize; 
    LPQUERY_SERVICE_CONFIGW lpsc; 
    auto success = QueryServiceConfigW(schService, nullptr, 0, & dwBytesNeeded) != FALSE;

    if (!success) {
        auto dwError = GetLastError();

        if (dwError == ERROR_INSUFFICIENT_BUFFER) {
            cbBufSize = dwBytesNeeded;
            lpsc = static_cast<LPQUERY_SERVICE_CONFIGW>(LocalAlloc(LMEM_FIXED, cbBufSize));
        } else {
            return pfs::make_unexpected(tr::f_("QueryServiceConfig: {}"
                , pfs::get_last_system_error().message()));
        }
    }

    auto lpsc_guard = pfs::make_scope_exit([lpsc] { LocalFree(lpsc); });

    success = QueryServiceConfigW(schService, lpsc, cbBufSize, & dwBytesNeeded) != FALSE;

    if (!success) {
        return pfs::make_unexpected(tr::f_("QueryServiceConfig: {}"
            , pfs::get_last_system_error().message()));
    }

    LPSERVICE_DESCRIPTIONW lpsd;
    success = QueryServiceConfig2W(schService, SERVICE_CONFIG_DESCRIPTION, nullptr, 0, & dwBytesNeeded) != FALSE;
        
    if (!success) {
        auto dwError = GetLastError();

        if (dwError == ERROR_INSUFFICIENT_BUFFER) {
            cbBufSize = dwBytesNeeded;
            lpsd = static_cast<LPSERVICE_DESCRIPTIONW>(LocalAlloc(LMEM_FIXED, cbBufSize));
        } else {
            return pfs::make_unexpected(tr::f_("QueryServiceConfig2: {}"
                , pfs::get_last_system_error().message()));
        }
    }

    auto lpsd_guard = pfs::make_scope_exit([lpsd] { LocalFree(lpsd); });

    success = QueryServiceConfig2W(schService, SERVICE_CONFIG_DESCRIPTION, (LPBYTE)lpsd
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

#if __FIXME__
//
// Purpose: 
//   Disables the service.
//
// Parameters:
//   None
// 
// Return value:
//   None
//
VOID __stdcall DoDisableSvc()
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;

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

    // Change the service start type.

    if (! ChangeServiceConfig( 
        schService,        // handle of service 
        SERVICE_NO_CHANGE, // service type: no change 
        SERVICE_DISABLED,  // service start type 
        SERVICE_NO_CHANGE, // error control: no change 
        NULL,              // binary path: no change 
        NULL,              // load order group: no change 
        NULL,              // tag ID: no change 
        NULL,              // dependencies: no change 
        NULL,              // account name: no change 
        NULL,              // password: no change 
        NULL) )            // display name: no change
    {
        printf("ChangeServiceConfig failed (%d)\n", GetLastError()); 
    }
    else printf("Service disabled successfully.\n"); 

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

//
// Purpose: 
//   Enables the service.
//
// Parameters:
//   None
// 
// Return value:
//   None
//
VOID __stdcall DoEnableSvc()
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;

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

    // Change the service start type.

    if (! ChangeServiceConfig( 
        schService,            // handle of service 
        SERVICE_NO_CHANGE,     // service type: no change 
        SERVICE_DEMAND_START,  // service start type 
        SERVICE_NO_CHANGE,     // error control: no change 
        NULL,                  // binary path: no change 
        NULL,                  // load order group: no change 
        NULL,                  // tag ID: no change 
        NULL,                  // dependencies: no change 
        NULL,                  // account name: no change 
        NULL,                  // password: no change 
        NULL) )                // display name: no change
    {
        printf("ChangeServiceConfig failed (%d)\n", GetLastError()); 
    }
    else printf("Service enabled successfully.\n"); 

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

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

//
// Purpose: 
//   Deletes a service from the SCM database
//
// Parameters:
//   None
// 
// Return value:
//   None
//
VOID __stdcall DoDeleteSvc()
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;

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
        schSCManager,       // SCM database 
        szSvcName,          // name of service 
        DELETE);            // need delete access 

    if (schService == NULL)
    { 
        printf("OpenService failed (%d)\n", GetLastError()); 
        CloseServiceHandle(schSCManager);
        return;
    }

    // Delete the service.

    if (! DeleteService(schService) ) 
    {
        printf("DeleteService failed (%d)\n", GetLastError()); 
    }
    else printf("Service deleted successfully\n"); 

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

#endif // __FIXME__