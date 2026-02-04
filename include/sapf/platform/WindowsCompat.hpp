//    SAPF - Sound As Pure Form
//    Copyright (C) 2019 James McCartney
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef SAPF_WINDOWS_COMPAT_HPP
#define SAPF_WINDOWS_COMPAT_HPP

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cstddef>
#include <cstdlib>
#include <ctype.h>

// ssize_t is a POSIX type, use ptrdiff_t on Windows
#ifndef ssize_t
typedef ptrdiff_t ssize_t;
#endif

// isascii and toascii are non-standard, MSVC provides __isascii and __toascii
// Undefine any existing macros and provide inline functions
#ifdef isascii
#undef isascii
#endif
#ifdef toascii
#undef toascii
#endif
inline int isascii(int c) { return __isascii(c); }
inline int toascii(int c) { return __toascii(c); }

// random() is BSD, use rand() on Windows
inline long random() { return rand(); }

// usleep is POSIX, implement using Windows Sleep
#include <windows.h>
typedef unsigned int useconds_t;
inline int usleep(useconds_t usec) {
    Sleep(usec / 1000);
    return 0;
}

// localtime_r is POSIX, use localtime_s on Windows (note: args are swapped)
#include <ctime>
inline struct tm* localtime_r(const time_t* timep, struct tm* result) {
    return localtime_s(result, timep) == 0 ? result : nullptr;
}

// ctime_r is POSIX, use ctime_s on Windows
inline char* ctime_r(const time_t* timep, char* buf) {
    return ctime_s(buf, 26, timep) == 0 ? buf : nullptr;
}

// gettimeofday is POSIX
#include <winsock2.h>
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};
inline int gettimeofday(struct timeval* tp, struct timezone* tzp) {
    (void)tzp;
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    // Convert from 100-nanosecond intervals since 1601 to microseconds since 1970
    // 11644473600 seconds between 1601 and 1970
    uli.QuadPart -= 116444736000000000ULL;
    uli.QuadPart /= 10;  // Convert to microseconds
    if (tp) {
        tp->tv_sec = (long)(uli.QuadPart / 1000000ULL);
        tp->tv_usec = (long)(uli.QuadPart % 1000000ULL);
    }
    return 0;
}

#endif // _WIN32

#endif // SAPF_WINDOWS_COMPAT_HPP
