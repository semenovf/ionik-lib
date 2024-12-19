////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "parser.hpp"
#include "ionik/error.hpp"
#include "ionik/local_file.hpp"
#include "ionik/metrics/linuxinfo_provider.hpp"
#include <pfs/filesystem.hpp>
#include <pfs/i18n.hpp>
#include <pfs/optional.hpp>
#include <pfs/string_view.hpp>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <cpuid.h>
#include <algorithm>

IONIK__NAMESPACE_BEGIN

namespace metrics {

namespace fs = pfs::filesystem;
using string_view = pfs::string_view;

//
// https://www.freedesktop.org/software/systemd/man/latest/os-release.html
//
// Example:
//--------------------------------------------------------------------------------------------------
// PRETTY_NAME="Ubuntu 22.04.5 LTS"
// NAME="Ubuntu"
// VERSION_ID="22.04"
// VERSION="22.04.5 LTS (Jammy Jellyfish)"
// VERSION_CODENAME=jammy
// ID=ubuntu
// ID_LIKE=debian
// ...
//--------------------------------------------------------------------------------------------------
struct os_release_info
{
    // Excluded URLs

    // A pretty operating system name in a format suitable for presentation to the user.
    // May or may not contain a release code name or OS version of some kind, as suitable.
    // If not set, a default of "PRETTY_NAME="Linux"" may be used
    std::string PRETTY_NAME;

    // A string identifying the operating system, without a version component, and suitable for
    // presentation to the user
    std::string NAME;

    // A string identifying the operating system version, excluding any OS name information,
    // possibly including a release code name, and suitable for presentation to the user.
    // This field is optional.
    std::string VERSION;

    // A lower-case string (mostly numeric, no spaces or other characters outside
    // of 0–9, a–z, ".", "_" and "-") identifying the operating system version, excluding any OS
    // name information or release code name, and suitable for processing by scripts or usage in
    // generated filenames. This field is optional.
    std::string VERSION_ID;        //

    // A lower-case string (no spaces or other characters outside of 0–9, a–z, ".", "_" and "-")
    // identifying the operating system release code name, excluding any OS name information or
    // release version, and suitable for processing by scripts or usage in generated filenames.
    // This field is optional and may not be implemented on all systems.
    std::string VERSION_CODENAME;

    // A lower-case string (no spaces or other characters outside of 0–9, a–z, ".", "_" and "-")
    // identifying the operating system, excluding any version information and suitable for
    // processing by scripts or usage in generated filenames. If not set, a default of "ID=linux"
    // may be used. Note that even though this string may not include characters that require
    // shell quoting, quoting may nevertheless be used.
    std::string ID;

    // A space-separated list of operating system identifiers in the same syntax as the ID= setting.
    // It should list identifiers of operating systems that are closely related to the local
    // operating system in regards to packaging and programming interfaces, for example listing
    // one or more OS identifiers the local OS is a derivative from. An OS should generally only
    // list other OS identifiers it itself is a derivative of, and not any OSes that are derived
    // from it, though symmetric relationships are possible. Build scripts and similar should check
    // this variable if they need to identify the local operating system and the value of ID= is
    // not recognized. Operating systems should be listed in order of how closely the local
    // operating system relates to the listed ones, starting with the closest.
    // This field is optional.
    std::string ID_LIKE;
};

struct os_release_record
{
    string_view key;
    string_view value;
};

static bool parse_record (string_view::const_iterator & pos, string_view::const_iterator last
    , os_release_record & rec, error * perr)
{
    auto p = pos;

    if (p == last)
        return false;

    skip_ws(p, last);

    if (p == last)
        return false;

    rec.key = string_view{};
    rec.value = string_view{};

    auto success = advance_key(p, last, rec.key)
        && advance_assign(p, last)
        && advance_ws0n(p, last)
        && advance_unparsed_value(p, last, rec.value)
        && advance_nl_or_endp(p, last);

    if (!(success && !rec.key.empty())) {
        pfs::throw_or(perr, error {
              pfs::errc::unexpected_data
            , tr::_("unexpected 'os_release' record format")
        });

        return false;
    }

    // Remove double qoutes
    if (!rec.value.empty()) {
        if (rec.value.front() == '"')
            rec.value.remove_prefix(1);

        if (rec.value.back() == '"')
            rec.value.remove_suffix(1);
    }

    pos = p;
    return true;
}

static error parse_os_release (os_release_info & osi)
{
    fs::path os_release_path;

    for (auto const & p: {PFS__LITERAL_PATH("/etc/os-release"), PFS__LITERAL_PATH("/usr/lib/os-release")}) {
        if (fs::exists(p)) {
            os_release_path = p;
            break;
        }
    }

    if (os_release_path.empty()) {
        return error {
              std::make_error_code(std::errc::no_such_file_or_directory)
            , tr::_("'os-release'")
        };
    }

    error err;
    auto os_release_file = local_file::open_read_only(os_release_path, & err);

    if (!os_release_file)
        return err;

    auto os_release_content = os_release_file.read_all(& err);

    if (err)
        return err;

    string_view content_view {os_release_content};
    auto pos = content_view.cbegin();
    auto last = content_view.cend();

    os_release_record rec;

    while (parse_record(pos, last, rec, & err)) {
        if (rec.key == "PRETTY_NAME") {
            osi.PRETTY_NAME = to_string(rec.value);
            std::replace(osi.PRETTY_NAME.begin(), osi.PRETTY_NAME.end(), '_', ' ');
        } else if (rec.key == "NAME")
            osi.NAME = to_string(rec.value);
        else if (rec.key == "VERSION_ID")
            osi.VERSION_ID = to_string(rec.value);
        else if (rec.key == "VERSION")
            osi.VERSION = to_string(rec.value);
        else if (rec.key == "VERSION_CODENAME")
            osi.VERSION_CODENAME = to_string(rec.value);
        else if (rec.key == "ID")
            osi.ID = to_string(rec.value);
        else if (rec.key == "ID_LIKE")
            osi.ID_LIKE = to_string(rec.value);
    }

    if (!err) {
        if (osi.PRETTY_NAME.empty())
            osi.PRETTY_NAME = "Linux";

        if (osi.ID.empty())
            osi.ID = "linux";

        if (osi.ID_LIKE.empty())
            osi.ID_LIKE = osi.ID;
    }

    return err;
}


struct cpu_info
{
    std::string vendor;
    std::string brand;
};

static pfs::optional<cpu_info> cpu_info_from_cpuid ()
{
    std::array<int, 4> cpui;
    unsigned int eax, ebx, ecx, edx;
    std::vector<std::array<int, 4>> data;
    std::vector<std::array<int, 4>> extdata;

    __cpuid(0, cpui[0], cpui[1], cpui[2], cpui[3]);
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

    // Calling __cpuid with 0x80000000 as the function_id argument
    // gets the number of the highest valid extended ID.
    __cpuid(0x80000000, cpui[0], cpui[1], cpui[2], cpui[3]);
    n = cpui[0];

    char brand[0x40];
    char * pbrand = brand;
    std::memset(brand, 0, sizeof(brand));

    for (int i = 0x80000000; i <= n; ++i) {
        __cpuidex(cpui.data(), i, 0);
        extdata.push_back(cpui);
    }

    // Interpret CPU brand string if reported
    if (n >= 0x80000004) {
        std::memcpy(brand, extdata[2].data(), 16);
        std::memcpy(brand + 16, extdata[3].data(), 16);
        std::memcpy(brand + 32, extdata[4].data(), 16);

        // Trim prefix spaces
        while (*pbrand == ' ')
            pbrand++;
    }

    return cpu_info {vendor, pbrand};
}

linuxinfo_provider::linuxinfo_provider (error * perr)
{
    os_release_info osi;
    auto err = parse_os_release(osi);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return;
    }

    _os_info.name        = std::move(osi.NAME);
    _os_info.pretty_name = std::move(osi.PRETTY_NAME);
    _os_info.version     = std::move(osi.VERSION);
    _os_info.version_id  = std::move(osi.VERSION_ID);
    _os_info.codename    = std::move(osi.VERSION_CODENAME);
    _os_info.id          = std::move(osi.ID);
    _os_info.id_like     = std::move(osi.ID_LIKE);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Device info
    ////////////////////////////////////////////////////////////////////////////////////////////////
    std::array<char, 16> hostname;
    auto rc = gethostname(hostname.data(), hostname.size());

    if (rc != 0) {
        pfs::throw_or(perr, error {pfs::get_last_system_error(), tr::_("gethostname")});
        return;
    }

    _os_info.device_name = hostname.data();

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // RAM installed
    ////////////////////////////////////////////////////////////////////////////////////////////////
    struct sysinfo si;

    rc = sysinfo(& si);

    if (rc != 0) {
        pfs::throw_or(perr, error {pfs::get_last_system_error(), tr::_("sysinfo")});
        return;
    }

    _os_info.ram_installed = (static_cast<double>(si.totalram) * si.mem_unit) / 1024 / 1024;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // CPU
    ////////////////////////////////////////////////////////////////////////////////////////////////
    auto cpu_info_opt = cpu_info_from_cpuid();

    if (cpu_info_opt) {
        _os_info.cpu_vendor = std::move(cpu_info_opt->vendor);
        _os_info.cpu_brand = std::move(cpu_info_opt->brand);
    }

    if (_os_info.cpu_brand.empty()) {
        // //auto cpu_arch_opt = pfs::getenv("PROCESSOR_ARCHITECTURE"); // e.g. AMD64
        // //auto cpu_ncores_opt = pfs::getenv("NUMBER_OF_PROCESSORS"); // e.g. 6
        // auto cpu_ident_opt = pfs::getenv("PROCESSOR_IDENTIFIER");  // e.,g. Intel64 Family 6 Model 158 Stepping 12, GenuineIntel
        //
        // if (cpu_ident_opt)
        //     _os_info.cpu_brand = std::move(*cpu_ident_opt);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Kernel
    // See man uname.2
    ////////////////////////////////////////////////////////////////////////////////////////////////
    struct utsname un;

    rc = uname(& un);

    if (rc != 0) {
        pfs::throw_or(perr, error {pfs::get_last_system_error(), tr::_("uname")});
        return;
    }

    _os_info.sysname = un.sysname;
    _os_info.kernel_release = un.release;
    _os_info.machine = un.machine;
}

} // namespace metrics

IONIK__NAMESPACE_END
