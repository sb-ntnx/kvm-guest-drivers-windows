#pragma once

/*
*
*
* Copyright (c) 2017  Red Hat, Inc.
*
* Authors:
*  Sameeh Jubran  <sjubran@redhat.com>
*
*/

#include "kdebugprint.h"

// Undefine for disabling WPP tracing
#define NETKVM_WPP_ENABLED

#ifndef NETKVM_WPP_ENABLED
#define TraceNoPrefix DPrintf
#endif

#define POLL_PRINT_LEVEL 3

//
// Define the tracing flags.
//
// Tracing GUID - {5666D67E - 281E-43ED - 8B8D - 4347080198AA}
//

#define WPP_CONTROL_GUIDS                                                  \
    WPP_DEFINE_CONTROL_GUID(                                               \
    NetKVM,                                                                \
    (5666D67E,281E,43ED,8B8D,4347080198AA),                                \
        WPP_DEFINE_BIT(TRACE_DRIVER)             /* bit  0 = 0x00000000 */ \
    )

// our level 0 has the same meaning as TRACE_LEVEL_INFORMATION
#define WPP_LEVEL_COMPARE(level) \
    (WPP_CONTROL(WPP_BIT_ ## TRACE_DRIVER).Level >= (level + TRACE_LEVEL_INFORMATION))

#define WPP_Flags_LEVEL_LOGGER(Flags, level)                                  \
    WPP_LEVEL_LOGGER(Flags)
#define WPP_Flags_LEVEL_ENABLED(Flags, level)                                 \
    (level <= virtioDebugLevel || (WPP_LEVEL_COMPARE(level)))

#define WPP_Flags_LEVEL_STATUS_LOGGER(Flags, level, status)                   \
    WPP_LEVEL_LOGGER(Flags)
#define WPP_Flags_LEVEL_STATUS_ENABLED(Flags, level, status)                  \
    (level <= virtioDebugLevel || WPP_LEVEL_COMPARE(level))


//
// This comment block is scanned by the trace preprocessor to convert the DPrintf,
// DEBUG_ENTRY and DEBUG_EXIT_STATUS MACROS.
//
// begin_wpp config
// USEPREFIX (DPrintf, "%!STDPREFIX![%!FUNC!] ");
// FUNC DPrintf{Flags=TRACE_DRIVER}(LEVEL, MSG, ...);
//
// USEPREFIX (DEBUG_ENTRY, "%!STDPREFIX![%!FUNC!]=>");
// FUNC DEBUG_ENTRY{Flags=TRACE_DRIVER}(LEVEL, ...);
//
// USEPREFIX (DEBUG_EXIT_STATUS, "%!STDPREFIX![%!FUNC!]<= 0x%x", STATUS);
// FUNC DEBUG_EXIT_STATUS{Flags=TRACE_DRIVER}(LEVEL, STATUS);
//
// FUNC TraceNoPrefix{Flags=TRACE_DRIVER}(LEVEL, MSG, ...);
// end_wpp
//
