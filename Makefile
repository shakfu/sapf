
.phony: all build xcode clean


all: build


build:
	@mkdir -p build && cd build && \
		cmake .. && \
		cmake --build . --config Release

xcode:
	@xcodebuild -project SoundAsPureForm.xcodeproj -target SoundAsPureForm

clean:
	@rm -rf build
