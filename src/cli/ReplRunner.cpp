#include "sapf/ReplRunner.hpp"

#include <stdio.h>
#include <stdlib.h>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <dispatch/dispatch.h>

void RunSapfRepl(Thread& th, const char* logFile)
{
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		th.repl(stdin, logFile);
		exit(0);
	});

	CFRunLoopRun();
}

#else
#include <thread>
#include <condition_variable>
#include <mutex>

void RunSapfRepl(Thread& th, const char* logFile)
{
	std::mutex mtx;
	std::condition_variable cv;
	bool done = false;

	std::thread replThread([&]() {
		th.repl(stdin, logFile);
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

#endif
