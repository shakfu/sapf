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

#ifndef SAPF_PLATFORM_HPP
#define SAPF_PLATFORM_HPP

#include <functional>

/**
 * Platform abstraction layer for SAPF.
 *
 * This header provides platform-independent interfaces for:
 * - Background task execution
 * - Run loop management
 * - Platform identification
 *
 * Platform-specific implementations are in:
 * - src/engine/platform/PlatformMac.cpp (macOS)
 * - src/engine/platform/PlatformUnix.cpp (Linux and other Unix)
 * - src/engine/platform/PlatformWindows.cpp (Windows)
 */

namespace sapf {
namespace platform {

/**
 * Platform identification
 */
enum class PlatformType {
    MacOS,
    Linux,
    Windows,
    Unknown
};

/**
 * Get the current platform type.
 */
PlatformType getCurrentPlatform();

/**
 * Get a human-readable name for the current platform.
 */
const char* getPlatformName();

/**
 * Execute a task asynchronously in a background thread/queue.
 *
 * On macOS: Uses GCD (Grand Central Dispatch)
 * On Linux/others: Uses std::thread
 *
 * @param task The function to execute
 * @param detach If true, the task runs independently (fire-and-forget)
 *               If false, the caller may need to manage the task lifetime
 */
void runAsync(std::function<void()> task, bool detach = true);

/**
 * Run a blocking event loop.
 *
 * On macOS: Runs CFRunLoopRun()
 * On Linux/others: Blocks on a condition variable until done is set
 *
 * This function blocks until the run loop is stopped.
 */
void runEventLoop();

/**
 * Stop the current event loop.
 *
 * On macOS: Stops the CFRunLoop
 * On Linux/others: Signals the condition variable
 */
void stopEventLoop();

/**
 * Run a REPL-style event loop with a task.
 *
 * Executes the given task in a background thread/queue while maintaining
 * an event loop on the main thread. When the task completes, the event
 * loop is stopped.
 *
 * @param task The REPL task to execute
 */
void runReplLoop(std::function<void()> task);

} // namespace platform
} // namespace sapf

#endif // SAPF_PLATFORM_HPP
