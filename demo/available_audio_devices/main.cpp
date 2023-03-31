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
#include "pfs/fmt.hpp"
#include "pfs/ionik/audio/device.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <cerrno>

using namespace ionik;

int main (int argc, char * argv[])
{
    auto default_input_device = audio::default_input_device();
    auto default_output_device = audio::default_output_device();

    if (!default_input_device.name.empty()) {
        fmt::print("Default input device:\n"
                "\tname={}\n"
                "\treadable name={}\n"
            , default_input_device.name, default_input_device.readable_name);
    } else {
        fmt::print("Default input device: none\n");
    }

    if (!default_output_device.name.empty()) {
        fmt::print("Default output device:\n"
		        "\tname={}\n"
		        "\treadable name={}\n"
            , default_output_device.name, default_output_device.readable_name);
    } else {
        fmt::print("Default output device: none\n");
    }

    std::string indent{"     "};
    std::string   mark{"  (*)"};
    std::string * prefix = & indent;

    int counter = 0;
    auto input_devices = audio::fetch_devices(audio::device_mode::input);

    std::cout << "Input devices:\n";

    for (auto const & d: input_devices) {
        prefix = (d.name == default_input_device.name)
            ? & mark : & indent;
        std::cout << *prefix << std::setw(2) << ++counter << ". " << d.readable_name << "\n";
        std::cout << indent << "    name: " << d.name << "\n";
    }

    auto output_devices = audio::fetch_devices(audio::device_mode::output);
    counter = 0;

    std::cout << "Output devices:\n";

    for (auto const & d: output_devices) {
        prefix = (d.name == default_output_device.name)
            ? & mark : & indent;
        std::cout << *prefix << std::setw(2) << ++counter << ". " << d.readable_name << "\n";
        std::cout << indent << "    name: " << d.name << "\n";
    }

    return EXIT_SUCCESS;
}
