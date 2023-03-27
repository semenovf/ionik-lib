////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022-2023 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.03.27 Initial version
////////////////////////////////////////////////////////////////////////////////
#include "pfs/ionik/error.hpp"
#include "pfs/i18n.hpp"

namespace ionik {

char const * error_category::name () const noexcept
{
    return "ionik::category";
}

std::string error_category::message (int ev) const
{
    switch (ev) {
        case static_cast<int>(errc::success):
            return tr::_("no error");
        case static_cast<int>(errc::acquire_device_observer):
            return tr::_("acquire device observer");
        case static_cast<int>(errc::operation_system_error):
            return tr::_("operation system error");

        default: return tr::_("unknown net error");
    }
}

} // namespace ionik
