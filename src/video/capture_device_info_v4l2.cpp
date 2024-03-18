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
#include <cstring>
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
    int r = 0;

    do {
        r = ::ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

inline std::string pixel_format2string (std::uint32_t pixfmt)
{
    return fmt::format("{}{}{}{}", char(pixfmt), char(pixfmt >>  8), char(pixfmt >> 16), char(pixfmt >> 24));
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
        auto fd = ::open(fs::utf8_encode(path).c_str(), O_RDWR /*O_RDONLY*/);

        if (fd >= 0) {
            v4l2_capability vcap;

            auto rc = xioctl(fd, VIDIOC_QUERYCAP, & vcap);

            if ( rc >= 0) {
                if ((vcap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                    result.emplace_back();
                    capture_device_info & cdi = result.back();

                    cdi.subsystem       = subsystem_enum::video4linux2;
                    cdi.id              = fs::utf8_encode(path);
                    cdi.readable_name   = reinterpret_cast<char const *>(vcap.card);
                    cdi.data["path"]    = cdi.id;
                    cdi.data["driver"]  = reinterpret_cast<char const *>(vcap.driver);
                    cdi.data["card"]    = reinterpret_cast<char const *>(vcap.card);
                    cdi.data["bus"]     = reinterpret_cast<char const *>(vcap.bus_info);
                    cdi.data["version"] = fmt::format("{}.{}.{}"
                        , (vcap.version >> 16) & 0x0000FFFF
                        , (vcap.version >>  8) & 0x000000FF
                        , vcap.version         & 0x000000FF);

                    //
                    // Get current pixel format and resolution
                    //
                    std::uint32_t current_pixel_format = 0;
                    cdi.current_pixel_format_index = 0;

                    v4l2_format format;
                    std::memset(& format, 0, sizeof(format));
                    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

                    if ((rc = xioctl(fd, VIDIOC_G_FMT, & format)) >= 0) {
                        current_pixel_format = format.fmt.pix.pixelformat;
                        cdi.current_frame_size.width = format.fmt.pix.width;
                        cdi.current_frame_size.height = format.fmt.pix.height;
                    }

                    //
                    // Get supported pixel formats
                    //
                    v4l2_fmtdesc fmtdesc;
                    std::memset(& fmtdesc, 0, sizeof(fmtdesc));
                    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

                    while ((rc = xioctl(fd, VIDIOC_ENUM_FMT, & fmtdesc)) >= 0) {
                        cdi.pixel_formats.emplace_back();
                        pixel_format & pxf = cdi.pixel_formats.back();

                        if (fmtdesc.pixelformat == current_pixel_format)
                            cdi.current_pixel_format_index = fmtdesc.index;

                        pxf.name = pixel_format2string(fmtdesc.pixelformat);
                        pxf.description = reinterpret_cast<char const *>(fmtdesc.description);

                        //
                        // Get frame sizes and frame rates
                        //
                        {
                            v4l2_frmsizeenum frmsizeenum;
                            std::memset(& frmsizeenum, 0, sizeof(frmsizeenum));
                            frmsizeenum.index = 0;
                            frmsizeenum.pixel_format = fmtdesc.pixelformat;

                            while ((rc = xioctl(fd, VIDIOC_ENUM_FRAMESIZES, & frmsizeenum)) >= 0) {
                                switch (frmsizeenum.type) {
                                    case V4L2_FRMSIZE_TYPE_DISCRETE: {
                                        pxf.discrete_frame_sizes.emplace_back();
                                        auto & framesize = pxf.discrete_frame_sizes.back();
                                        framesize.width = frmsizeenum.discrete.width;
                                        framesize.height = frmsizeenum.discrete.height;

                                        //
                                        // Get frame rates
                                        //
                                        {
                                            v4l2_frmivalenum frmivalenum;
                                            std::memset(& frmivalenum, 0, sizeof(frmivalenum));
                                            frmivalenum.index = 0;
                                            frmivalenum.pixel_format = fmtdesc.pixelformat;
                                            frmivalenum.width = framesize.width;
                                            frmivalenum.height = framesize.height;

                                            while ((rc = xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, & frmivalenum)) >= 0) {
                                                if (frmivalenum.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                                                    framesize.frame_rates.push_back(frame_rate {
                                                          frmivalenum.discrete.numerator
                                                        , frmivalenum.discrete.denominator
                                                    });
                                                }

                                                frmivalenum.index++;
                                            }

                                            std::sort(framesize.frame_rates.begin(), framesize.frame_rates.end()
                                                , [] (frame_rate const & a, frame_rate const & b) {
                                                    return (static_cast<double>(a.num) / a.denom)
                                                        > (static_cast<double>(b.num) / b.denom);
                                                });
                                        }

                                        break;
                                    }
                                    case V4L2_FRMSIZE_TYPE_CONTINUOUS:
                                        break;
                                    case V4L2_FRMSIZE_TYPE_STEPWISE:
                                        break;
                                    default:
                                        break;
                                }

                                frmsizeenum.index++;
                            }

                            std::sort(pxf.discrete_frame_sizes.begin(), pxf.discrete_frame_sizes.end()
                                , [] (discrete_frame_size const & a, discrete_frame_size const & b) {
                                    return (static_cast<double>(a.width) * a.height)
                                        < (static_cast<double>(b.width) * b.height);
                                });
                        }

                        fmtdesc.index++;
                    }
                }
            }

            ::close(fd);
        }
    }

    return result;
}

}} // namespace ionik::video
