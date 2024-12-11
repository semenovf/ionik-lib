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
#include "ionik/metrics/freedesktop_provider.hpp"
#include <pfs/filesystem.hpp>
#include <pfs/i18n.hpp>
#include <pfs/string_view.hpp>
// #include "pfs/numeric_cast.hpp"

IONIK__NAMESPACE_BEGIN

namespace metrics {

namespace fs = pfs::filesystem;
using string_view = pfs::string_view;

// Example:
//--------------------------------------------------------------------------------------------------
// PRETTY_NAME="Ubuntu 22.04.5 LTS"
// NAME="Ubuntu"
// VERSION_ID="22.04"
// VERSION="22.04.5 LTS (Jammy Jellyfish)"
// VERSION_CODENAME=jammy
// ID=ubuntu
// ID_LIKE=debian
// HOME_URL="https://www.ubuntu.com/"
// SUPPORT_URL="https://help.ubuntu.com/"
// BUG_REPORT_URL="https://bugs.launchpad.net/ubuntu/"
// PRIVACY_POLICY_URL="https://www.ubuntu.com/legal/terms-and-policies/privacy-policy"
// UBUNTU_CODENAME=jammy
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
        if (rec.key == "PRETTY_NAME")
            osi.PRETTY_NAME = to_string(rec.value);
        else if (rec.key == "NAME")
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

//
// https://www.freedesktop.org/software/systemd/man/latest/os-release.html
//
freedesktop_provider::freedesktop_provider (error * perr)
{
    os_release_info osi;
    auto err = parse_os_release(osi);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return;
    }

    _os_name = osi.NAME;
    _os_pretty_name = osi.PRETTY_NAME;
}

} // namespace metrics

IONIK__NAMESPACE_END
