////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.10.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/ionik/local_file.hpp"
#include <pfs/standard_paths.hpp>
#include <pfs/universal_id.hpp>
#include <algorithm>
#include <numeric>

namespace fs = pfs::filesystem;

static fs::path unique_temp_file_path ()
{
    fs::path result;
    int counter = 100;

    while (result.empty() && counter-- > 0) {
        result = fs::standard_paths::temp_folder()
            / fs::utf8_decode(to_string(pfs::generate_uuid()) + ".ionik");

        if (fs::exists(result))
            result.clear();
    }

    if (result.empty())
        throw std::runtime_error("unable to generate unique file");

    return result;
}

TEST_CASE("local_file") {
    auto test_file_path = unique_temp_file_path();
    MESSAGE("Test file path: ", pfs::filesystem::utf8_encode(test_file_path));

    // === Create file and write test data
    auto test_file = ionik::local_file::open_write_only(test_file_path);

    REQUIRE_EQ(test_file, true);

    std::vector<char> binary_data(256);
    std::iota(binary_data.begin(), binary_data.end(), 0);
    auto res = test_file.write(binary_data.data(), binary_data.size());
    test_file.close();

    REQUIRE_EQ(res.first, binary_data.size());

    // === Read all
    test_file = ionik::local_file::open_read_only(test_file_path);

    REQUIRE_EQ(test_file, true);

    auto test_file_content = test_file.read_all();
    REQUIRE_EQ(test_file_content.size(), binary_data.size());
    REQUIRE(std::equal(test_file_content.cbegin(), test_file_content.cend()
        , binary_data.cbegin()));
    REQUIRE_EQ(test_file.offset().first, test_file_content.size());

    REQUIRE(test_file.set_pos(0));
    REQUIRE_EQ(test_file.offset().first, 0);

    REQUIRE_THROWS(test_file.set_pos(test_file_content.size() + 1));

    test_file.close();

    // === Read
    test_file = ionik::local_file::open_read_only(test_file_path);
    REQUIRE_EQ(test_file, true);

    char ch;
    REQUIRE_EQ(test_file.read(ch).first, 1);
    REQUIRE_EQ(ch, '\x0');
    REQUIRE_EQ(test_file.read(ch).first, 1);
    REQUIRE_EQ(ch, '\x1');

    REQUIRE_EQ(test_file.set_pos(127), true);
    REQUIRE_EQ(test_file.read(ch).first, 1);
    REQUIRE_EQ(ch, '\x7f');

    test_file.close();

    fs::remove(test_file_path);
}

TEST_CASE("initial size") {
    auto test_file_path = unique_temp_file_path();
    MESSAGE("Test file path: ", pfs::filesystem::utf8_encode(test_file_path));

    ionik::local_file::filesize_type initial_size = 10UL * 1024 * 1024 * 1024; // 10 Mib

    auto test_file = ionik::local_file::open_write_only(test_file_path, ionik::truncate_enum::on
        , initial_size);

    test_file.close();

    CHECK_EQ(fs::file_size(test_file_path), initial_size);
    fs::remove(test_file_path);
}
