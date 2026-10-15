
.PHONY: all build clean test editor


all: build


build:
	@mkdir -p build && cd build && \
		cmake .. && \
		cmake --build . --config Release

test: build
	@cd build && ctest --output-on-failure

# Opens the CLAP plugin's editor in a minimal host, with audio to the default output.
editor: build
	@./build/examples/clap/sapf_dev_host

clean:
	@rm -rf build
