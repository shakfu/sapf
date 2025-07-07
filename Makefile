
.phony: all build xcode xcode-native xcode-universal clean


all: build


build:
	@mkdir -p build && cd build && \
		cmake .. && \
		cmake --build . --config Release

xcode: xcode-native

xcode-native:
	@xcodebuild -project SoundAsPureForm.xcodeproj -arch arm64 -target SoundAsPureForm 

xcode-universal:
	@xcodebuild -project SoundAsPureForm.xcodeproj -target SoundAsPureForm 

clean:
	@rm -rf build
