#include "sapf/ReplRunner.hpp"
#include "sapf/platform/Platform.hpp"

#include <stdio.h>
#include <stdlib.h>

void RunSapfRepl(Thread& th, const char* logFile)
{
    sapf::platform::runReplLoop([&th, logFile]() {
        th.repl(stdin, logFile);
    });
}
