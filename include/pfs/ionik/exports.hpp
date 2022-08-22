////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// References:
//
// Changelog:
//      2022.08.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef IONIK__STATIC
#   ifndef IONIK__EXPORT
#       if _MSC_VER
#           if defined(IONIK__EXPORTS)
#               define IONIK__EXPORT __declspec(dllexport)
#           else
#               define IONIK__EXPORT __declspec(dllimport)
#           endif
#       else
#           define IONIK__EXPORT
#       endif
#   endif
#else
#   define IONIK__EXPORT
#endif // !IONIK__STATIC
