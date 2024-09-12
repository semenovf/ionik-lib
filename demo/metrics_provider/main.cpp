////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "proc_provider.hpp"
#include "sysinfo_provider.hpp"
#include <pfs/i18n.hpp>
#include <pfs/log.hpp>
#include <atomic>
#include <cstdlib>
#include <thread>
#include <signal.h>

static std::atomic_bool TERM_APP {false};

static void sigterm_handler (int /*sig*/)
{
    TERM_APP = true;
}

class proc_stat_provider
{

};

int main (int argc, char * argv[])
{
    // if (argc < 2) {
    //     fmt::println(stderr, "ERROR: Too few arguments");
    //     return EXIT_FAILURE;
    // }
    //
    // signal(SIGINT, sigterm_handler);
    // signal(SIGTERM, sigterm_handler);
    //
    // auto path = pfs::filesystem::utf8_decode(argv[1]);
    // ionik::filesystem_monitor::functional_callbacks callbacks;
    //
    // callbacks.accessed = [] (fs::path const & p) {
    //     fmt::println("-- ACCESSED: {}", p);
    // };
    //
    // callbacks.modified = [] (fs::path const & p) {
    //     fmt::println("-- MODIFIED: {}", p);
    // };
    //
    // callbacks.metadata_changed = [] (fs::path const & p) {
    //     fmt::println("-- METADATA: {}", p);
    // };
    //
    // callbacks.opened = [] (fs::path const & p) {
    //     fmt::println("-- OPENED: {}", p);
    // };
    //
    // callbacks.closed = [] (fs::path const & p) {
    //     fmt::println("-- CLOSED: {}", p);
    // };
    //
    // callbacks.created = [] (fs::path const & p) {
    //     fmt::println("-- CREATED: {}", p);
    // };
    //
    // callbacks.deleted = [] (fs::path const & p) {
    //     fmt::println("-- DELETED: {}", p);
    // };
    //
    // callbacks.moved = [] (fs::path const & p) {
    //     fmt::println("-- MOVED: {}", p);
    // };
    //
    // filesystem_monitor mon;
    //
    // fmt::println("Watching: {}", path);
    //
    // try {
    //     mon.add(path);
    // } catch (pfs::error const & ex) {
    //     LOGE("", "Exception: {}", ex.what());
    //     return EXIT_FAILURE;
    // }

    std::chrono::seconds query_interval{1};
    ionik::metrics::proc_meminfo_provider pmp;
    ionik::metrics::sysinfo_provider sp;

    while (!TERM_APP && sp.query() && pmp.query()) {
        std::this_thread::sleep_for(query_interval);
    }

    fmt::println("Finishing application");

    return EXIT_SUCCESS;
}
