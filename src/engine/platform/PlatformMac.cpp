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

#if defined(__APPLE__)

#include "sapf/platform/Platform.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <dispatch/dispatch.h>
#include <stdlib.h>

namespace sapf {
namespace platform {

PlatformType getCurrentPlatform()
{
    return PlatformType::MacOS;
}

const char* getPlatformName()
{
    return "macOS";
}

void runAsync(std::function<void()> task, bool detach)
{
    // Copy the task into a block-compatible form
    auto* taskPtr = new std::function<void()>(std::move(task));

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        (*taskPtr)();
        delete taskPtr;
    });
}

void runEventLoop()
{
    CFRunLoopRun();
}

void stopEventLoop()
{
    CFRunLoopStop(CFRunLoopGetMain());
}

void runReplLoop(std::function<void()> task)
{
    // Copy the task for block capture
    auto* taskPtr = new std::function<void()>(std::move(task));

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        (*taskPtr)();
        delete taskPtr;
        exit(0);
    });

    CFRunLoopRun();
}

} // namespace platform
} // namespace sapf

#endif // __APPLE__
