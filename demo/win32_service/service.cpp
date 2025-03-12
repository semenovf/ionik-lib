////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.12.15 Initial version (https://learn.microsoft.com/en-us/windows/win32/services/svc-cpp).
////////////////////////////////////////////////////////////////////////////////
#include "service_config.hpp"
#include "utils.hpp"
#include <pfs/argvapi.hpp>
#include <pfs/expected.hpp>
#include <pfs/string_view.hpp>
#include <pfs/windows.hpp>
#include <windows.h>
#include "messages.h"
#include <cstdlib>

#pragma comment(lib, "advapi32.lib")

namespace fs = pfs::filesystem;
using result_type = pfs::expected<bool, std::string>;

//SERVICE_STATUS          gSvcStatus; 
//SERVICE_STATUS_HANDLE   gSvcStatusHandle; 
//HANDLE                  ghSvcStopEvent = NULL;

result_type WINAPI do_install_service (std::wstring service_name);

//VOID SvcInstall(void);
//VOID WINAPI SvcCtrlHandler( DWORD ); 
//VOID WINAPI SvcMain( DWORD, LPTSTR * ); 
//
//VOID ReportSvcStatus( DWORD, DWORD, DWORD );
//VOID SvcInit( DWORD, LPTSTR * ); 
//VOID SvcReportEvent( LPTSTR );

void print_usage (fs::path const & program_name)
{
    fmt::println(tr::_("Description:"));
    fmt::println(tr::_("    Command-line tool that installs a service.\n"));
    fmt::println(tr::_("Usage:"));
    fmt::println(tr::f_("    {} -h | --help", program_name));
    fmt::println(tr::_ ("        Print this usage\n"));
    fmt::println(tr::f_("    {} install", program_name));
    fmt::println(tr::_ ("        Install the service"));
}

int __cdecl wmain (int argc, WCHAR * argv[]) 
{ 
    auto command_line = pfs::make_argvapi(argc, argv);
    auto it = command_line.begin();

    if (it.has_more()) {
        auto item = it.next();

        if (item.is_option()) {
            if (item.optname() == L"h" || item.optname() == L"help") {
                print_usage(command_line.program_name());
                return EXIT_SUCCESS;
            } else {
                print_error(tr::_("Unexpected option\n"));
                print_usage(command_line.program_name());
                return EXIT_FAILURE;
            }
        } else {
            auto command_name = item.arg();

            // Install the service. 
            if (command_name == L"install") {
                auto res = do_install_service(kSERVICE_NAME);

                if (!res)
                    return EXIT_FAILURE;
            }
        }
    }

#if __FIXME__    
    // The service is probably being started by the SCM.

    // TO_DO: Add any additional services for the process to this table.
    SERVICE_TABLE_ENTRY DispatchTable[] = { 
        { kSERVICE_NAME, (LPSERVICE_MAIN_FUNCTION) SvcMain }, 
        { NULL, NULL } 
    }; 

    // This call returns when the service has stopped. 
    // The process should simply terminate when the call returns.

    if (!StartServiceCtrlDispatcher( DispatchTable )) 
    { 
        SvcReportEvent(TEXT("StartServiceCtrlDispatcher")); 
    } 
#endif // __FIXME__
} 

result_type WINAPI do_install_service (std::wstring service_name)
{
    return true;
}
#if __FIXME__
//
// Purpose: 
//   Installs a service in the SCM database
//
// Parameters:
//   None
// 
// Return value:
//   None
//
VOID SvcInstall()
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    TCHAR szUnquotedPath[MAX_PATH];

    if( !GetModuleFileName( NULL, szUnquotedPath, MAX_PATH ) )
    {
        printf("Cannot install service (%d)\n", GetLastError());
        return;
    }

    // In case the path contains a space, it must be quoted so that
    // it is correctly interpreted. For example,
    // "d:\my share\myservice.exe" should be specified as
    // ""d:\my share\myservice.exe"".
    TCHAR szPath[MAX_PATH];
    StringCbPrintf(szPath, MAX_PATH, TEXT("\"%s\""), szUnquotedPath);

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

    // Create the service

    schService = CreateService( 
        schSCManager,              // SCM database 
        SVCNAME,                   // name of service 
        SVCNAME,                   // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_WIN32_OWN_PROCESS, // service type 
        SERVICE_DEMAND_START,      // start type 
        SERVICE_ERROR_NORMAL,      // error control type 
        szPath,                    // path to service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL);                     // no password 

    if (schService == NULL) 
    {
        printf("CreateService failed (%d)\n", GetLastError()); 
        CloseServiceHandle(schSCManager);
        return;
    }
    else printf("Service installed successfully\n"); 

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

//
// Purpose: 
//   Entry point for the service
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None.
//
VOID WINAPI SvcMain( DWORD dwArgc, LPTSTR *lpszArgv )
{
    // Register the handler function for the service

    gSvcStatusHandle = RegisterServiceCtrlHandler( 
        SVCNAME, 
        SvcCtrlHandler);

    if( !gSvcStatusHandle )
    { 
        SvcReportEvent(TEXT("RegisterServiceCtrlHandler")); 
        return; 
    } 

    // These SERVICE_STATUS members remain as set here

    gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
    gSvcStatus.dwServiceSpecificExitCode = 0;    

    // Report initial status to the SCM

    ReportSvcStatus( SERVICE_START_PENDING, NO_ERROR, 3000 );

    // Perform service-specific initialization and work.

    SvcInit( dwArgc, lpszArgv );
}

//
// Purpose: 
//   The service code
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None
//
VOID SvcInit( DWORD dwArgc, LPTSTR *lpszArgv)
{
    // TO_DO: Declare and set any required variables.
    //   Be sure to periodically call ReportSvcStatus() with 
    //   SERVICE_START_PENDING. If initialization fails, call
    //   ReportSvcStatus with SERVICE_STOPPED.

    // Create an event. The control handler function, SvcCtrlHandler,
    // signals this event when it receives the stop control code.

    ghSvcStopEvent = CreateEvent(
        NULL,    // default security attributes
        TRUE,    // manual reset event
        FALSE,   // not signaled
        NULL);   // no name

    if ( ghSvcStopEvent == NULL)
    {
        ReportSvcStatus( SERVICE_STOPPED, GetLastError(), 0 );
        return;
    }

    // Report running status when initialization is complete.

    ReportSvcStatus( SERVICE_RUNNING, NO_ERROR, 0 );

    // TO_DO: Perform work until service stops.

    while(1)
    {
        // Check whether to stop the service.

        WaitForSingleObject(ghSvcStopEvent, INFINITE);

        ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
        return;
    }
}

//
// Purpose: 
//   Sets the current service status and reports it to the SCM.
//
// Parameters:
//   dwCurrentState - The current state (see SERVICE_STATUS)
//   dwWin32ExitCode - The system error code
//   dwWaitHint - Estimated time for pending operation, 
//     in milliseconds
// 
// Return value:
//   None
//
VOID ReportSvcStatus( DWORD dwCurrentState,
    DWORD dwWin32ExitCode,
    DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

    // Fill in the SERVICE_STATUS structure.

    gSvcStatus.dwCurrentState = dwCurrentState;
    gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
    gSvcStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_START_PENDING)
        gSvcStatus.dwControlsAccepted = 0;
    else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if ( (dwCurrentState == SERVICE_RUNNING) ||
        (dwCurrentState == SERVICE_STOPPED) )
        gSvcStatus.dwCheckPoint = 0;
    else gSvcStatus.dwCheckPoint = dwCheckPoint++;

    // Report the status of the service to the SCM.
    SetServiceStatus( gSvcStatusHandle, &gSvcStatus );
}

//
// Purpose: 
//   Called by SCM whenever a control code is sent to the service
//   using the ControlService function.
//
// Parameters:
//   dwCtrl - control code
// 
// Return value:
//   None
//
VOID WINAPI SvcCtrlHandler( DWORD dwCtrl )
{
    // Handle the requested control code. 

    switch(dwCtrl) 
    {  
    case SERVICE_CONTROL_STOP: 
        ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

        // Signal the service to stop.

        SetEvent(ghSvcStopEvent);
        ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

        return;

    case SERVICE_CONTROL_INTERROGATE: 
        break; 

    default: 
        break;
    } 

}

//
// Purpose: 
//   Logs messages to the event log
//
// Parameters:
//   szFunction - name of function that failed
// 
// Return value:
//   None
//
// Remarks:
//   The service must have an entry in the Application event log.
//
VOID SvcReportEvent(LPTSTR szFunction) 
{ 
    LPCTSTR lpszStrings[2];
    WCHAR Buffer[80];

    auto hEventSource = RegisterEventSource(NULL, SVCNAME);

    if( NULL != hEventSource ) {
        StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

        lpszStrings[0] = SVCNAME;
        lpszStrings[1] = Buffer;

        ReportEvent(hEventSource, // event log handle
            EVENTLOG_ERROR_TYPE,  // event type
            0,                    // event category
            SVC_ERROR,            // event identifier
            NULL,                 // no security identifier
            2,                    // size of lpszStrings array
            0,                    // no binary data
            lpszStrings,          // array of strings
            NULL);                // no binary data

        DeregisterEventSource(hEventSource);
    }
}
#endif // __FIXME__