////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file was part of `multimedia-lib` until 2023.03.31.
// This file is part of `ionik-lib`.
//
// Changelog:
//      2021.08.03 Initial version (multimedia-lib).
//      2023.03.31 Initial version (ionik-lib).
////////////////////////////////////////////////////////////////////////////////
#include "pfs/ionik/audio/device.hpp"
#include <pfs/fmt.hpp>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <string>

using namespace ionik;

int main (int, char *[])
{
    if (!audio::supported()) {
        fmt::println("Attention!!! This library is compiled without support for audio devices.");
        return EXIT_SUCCESS;
    }

    auto default_input_device = audio::default_input_device();
    auto default_output_device = audio::default_output_device();

    if (!default_input_device.name.empty()) {
        fmt::println("Default input device:\n"
                "\tname={}\n"
                "\treadable name={}"
            , default_input_device.name, default_input_device.readable_name);
    } else {
        fmt::println("Default input device: none");
    }

    if (!default_output_device.name.empty()) {
        fmt::println("Default output device:\n"
            "\tname={}\n"
            "\treadable name={}"
            , default_output_device.name, default_output_device.readable_name);
    } else {
        fmt::println("Default output device: none");
    }

    std::string indent{"     "};
    std::string   mark{"  (*)"};
    std::string * prefix = & indent;

    int counter = 0;
    auto input_devices = audio::fetch_devices(audio::device_mode::input);

    if (input_devices.empty())
        fmt::println("Input devices: none");
    else
        fmt::println("Input devices:");

    for (auto const & d: input_devices) {
        prefix = (d.name == default_input_device.name)
            ? & mark : & indent;
        std::cout << *prefix << std::setw(2) << ++counter << ". " << d.readable_name << "\n";
        std::cout << indent << "    name: " << d.name << "\n";
    }

    auto output_devices = audio::fetch_devices(audio::device_mode::output);
    counter = 0;

    if (output_devices.empty())
        fmt::println("Output devices: none");
    else
        fmt::println("Output devices:");

    for (auto const & d: output_devices) {
        prefix = (d.name == default_output_device.name)
            ? & mark : & indent;
        std::cout << *prefix << std::setw(2) << ++counter << ". " << d.readable_name << "\n";
        std::cout << indent << "    name: " << d.name << "\n";
    }

    return EXIT_SUCCESS;
}
