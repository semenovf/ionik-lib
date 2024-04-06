////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.04.06 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "video/capture_device.hpp"
//#include "pfs/filesystem.hpp"
//#include "pfs/fmt.hpp"
//#include "pfs/scan_directory.hpp"
//#include "pfs/string_view.hpp"
//#include <algorithm>
//#include <cerrno>
//#include <cstring>
//#include <string>
//#include <vector>
//#include <fcntl.h>
//#include <unistd.h>
//#include <sys/ioctl.h>
//#include <linux/videodev2.h>

//namespace fs = pfs::filesystem;

namespace ionik {
namespace video {

std::vector<capture_device_info> fetch_capture_devices (error * /*perr*/)
{
    std::vector<capture_device_info> result;
    return result;
}

}} // namespace ionik::video
