////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.06.30 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/ionik/error.hpp"
#include <pfs/filesystem.hpp>
#include <chrono>

namespace ionik {
namespace filesystem_monitor {

template <typename Rep>
class monitor
{
    Rep _rep;

public:
    IONIK__EXPORT  monitor (error * perr = nullptr);

    monitor (monitor const &) = delete;
    monitor (monitor &&) = delete;
    monitor & operator = (monitor const &) = delete;
    monitor & operator = (monitor &&) = delete;

    IONIK__EXPORT ~monitor ();

    IONIK__EXPORT bool add (pfs::filesystem::path const & path, error * perr = nullptr);

    template <typename Callbacks>
    IONIK__EXPORT int poll (std::chrono::milliseconds timeout, Callbacks & cb, error * perr = nullptr);
};

}} // namespace ionik::filesystem_monitor
