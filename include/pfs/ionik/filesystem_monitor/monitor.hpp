////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.06.30 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "error.hpp"
#include <pfs/filesystem.hpp>
#include <chrono>

namespace ionik {
namespace filesystem_monitor {

template <typename Rep>
class monitor
{
    Rep _rep;

public:
    monitor (error * perr = nullptr);
    monitor (monitor const &) = delete;
    monitor (monitor &&) = delete;
    monitor & operator = (monitor const &) = delete;
    monitor & operator = (monitor &&) = delete;

    ~monitor ();

    bool add (pfs::filesystem::path const & path, error * perr = nullptr);

    template <typename Callbacks>
    int poll (std::chrono::milliseconds timeout, Callbacks & cb, error * perr = nullptr);
};

}} // namespace ionik::filesystem_monitor
