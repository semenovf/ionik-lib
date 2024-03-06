////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.03.06 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "video/capture_device.hpp"
#include "pfs/filesystem.hpp"
#include "pfs/fmt.hpp"
#include "pfs/scan_directory.hpp"
#include "pfs/string_view.hpp"
#include <algorithm>
#include <cerrno>
#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

namespace fs = pfs::filesystem;

namespace ionik {
namespace video {

static int xioctl (int fh, int request, void * arg)
{
    int r;

    do {
        r = ::ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

std::vector<capture_device_info> fetch_capture_devices (error * /*perr*/)
{
    std::vector<capture_device_info> result;
    std::vector<fs::path> files;
    fs::directory_scanner ds;

    auto deviceDir = fs::utf8_decode("/dev");

    ds.on_entry = [& files, deviceDir] (pfs::filesystem::path const & path) {
        if (fs::is_character_file(path)) {
            auto filename = path.filename();

            if (pfs::starts_with(fs::utf8_encode(filename), "video")) {
                fs::path target = path;

                while (fs::is_symlink(target)) {
                    target = fs::read_symlink(path);
                    target = fs::canonical(deviceDir / target);
                }

                if (std::find(files.begin(), files.end(), target) == files.end())
                    files.push_back(target);
            }
        }
    };

    ds.scan(deviceDir);

    for (auto const & path: files) {
        auto fd = ::open(fs::utf8_encode(path).c_str(), O_RDONLY);

        if (fd >= 0) {
            v4l2_capability vcap;

            if (xioctl(fd, VIDIOC_QUERYCAP, & vcap) >= 0) {
                if ((vcap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                    result.emplace_back();
                    capture_device_info & cdi = result.back();

                    cdi.subsystem      = subsystem_enum::video4linux2;
                    cdi.readable_name  = reinterpret_cast<char const *>(vcap.card);
                    cdi.data["path"]   = fs::utf8_encode(path);
                    cdi.data["driver"] = reinterpret_cast<char const *>(vcap.driver);
                    cdi.data["card"]   = reinterpret_cast<char const *>(vcap.card);
                    cdi.data["bus"]    = reinterpret_cast<char const *>(vcap.bus_info);
                    cdi.data["version"] = std::to_string(vcap.version);
                }
            }

            ::close(fd);
        }
    }

    return result;
}

}} // namespace ionik::video
