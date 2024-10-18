// Platform specific includes
#pragma once

#ifndef XPLM300
#error This is made to be compiled against the XPLM300 SDK
#endif

#if IBM
    #define WIN32
#elif LIN
    #define LINUX
#elif APL
    #define APPLE
#endif

#if defined(WIN32)
    #include <windows.h>
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
#endif

#if defined(LINUX) || defined(APPLE)
    #define MAX_PATH 1260
    #include <unistd.h>
#endif
