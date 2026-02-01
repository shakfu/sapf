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

#if defined(_WIN32)

#include "sapf/platform/Platform.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdlib>

namespace sapf {
namespace platform {

namespace {
    std::mutex gEventLoopMutex;
    std::condition_variable gEventLoopCV;
    std::atomic<bool> gEventLoopRunning{false};
}

PlatformType getCurrentPlatform()
{
    return PlatformType::Windows;
}

const char* getPlatformName()
{
    return "Windows";
}

void runAsync(std::function<void()> task, bool detach)
{
    std::thread t(std::move(task));
    if (detach) {
        t.detach();
    } else {
        t.join();
    }
}

void runEventLoop()
{
    gEventLoopRunning = true;
    std::unique_lock<std::mutex> lock(gEventLoopMutex);
    gEventLoopCV.wait(lock, []{ return !gEventLoopRunning.load(); });
}

void stopEventLoop()
{
    {
        std::lock_guard<std::mutex> lock(gEventLoopMutex);
        gEventLoopRunning = false;
    }
    gEventLoopCV.notify_all();
}

void runReplLoop(std::function<void()> task)
{
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;

    std::thread replThread([&]() {
        task();
        {
            std::lock_guard<std::mutex> lock(mtx);
            done = true;
        }
        cv.notify_one();
    });

    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]{ return done; });
    }

    replThread.join();
    exit(0);
}

} // namespace platform
} // namespace sapf

#endif // _WIN32
