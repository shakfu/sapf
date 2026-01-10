#include "sapf/Engine.hpp"
#include "sapf/ReplRunner.hpp"
#include "VM.hpp"
#include "CLI11.hpp"
#include <cstdio>

int main(int argc, const char * argv[])
{
	SapfEngineConfig config;
	std::string inputFile;
	std::string preludeFile;
	bool startManta = false;
	bool interactive = false;
	bool quiet = false;

	CLI::App app{"sapf - A tool for the expression of sound as pure form"};
	app.add_option("-r,--rate", config.sampleRate, "Sample rate (1000-768000)")
		->check(CLI::Range(1000.0, 768000.0));
	app.add_option("-p,--prelude", preludeFile, "Prelude file to load");
	app.add_flag("-m,--manta", startManta, "Start Manta event loop");
	app.add_flag("-i,--interactive", interactive, "Interactive mode (enter REPL after running file)");
	app.add_flag("-q,--quiet", quiet, "Quiet mode (suppress banner)");
	app.add_option("file", inputFile, "Input file to load and execute");

	CLI11_PARSE(app, argc, argv);

	if (!preludeFile.empty()) {
		config.preludeFile = preludeFile.c_str();
	}

	if (!quiet) {
		post("------------------------------------------------\n");
		post("A tool for the expression of sound as pure form.\n");
		post("------------------------------------------------\n");
		post("--- version %s\n", SapfGetVersionString());
	}

	SapfEngine& engine = GetSapfEngine();
	engine.configure(config);
	engine.initialize();
	if (startManta) {
		engine.startMantaEventLoop();
	}

	Thread th;
	engine.loadPrelude(th);

	if (!inputFile.empty()) {
		loadFile(th, inputFile.c_str());

		if (!interactive) {
			return 0;
		}
	}

	// Enter REPL
	RunSapfRepl(th, engine.logFile());

	return 0;
}
