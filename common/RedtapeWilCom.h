// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#ifdef _WIN32

#include "common/RedtapeWindows.h"

// warning : variable 's_hrErrorLast' set but not used [-Wunused-but-set-variable]
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#endif

#ifndef __is_convertible_to
#define __is_convertible_to(from, to) std::is_convertible<from, to>::value
#endif

#include <wil/com.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif
