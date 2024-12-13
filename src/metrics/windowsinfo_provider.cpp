////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.12.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/tag.hpp"
#include "ionik/metrics/windowsinfo_provider.hpp"
#include <pfs/getenv.hpp>
#include <pfs/log.hpp>
#include <pfs/numeric_cast.hpp>
#include <pfs/optional.hpp>
#include <windows.h>
#include <winnt.h>
#include <bcrypt.h>         // NTSTATUS
#include <intrin.h>         // __cpuid
#include <sysinfoapi.h>     // GetProductInfo
//#include <versionhelpers.h> // VerifyVersionInfoA
#include <libloaderapi.h>   // GetProcAddress, GetModuleHandleA
#include <array>
#include <cstring>


IONIK__NAMESPACE_BEGIN

namespace metrics {

// https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getproductinfo
static char const * stringify_product_type (DWORD product_type)
{
    switch (product_type) {
        case PRODUCT_ULTIMATE                            : return "Ultimate";
        case PRODUCT_HOME_BASIC                          : return "Home Basic";
        case PRODUCT_HOME_PREMIUM                        : return "Home Premium";
        case PRODUCT_ENTERPRISE                          : return "Windows 10 Enterprise";
        case PRODUCT_HOME_BASIC_N                        : return "Home Basic N";
        case PRODUCT_BUSINESS                            : return "Business";
        case PRODUCT_STANDARD_SERVER                     : return "Server Standard (full installation)";
        case PRODUCT_DATACENTER_SERVER                   : return "Server Datacenter (full installation)";
        case PRODUCT_SMALLBUSINESS_SERVER                : return "Windows Small Business Server";
        case PRODUCT_ENTERPRISE_SERVER                   : return "Server Enterprise (full installation)";
        case PRODUCT_STARTER                             : return "Starter";
        case PRODUCT_DATACENTER_SERVER_CORE              : return "Server Datacenter (core installation)";
        case PRODUCT_STANDARD_SERVER_CORE                : return "Server Standard (core installation)";
        case PRODUCT_ENTERPRISE_SERVER_CORE              : return "Server Enterprise (core installation)";
        case PRODUCT_ENTERPRISE_SERVER_IA64              : return "Server Enterprise for Itanium-based Systems";
        case PRODUCT_BUSINESS_N                          : return "Business N";
        case PRODUCT_WEB_SERVER                          : return "Web Server (full installation)";
        case PRODUCT_CLUSTER_SERVER                      : return "HPC Edition";
        case PRODUCT_HOME_SERVER                         : return "Windows Storage Server 2008 R2 Essentials";
        case PRODUCT_STORAGE_EXPRESS_SERVER              : return "Storage Server Express";
        case PRODUCT_STORAGE_STANDARD_SERVER             : return "Storage Server Standard";
        case PRODUCT_STORAGE_WORKGROUP_SERVER            : return "Storage Server Workgroup";
        case PRODUCT_STORAGE_ENTERPRISE_SERVER           : return "Storage Server Enterprise";
        case PRODUCT_SERVER_FOR_SMALLBUSINESS            : return "Windows Server 2008 for Windows Essential Server Solutions";
        case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM        : return "Small Business Server Premium";
        case PRODUCT_HOME_PREMIUM_N                      : return "Home Premium N";
        case PRODUCT_ENTERPRISE_N                        : return "Windows 10 Enterprise N";
        case PRODUCT_ULTIMATE_N                          : return "Ultimate N";
        case PRODUCT_WEB_SERVER_CORE                     : return "Web Server (core installation)";
        case PRODUCT_MEDIUMBUSINESS_SERVER_MANAGEMENT    : return "Windows Essential Business Server Management Server";
        case PRODUCT_MEDIUMBUSINESS_SERVER_SECURITY      : return "Windows Essential Business Server Security Server";
        case PRODUCT_MEDIUMBUSINESS_SERVER_MESSAGING     : return "Windows Essential Business Server Messaging Server";
        case PRODUCT_SERVER_FOUNDATION                   : return "Server Foundation";
        case PRODUCT_HOME_PREMIUM_SERVER                 : return "Windows Home Server 2011";
        case PRODUCT_SERVER_FOR_SMALLBUSINESS_V          : return "Windows Server 2008 without Hyper-V for Windows Essential Server Solutions";
        case PRODUCT_STANDARD_SERVER_V                   : return "Server Standard without Hyper-V";
        case PRODUCT_DATACENTER_SERVER_V                 : return "Server Datacenter without Hyper-V (full installation)";
        case PRODUCT_ENTERPRISE_SERVER_V                 : return "Server Enterprise without Hyper-V (full installation)";
        case PRODUCT_DATACENTER_SERVER_CORE_V            : return "Server Datacenter without Hyper-V (core installation)";
        case PRODUCT_STANDARD_SERVER_CORE_V              : return "Server Standard without Hyper-V (core installation)";
        case PRODUCT_ENTERPRISE_SERVER_CORE_V            : return "Server Enterprise without Hyper-V (core installation)";
        case PRODUCT_HYPERV                              : return "Microsoft Hyper-V Server";
        case PRODUCT_STORAGE_EXPRESS_SERVER_CORE         : return "Storage Server Express (core installation)";
        case PRODUCT_STORAGE_STANDARD_SERVER_CORE        : return "Storage Server Standard (core installation)";
        case PRODUCT_STORAGE_WORKGROUP_SERVER_CORE       : return "Storage Server Workgroup (core installation)";
        case PRODUCT_STORAGE_ENTERPRISE_SERVER_CORE      : return "Storage Server Enterprise (core installation)";
        case PRODUCT_STARTER_N                           : return "Starter N";
        case PRODUCT_PROFESSIONAL                        : return "Windows 10 Pro";
        case PRODUCT_PROFESSIONAL_N                      : return "Windows 10 Pro N";
        case PRODUCT_SB_SOLUTION_SERVER                  : return "Windows Small Business Server 2011 Essentials";
        case PRODUCT_SERVER_FOR_SB_SOLUTIONS             : return "Server For SB Solutions";
        case PRODUCT_STANDARD_SERVER_SOLUTIONS           : return "Server Solutions Premium";
        case PRODUCT_STANDARD_SERVER_SOLUTIONS_CORE      : return "Server Solutions Premium (core installation)";
        case PRODUCT_SB_SOLUTION_SERVER_EM               : return "Server For SB Solutions EM";
        case PRODUCT_SERVER_FOR_SB_SOLUTIONS_EM          : return "Server For SB Solutions EM";
        case PRODUCT_SOLUTION_EMBEDDEDSERVER             : return "Solution Embedded Server";
        case PRODUCT_SOLUTION_EMBEDDEDSERVER_CORE        : return "Solution Embedded Server (core installation)";
        case PRODUCT_PROFESSIONAL_EMBEDDED               : return "Professional Embedded";
        case PRODUCT_ESSENTIALBUSINESS_SERVER_MGMT       : return "Windows Essential Server Solution Management";
        case PRODUCT_ESSENTIALBUSINESS_SERVER_ADDL       : return "Windows Essential Server Solution Additional";
        case PRODUCT_ESSENTIALBUSINESS_SERVER_MGMTSVC    : return "Windows Essential Server Solution Management SVC";
        case PRODUCT_ESSENTIALBUSINESS_SERVER_ADDLSVC    : return "Windows Essential Server Solution Additional SVC";
        case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE   : return "Small Business Server Premium (core installation)";
        case PRODUCT_CLUSTER_SERVER_V                    : return "Server Hyper Core V";
        case PRODUCT_EMBEDDED                            : return "Embedded";
        case PRODUCT_STARTER_E                           : return "Starter E"; // Not supported
        case PRODUCT_HOME_BASIC_E                        : return "Home Basic E"; // Not supported
        case PRODUCT_HOME_PREMIUM_E                      : return "Home Premium E"; // Not supported
        case PRODUCT_PROFESSIONAL_E                      : return "Windows 10 Pro E"; // Not supported
        case PRODUCT_ENTERPRISE_E                        : return "Windows 10 Enterprise E";
        case PRODUCT_ULTIMATE_E                          : return "Ultimate E"; // Not supported
        case PRODUCT_ENTERPRISE_EVALUATION               : return "Windows 10 Enterprise Evaluation";
        case PRODUCT_MULTIPOINT_STANDARD_SERVER          : return "Windows MultiPoint Server Standard (full installation)";
        case PRODUCT_MULTIPOINT_PREMIUM_SERVER           : return "Windows MultiPoint Server Premium (full installation)";
        case PRODUCT_STANDARD_EVALUATION_SERVER          : return "Server Standard (evaluation installation)";
        case PRODUCT_DATACENTER_EVALUATION_SERVER        : return "Server Datacenter (evaluation installation)";
        case PRODUCT_ENTERPRISE_N_EVALUATION             : return "Windows 10 Enterprise N Evaluation";
        case PRODUCT_EMBEDDED_AUTOMOTIVE                 : return "PRODUCT_EMBEDDED_AUTOMOTIVE";
        case PRODUCT_EMBEDDED_INDUSTRY_A                 : return "PRODUCT_EMBEDDED_INDUSTRY_A";
        case PRODUCT_THINPC                              : return "PRODUCT_THINPC";
        case PRODUCT_EMBEDDED_A                          : return "PRODUCT_EMBEDDED_A";
        case PRODUCT_EMBEDDED_INDUSTRY                   : return "PRODUCT_EMBEDDED_INDUSTRY";
        case PRODUCT_EMBEDDED_E                          : return "PRODUCT_EMBEDDED_E";
        case PRODUCT_EMBEDDED_INDUSTRY_E                 : return "PRODUCT_EMBEDDED_INDUSTRY_E";
        case PRODUCT_EMBEDDED_INDUSTRY_A_E               : return "PRODUCT_EMBEDDED_INDUSTRY_A_E";
        case PRODUCT_STORAGE_WORKGROUP_EVALUATION_SERVER : return "Storage Server Workgroup (evaluation installation)";
        case PRODUCT_STORAGE_STANDARD_EVALUATION_SERVER  : return "Storage Server Standard (evaluation installation)";
        case PRODUCT_CORE_ARM                            : return "PRODUCT_CORE_ARM";
        case PRODUCT_CORE_N                              : return "Windows 10 Home N";
        case PRODUCT_CORE_COUNTRYSPECIFIC                : return "Windows 10 Home China";
        case PRODUCT_CORE_SINGLELANGUAGE                 : return "Windows 10 Home Single Language";
        case PRODUCT_CORE                                : return "Windows 10 Home";
        case PRODUCT_PROFESSIONAL_WMC                    : return "Professional with Media Center";
        case PRODUCT_EMBEDDED_INDUSTRY_EVAL              : return "PRODUCT_EMBEDDED_INDUSTRY_EVAL";
        case PRODUCT_EMBEDDED_INDUSTRY_E_EVAL            : return "PRODUCT_EMBEDDED_INDUSTRY_E_EVAL";
        case PRODUCT_EMBEDDED_EVAL                       : return "PRODUCT_EMBEDDED_EVAL";
        case PRODUCT_EMBEDDED_E_EVAL                     : return "PRODUCT_EMBEDDED_E_EVAL";
        case PRODUCT_NANO_SERVER                         : return "PRODUCT_NANO_SERVER";
        case PRODUCT_CLOUD_STORAGE_SERVER                : return "PRODUCT_CLOUD_STORAGE_SERVER";
        case PRODUCT_CORE_CONNECTED                      : return "PRODUCT_CORE_CONNECTED";
        case PRODUCT_PROFESSIONAL_STUDENT                : return "PRODUCT_PROFESSIONAL_STUDENT";
        case PRODUCT_CORE_CONNECTED_N                    : return "PRODUCT_CORE_CONNECTED_N";
        case PRODUCT_PROFESSIONAL_STUDENT_N              : return "PRODUCT_PROFESSIONAL_STUDENT_N";
        case PRODUCT_CORE_CONNECTED_SINGLELANGUAGE       : return "PRODUCT_CORE_CONNECTED_SINGLELANGUAGE";
        case PRODUCT_CORE_CONNECTED_COUNTRYSPECIFIC      : return "PRODUCT_CORE_CONNECTED_COUNTRYSPECIFIC";
        case PRODUCT_CONNECTED_CAR                       : return "PRODUCT_CONNECTED_CAR";
        case PRODUCT_INDUSTRY_HANDHELD                   : return "PRODUCT_INDUSTRY_HANDHELD";
        case PRODUCT_PPI_PRO                             : return "Windows 10 Team";
        case PRODUCT_ARM64_SERVER                        : return "PRODUCT_ARM64_SERVER";
        case PRODUCT_EDUCATION                           : return "Windows 10 Education";
        case PRODUCT_EDUCATION_N                         : return "Windows 10 Education N";
        case PRODUCT_IOTUAP                              : return "Windows 10 IoT Core";
        case PRODUCT_CLOUD_HOST_INFRASTRUCTURE_SERVER    : return "PRODUCT_CLOUD_HOST_INFRASTRUCTURE_SERVER";
        case PRODUCT_ENTERPRISE_S                        : return "Windows 10 Enterprise 2015 LTSB";
        case PRODUCT_ENTERPRISE_S_N                      : return "Windows 10 Enterprise 2015 LTSB N";
        case PRODUCT_PROFESSIONAL_S                      : return "PRODUCT_PROFESSIONAL_S";
        case PRODUCT_PROFESSIONAL_S_N                    : return "PRODUCT_PROFESSIONAL_S_N";
        case PRODUCT_ENTERPRISE_S_EVALUATION             : return "Windows 10 Enterprise 2015 LTSB Evaluation";
        case PRODUCT_ENTERPRISE_S_N_EVALUATION           : return "Windows 10 Enterprise 2015 LTSB N Evaluation";
        case PRODUCT_HOLOGRAPHIC                         : return "PRODUCT_HOLOGRAPHIC";
        case PRODUCT_HOLOGRAPHIC_BUSINESS                : return "PRODUCT_HOLOGRAPHIC_BUSINESS";
        case PRODUCT_PRO_SINGLE_LANGUAGE                 : return "PRODUCT_PRO_SINGLE_LANGUAGE";
        case PRODUCT_PRO_CHINA                           : return "PRODUCT_PRO_CHINA";
        case PRODUCT_ENTERPRISE_SUBSCRIPTION             : return "PRODUCT_ENTERPRISE_SUBSCRIPTION";
        case PRODUCT_ENTERPRISE_SUBSCRIPTION_N           : return "PRODUCT_ENTERPRISE_SUBSCRIPTION_N";
        case PRODUCT_DATACENTER_NANO_SERVER              : return "PRODUCT_DATACENTER_NANO_SERVER";
        case PRODUCT_STANDARD_NANO_SERVER                : return "PRODUCT_STANDARD_NANO_SERVER";
        case PRODUCT_DATACENTER_A_SERVER_CORE            : return "PRODUCT_DATACENTER_A_SERVER_CORE";
        case PRODUCT_STANDARD_A_SERVER_CORE              : return "Server Standard, Semi-Annual Channel (core installation)";
        case PRODUCT_DATACENTER_WS_SERVER_CORE           : return "PRODUCT_DATACENTER_WS_SERVER_CORE";
        case PRODUCT_STANDARD_WS_SERVER_CORE             : return "PRODUCT_STANDARD_WS_SERVER_CORE";
        case PRODUCT_UTILITY_VM                          : return "PRODUCT_UTILITY_VM";
        case PRODUCT_DATACENTER_EVALUATION_SERVER_CORE   : return "PRODUCT_DATACENTER_EVALUATION_SERVER_CORE";
        case PRODUCT_STANDARD_EVALUATION_SERVER_CORE     : return "PRODUCT_STANDARD_EVALUATION_SERVER_CORE";
        case PRODUCT_PRO_WORKSTATION                     : return "Windows 10 Pro for Workstations";
        case PRODUCT_PRO_WORKSTATION_N                   : return "Windows 10 Pro for Workstations N";
        case PRODUCT_PRO_FOR_EDUCATION                   : return "Windows 10 Pro Education";
        case PRODUCT_PRO_FOR_EDUCATION_N                 : return "PRODUCT_PRO_FOR_EDUCATION_N";
        case PRODUCT_AZURE_SERVER_CORE                   : return "PRODUCT_AZURE_SERVER_CORE";
        case PRODUCT_AZURE_NANO_SERVER                   : return "PRODUCT_AZURE_NANO_SERVER";
        case PRODUCT_ENTERPRISEG                         : return "PRODUCT_ENTERPRISEG";
        case PRODUCT_ENTERPRISEGN                        : return "PRODUCT_ENTERPRISEGN";
        case PRODUCT_SERVERRDSH                          : return "Windows 10 Enterprise for Virtual Desktops";
        case PRODUCT_CLOUD                               : return "PRODUCT_CLOUD";
        case PRODUCT_CLOUDN                              : return "PRODUCT_CLOUDN";
        case PRODUCT_HUBOS                               : return "PRODUCT_HUBOS";
        case PRODUCT_ONECOREUPDATEOS                     : return "PRODUCT_ONECOREUPDATEOS";
        case PRODUCT_CLOUDE                              : return "PRODUCT_CLOUDE";
        case PRODUCT_IOTOS                               : return "PRODUCT_IOTOS";
        case PRODUCT_CLOUDEN                             : return "PRODUCT_CLOUDEN";
        case PRODUCT_IOTEDGEOS                           : return "PRODUCT_IOTEDGEOS";
        case PRODUCT_IOTENTERPRISE                       : return "Windows IoT Enterprise";
        case PRODUCT_LITE                                : return "PRODUCT_LITE";
        case PRODUCT_IOTENTERPRISES                      : return "Windows IoT Enterprise LTSC";
        case PRODUCT_XBOX_SYSTEMOS                       : return "PRODUCT_XBOX_SYSTEMOS";
        case PRODUCT_XBOX_NATIVEOS                       : return "PRODUCT_XBOX_NATIVEOS";
        case PRODUCT_XBOX_GAMEOS                         : return "PRODUCT_XBOX_GAMEOS";
        case PRODUCT_XBOX_ERAOS                          : return "PRODUCT_XBOX_ERAOS";
        case PRODUCT_XBOX_DURANGOHOSTOS                  : return "PRODUCT_XBOX_DURANGOHOSTOS";
        case PRODUCT_XBOX_SCARLETTHOSTOS                 : return "PRODUCT_XBOX_SCARLETTHOSTOS";
        case PRODUCT_AZURE_SERVER_CLOUDHOST              : return "PRODUCT_AZURE_SERVER_CLOUDHOST";
        case PRODUCT_AZURE_SERVER_CLOUDMOS               : return "PRODUCT_AZURE_SERVER_CLOUDMOS";
        case PRODUCT_CLOUDEDITIONN                       : return "PRODUCT_CLOUDEDITIONN";
        case PRODUCT_CLOUDEDITION                        : return "PRODUCT_CLOUDEDITION";
        case PRODUCT_AZURESTACKHCI_SERVER_CORE           : return "PRODUCT_AZURESTACKHCI_SERVER_CORE";
        case PRODUCT_DATACENTER_SERVER_AZURE_EDITION     : return "PRODUCT_DATACENTER_SERVER_AZURE_EDITION";
        case PRODUCT_DATACENTER_SERVER_CORE_AZURE_EDITION: return "PRODUCT_DATACENTER_SERVER_CORE_AZURE_EDITION";
        case PRODUCT_UNLICENSED                          : return "PRODUCT_UNLICENSED";

        case PRODUCT_UNDEFINED:
        default:
            break;
    }

    return "";
}

inline pfs::optional<OSVERSIONINFOEX> version_info_from_ntdll ()
{
    OSVERSIONINFOEX version_info;
    ZeroMemory(& version_info, sizeof(version_info));

    HMODULE ntdll_mod = GetModuleHandleA("ntdll");

    if (ntdll_mod != nullptr) {
        using RtlGetVersion_f = NTSTATUS (NTAPI *)(LPOSVERSIONINFO);

        // https://msdn.microsoft.com/en-us/library/windows/hardware/ff561910.aspx
        auto proc_addr = GetProcAddress(ntdll_mod, "RtlGetVersion");

        if (proc_addr != nullptr) {
            RtlGetVersion_f RtlGetVersion = reinterpret_cast<RtlGetVersion_f>(proc_addr);
            RtlGetVersion(reinterpret_cast<LPOSVERSIONINFO>(& version_info));
            return version_info;
        } else {
            LOGW(TAG, "GetProcAddress(\"ntdll\", \"RtlGetVersion\"): {}, error ignored"
                , pfs::get_last_system_error().message());
        }
    } else {
        LOGW(TAG, "GetModuleHandleA: {}, error ignored", pfs::get_last_system_error().message());
    }

    return pfs::nullopt;
}

struct cpu_info 
{
    std::string vendor;
    std::string brand;
};

static pfs::optional<cpu_info> cpu_info_from_cpuid ()
{
    std::array<int, 4> cpui;
    std::vector<std::array<int, 4>> data;
    std::vector<std::array<int, 4>> extdata;

    __cpuid(cpui.data(), 0);
    auto n = cpui[0];

    for (int i = 0; i <= n; ++i) {
        __cpuidex(cpui.data(), i, 0);
        data.push_back(cpui);
    }

    // Capture vendor string
    char vendor[0x20];
    std::memset(vendor, 0, sizeof(vendor));
    *reinterpret_cast<int*>(vendor) = data[0][1];
    *reinterpret_cast<int*>(vendor + 4) = data[0][3];
    *reinterpret_cast<int*>(vendor + 8) = data[0][2];

    ////////////////////////////////////////////

    // Calling __cpuid with 0x80000000 as the function_id argument
    // gets the number of the highest valid extended ID.
    __cpuid(cpui.data(), 0x80000000);
    n = cpui[0];

    char brand[0x40];
    std::memset(brand, 0, sizeof(brand));

    for (int i = 0x80000000; i <= n; ++i) {
        __cpuidex(cpui.data(), i, 0);
        extdata.push_back(cpui);
    }

    // Interpret CPU brand string if reported
    if (n >= 0x80000004) {
        std::memcpy(brand, extdata[2].data(), sizeof(cpui));
        std::memcpy(brand + 16, extdata[3].data(), sizeof(cpui));
        std::memcpy(brand + 32, extdata[4].data(), sizeof(cpui));
    }

    return cpu_info {vendor, brand};
}

windowsinfo_provider::windowsinfo_provider (error * perr)
{
#if __NOT_WHAT_IS_NEEDED__
    // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-verifyversioninfoa
    // NOTE. Use of the VerifyVersionInfo function to verify the currently running operating system 
    // is not recommended. Instead, use the Version Helper APIs
    OSVERSIONINFOEXA version_info;
    DWORD type_mask = VER_MINORVERSION
        | VER_MAJORVERSION
        | VER_BUILDNUMBER
        | VER_PLATFORMID
        | VER_SERVICEPACKMINOR
        | VER_SERVICEPACKMAJOR
        | VER_SUITENAME
        | VER_PRODUCT_TYPE;
    DWORDLONG cond_mask = 0;
    VER_SET_CONDITION(cond_mask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(cond_mask, VER_MINORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(cond_mask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
    VER_SET_CONDITION(cond_mask, VER_SERVICEPACKMINOR, VER_GREATER_EQUAL);

    ZeroMemory(& version_info, sizeof(OSVERSIONINFOEXA));
    version_info.dwOSVersionInfoSize = sizeof(version_info);
    version_info.dwMajorVersion = 5;
    version_info.dwMinorVersion = 0;
    version_info.wServicePackMajor = 0;
    version_info.wServicePackMinor = 0;

    BOOL success = VerifyVersionInfoA(& version_info, type_mask, cond_mask);

    if (success) {
} else {
        if (GetLastError() != ERROR_OLD_WIN_VERSION) {
            pfs::throw_or(perr, error {pfs::get_last_system_error(), "VerifyVersionInfoA"});
            return;
        }

        // Return 6.2 for Windows 10 
        success = GetVersionExA(reinterpret_cast<LPOSVERSIONINFOA>(& version_info));

        if (!success) {
            pfs::throw_or(perr, error {pfs::get_last_system_error(), "GetVersionExA"});
            return;
        }
    }
#endif // __NOT_WHAT_IS_NEEDED__

    DWORD product_type = 0;

    auto success = GetProductInfo(6, 1, 0, 0, & product_type) != FALSE;

    if (!success) {
        pfs::throw_or(perr, error {pfs::get_last_system_error(), "GetProductInfo"});
        return;
    }

    // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getcomputernamea
    char compname_buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD compname_buffer_size = sizeof(compname_buffer);
    success = GetComputerNameA(compname_buffer, & compname_buffer_size);

    if (success) {
        _os_release.device_name = std::string(compname_buffer, compname_buffer_size);
    } else {
        LOGW(TAG, "GetComputerNameA: {}, error ignored", pfs::get_last_system_error().message());

        auto computer_name_opt = pfs::getenv("COMPUTERNAME");

        if (computer_name_opt) {
            _os_release.device_name = std::move(*computer_name_opt);
        }
    }

    auto version_info = version_info_from_ntdll();

    if (version_info) {
        if (version_info->dwMajorVersion == 11) {
            if (version_info->dwMinorVersion == 0)
                _os_release.version_id = "11";
            else
                _os_release.version_id = fmt::format("11.{}", version_info->dwMinorVersion);
        } else if (version_info->dwMajorVersion == 10) {
            if (version_info->dwMinorVersion == 0)
                _os_release.version_id = "10";
            else
                _os_release.version_id = fmt::format("10.{}", version_info->dwMinorVersion);
        } else if (version_info->dwMajorVersion == 6) {
            if (version_info->dwMinorVersion == 3)
                _os_release.version_id = "8.1";
            else if (version_info->dwMinorVersion == 2)
                _os_release.version_id = "8";
            else if (version_info->dwMinorVersion == 1) // Windows 7 or Windows Server 2008 R2
                _os_release.version_id = "7";
        } else {
            _os_release.version_id = fmt::format("{}.{}", version_info->dwMajorVersion
                , version_info->dwMinorVersion);
        }
        
        _os_release.version = fmt::format("{} Build {}", _os_release.version_id, version_info->dwBuildNumber);

        auto product_type_cstr = stringify_product_type(product_type);

        if (std::strlen(product_type_cstr) == 0) {
            _os_release.pretty_name = fmt::format("Windows {}", _os_release.version);
        } else {
            _os_release.pretty_name = fmt::format("Windows {} {}", stringify_product_type(product_type)
                , _os_release.version);
        }
    }

    _os_release.name    = "Windows";
    _os_release.id      = "windows";
    _os_release.id_like = _os_release.id;

    if (_os_release.pretty_name.empty())
        _os_release.pretty_name = _os_release.name;

    /////////////////////////////////////////////////
    // RAM installed
    /////////////////////////////////////////////////
    ULONGLONG installed_ram_kibs = 0;
    success = GetPhysicallyInstalledSystemMemory(& installed_ram_kibs);

    if (success) {
        _os_release.ram_installed = pfs::numeric_cast<decltype(_os_release.ram_installed)>(installed_ram_kibs) / 1024;
    } else {
        LOGW(TAG, "GetPhysicallyInstalledSystemMemory: {}, error ignored", pfs::get_last_system_error().message());
        _os_release.ram_installed = 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // CPU
    // https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=msvc-170
    ////////////////////////////////////////////////////////////////////////////////////////////////
    auto cpu_info_opt = cpu_info_from_cpuid();

    if (cpu_info_opt) {
        _os_release.cpu_vendor = std::move(cpu_info_opt->vendor);
        _os_release.cpu_brand = std::move(cpu_info_opt->brand);
    }

    if (_os_release.cpu_brand.empty()) {
        //auto cpu_arch_opt = pfs::getenv("PROCESSOR_ARCHITECTURE"); // e.g. AMD64
        //auto cpu_ncores_opt = pfs::getenv("NUMBER_OF_PROCESSORS"); // e.g. 6 
        auto cpu_ident_opt = pfs::getenv("PROCESSOR_IDENTIFIER");  // e.,g. Intel64 Family 6 Model 158 Stepping 12, GenuineIntel

        if (cpu_ident_opt)
            _os_release.cpu_brand = std::move(*cpu_ident_opt);
    }
}

} // namespace metrics

IONIK__NAMESPACE_END
