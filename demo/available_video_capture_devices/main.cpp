////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.03.06 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/fmt.hpp"
#include "pfs/ionik/video/capture_device.hpp"

int main ()
{
    auto capture_devices = ionik::video::fetch_capture_devices();

    for (auto const & dev: capture_devices) {
        fmt::println("Name: {}", dev.readable_name);

        for (auto const & item : dev.data)
            fmt::println("\t{}: {}", item.first, item.second);

        fmt::println("\tPixel formats:");

        for (std::size_t index = 0; index < dev.pixel_formats.size(); index++) {
            auto & pxf = dev.pixel_formats[index];

            fmt::println("\t\t{}. {} '{}' ({})", index + 1
                , (index == dev.current_pixel_format_index ? "(*)" : "   ")
                , pxf.name, pxf.description);

            fmt::println("\t\t\tFrame sizes:");

            for (auto const & frame_size: pxf.discrete_frame_sizes) {
                fmt::print("\t\t\t\t{}{}x{}, frame rates:"
                    , index == dev.current_pixel_format_index
                            && frame_size.width == dev.current_frame_size.width
                            && frame_size.height == dev.current_frame_size.height
                        ? "(*) " : "    "
                    , frame_size.width, frame_size.height);

                for (auto const & frame_rate: frame_size.frame_rates) {
                    fmt::print(" {}/{}", frame_rate.num, frame_rate.denom);
                }

                fmt::print("\n");
            }
        }
    }

    return EXIT_SUCCESS;
}
