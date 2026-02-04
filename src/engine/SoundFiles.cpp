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

#include "SoundFiles.hpp"
#include <valarray>
#include <atomic>
#include <filesystem>

extern char gSessionTime[256];

std::atomic<int32_t> gFileCount = 0;

void makeRecordingPath(Arg filename, char* path, int len)
{
	auto tempDir = std::filesystem::temp_directory_path().string();
	if (filename.isString()) {
		const char* recDir = getenv("SAPF_RECORDINGS");
		if (!recDir || strlen(recDir)==0) recDir = tempDir.c_str();
		snprintf(path, len, "%s/%s.wav", recDir, ((String*)filename.o())->s);
	} else {
		int32_t count = ++gFileCount;
		snprintf(path, len, "%s/sapf-%s-%04d.wav", tempDir.c_str(), gSessionTime, count);
	}
}

#if defined(__APPLE__)
// macOS implementation using AudioToolbox

class SFReaderOutputChannel;

class SFReader : public Object
{
	ExtAudioFileRef mXAF;
	int64_t mFramesRemaining;
	SFReaderOutputChannel* mOutputs;
	int mNumChannels;
	AudioBufferList* mABL;
	bool mFinished = false;

public:

	SFReader(ExtAudioFileRef inXAF, int inNumChannels, int64_t inDuration);

	~SFReader();

	virtual const char* TypeName() const override { return "SFReader"; }

	P<List> createOutputs(Thread& th);

	bool pull(Thread& th);
	void fulfillOutputs(int blockSize);
	void produceOutputs(int shrinkBy);
};

class SFReaderOutputChannel : public Gen
{
	friend class SFReader;
	P<SFReader> mSFReader;
	SFReaderOutputChannel* mNextOutput = nullptr;
	Z* mDummy = nullptr;

public:
	SFReaderOutputChannel(Thread& th, SFReader* inSFReader)
        : Gen(th, itemTypeZ, true), mSFReader(inSFReader)
	{
	}

	~SFReaderOutputChannel()
	{
		if (mDummy) free(mDummy);
	}

	virtual void norefs() override
	{
		mOut = nullptr;
		mSFReader = nullptr;
	}

	virtual const char* TypeName() const override { return "SFReaderOutputChannel"; }

	virtual void pull(Thread& th) override
	{
		if (mSFReader->pull(th)) {
			end();
		}
	}

};

SFReader::SFReader(ExtAudioFileRef inXAF, int inNumChannels, int64_t inDuration)
	: mXAF(inXAF), mNumChannels(inNumChannels), mFramesRemaining(inDuration), mABL(nullptr)
{
	mABL = (AudioBufferList*)calloc(1, sizeof(AudioBufferList) + (mNumChannels - 1) * sizeof(AudioBuffer));
}

SFReader::~SFReader()
{
	ExtAudioFileDispose(mXAF); free(mABL);
	SFReaderOutputChannel* output = mOutputs;
	do {
		SFReaderOutputChannel* next = output->mNextOutput;
		delete output;
		output = next;
	} while (output);
}

void SFReader::fulfillOutputs(int blockSize)
{
	mABL->mNumberBuffers = mNumChannels;
	SFReaderOutputChannel* output = mOutputs;
	size_t bufSize = blockSize * sizeof(Z);
	for (int i = 0; output; ++i, output = output->mNextOutput){
		Z* out;
		if (output->mOut)
			out = output->mOut->fulfillz(blockSize);
		else {
			if (!output->mDummy)
				output->mDummy = (Z*)calloc(output->mBlockSize, sizeof(Z));

			out = output->mDummy;
		}

		mABL->mBuffers[i].mNumberChannels = 1;
		mABL->mBuffers[i].mData = out;
		mABL->mBuffers[i].mDataByteSize = (UInt32)bufSize;
		memset(out, 0, bufSize);
	};
}

void SFReader::produceOutputs(int shrinkBy)
{
	SFReaderOutputChannel* output = mOutputs;
	do {
		if (output->mOut)
			output->produce(shrinkBy);
		output = output->mNextOutput;
	} while (output);
}

P<List> SFReader::createOutputs(Thread& th)
{
	P<List> s = new List(itemTypeV, mNumChannels);

	// fill s->mArray with ola's output channels.
    SFReaderOutputChannel* last = nullptr;
	P<Array> a = s->mArray;
	for (int i = 0; i < mNumChannels; ++i) {
        SFReaderOutputChannel* c = new SFReaderOutputChannel(th, this);
        if (last) last->mNextOutput = c;
        else mOutputs = c;
        last = c;
		a->add(new List(c));
	}

	return s;
}

bool SFReader::pull(Thread& th)
{
	if (mFramesRemaining == 0)
		mFinished = true;

	if (mFinished)
		return true;

	SFReaderOutputChannel* output = mOutputs;
	int blockSize = output->mBlockSize;
	if (mFramesRemaining > 0)
		blockSize = (int)std::min(mFramesRemaining, (int64_t)blockSize);

	fulfillOutputs(blockSize);

	// read file here.
	UInt32 framesRead = blockSize;
	OSStatus err = ExtAudioFileRead(mXAF, &framesRead, mABL);

	if (err || framesRead == 0) {
		mFinished = true;
	}

	produceOutputs(blockSize - framesRead);
	if (mFramesRemaining > 0) mFramesRemaining -= blockSize;

	return mFinished;
}

void sfread(Thread& th, Arg filename, int64_t offset, int64_t frames)
{
	const char* path = ((String*)filename.o())->s;

	CFStringRef cfpath = CFStringCreateWithFileSystemRepresentation(0, path);
	if (!cfpath) {
		post("failed to create path\n");
		return;
	}
	CFReleaser cfpathReleaser(cfpath);

	CFURLRef url = CFURLCreateWithFileSystemPath(0, cfpath, kCFURLPOSIXPathStyle, false);
	if (!url) {
		post("failed to create url\n");
		return;
	}
	CFReleaser urlReleaser(url);

	ExtAudioFileRef xaf;
	OSStatus err = ExtAudioFileOpenURL(url, &xaf);

	cfpathReleaser.release();
	urlReleaser.release();

	if (err) {
		post("failed to open file %d\n", (int)err);
		return;
	}

	AudioStreamBasicDescription fileFormat;

	UInt32 propSize = sizeof(fileFormat);
	err = ExtAudioFileGetProperty(xaf, kExtAudioFileProperty_FileDataFormat, &propSize, &fileFormat);

	int numChannels = fileFormat.mChannelsPerFrame;

	AudioStreamBasicDescription clientFormat = {
		th.rate.sampleRate,
		kAudioFormatLinearPCM,
		kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved,
		static_cast<UInt32>(sizeof(double)),
		1,
		static_cast<UInt32>(sizeof(double)),
		static_cast<UInt32>(numChannels),
		64,
		0
	};

	err = ExtAudioFileSetProperty(xaf, kExtAudioFileProperty_ClientDataFormat, sizeof(clientFormat), &clientFormat);
	if (err) {
		post("failed to set client data format\n");
		ExtAudioFileDispose(xaf);
		return;
	}

	err = ExtAudioFileSeek(xaf, offset);
	if (err) {
		post("seek failed %d\n", (int)err);
		ExtAudioFileDispose(xaf);
		return;
	}

	SFReader* sfr = new SFReader(xaf, numChannels, -1);

	th.push(sfr->createOutputs(th));
}

ExtAudioFileRef sfcreate(Thread& th, const char* path, int numChannels, double fileSampleRate, bool interleaved)
{
	if (fileSampleRate == 0.)
		fileSampleRate = th.rate.sampleRate;

	CFStringRef cfpath = CFStringCreateWithFileSystemRepresentation(0, path);
	if (!cfpath) {
		post("failed to create path '%s'\n", path);
		return nullptr;
	}
	CFReleaser cfpathReleaser(cfpath);

	CFURLRef url = CFURLCreateWithFileSystemPath(0, cfpath, kCFURLPOSIXPathStyle, false);
	if (!url) {
		post("failed to create url\n");
		return nullptr;
	}
	CFReleaser urlReleaser(url);

	AudioStreamBasicDescription fileFormat = {
		fileSampleRate,
		kAudioFormatLinearPCM,
		kAudioFormatFlagsNativeFloatPacked,
		static_cast<UInt32>(sizeof(float) * numChannels),
		1,
		static_cast<UInt32>(sizeof(float) * numChannels),
		static_cast<UInt32>(numChannels),
		32,
		0
	};

	int interleavedChannels = interleaved ? numChannels : 1;
	UInt32 interleavedBit = interleaved ? 0 : kAudioFormatFlagIsNonInterleaved;

	AudioStreamBasicDescription clientFormat = {
		th.rate.sampleRate,
		kAudioFormatLinearPCM,
		kAudioFormatFlagsNativeFloatPacked | interleavedBit,
		static_cast<UInt32>(sizeof(float) * interleavedChannels),
		1,
		static_cast<UInt32>(sizeof(float) * interleavedChannels),
		static_cast<UInt32>(numChannels),
		32,
		0
	};

	ExtAudioFileRef xaf;
	OSStatus err = ExtAudioFileCreateWithURL(url, kAudioFileWAVEType, &fileFormat, nullptr, kAudioFileFlags_EraseFile, &xaf);

	if (err) {
		post("failed to create file '%s'. err: %d\n", path, (int)err);
		return nullptr;
	}

	err = ExtAudioFileSetProperty(xaf, kExtAudioFileProperty_ClientDataFormat, sizeof(clientFormat), &clientFormat);
	if (err) {
		post("failed to set client data format\n");
		ExtAudioFileDispose(xaf);
		return nullptr;
	}

	return xaf;
}

void sfwrite(Thread& th, V& v, Arg filename, bool openIt)
{
	std::vector<ZIn> in;

	int numChannels = 0;

	if (v.isZList()) {
		if (!v.isFinite()) indefiniteOp(">sf : s - indefinite number of frames", "");
		numChannels = 1;
		in.push_back(ZIn(v));
	} else {
		if (!v.isFinite()) indefiniteOp(">sf : s - indefinite number of channels", "");
		P<List> s = (List*)v.o();
		s = s->pack(th);
		Array* a = s->mArray();
		numChannels = (int)a->size();

		if (numChannels > kMaxSFChannels)
			throw errOutOfRange;

		bool allIndefinite = true;
		for (int i = 0; i < numChannels; ++i) {
			V va = a->at(i);
			if (va.isFinite()) allIndefinite = false;
			in.push_back(ZIn(va));
			va.o = nullptr;
		}

		s = nullptr;
		a = nullptr;

		if (allIndefinite) indefiniteOp(">sf : s - all channels have indefinite number of frames", "");
	}
	v.o = nullptr;

	char path[1024];

	makeRecordingPath(filename, path, 1024);

	ExtAudioFileRef xaf = sfcreate(th, path, numChannels, 0., true);
	if (!xaf) return;

	std::valarray<float> buf(0., numChannels * kBufSize);
	AudioBufferList abl;
	abl.mNumberBuffers = 1;
	abl.mBuffers[0].mNumberChannels = numChannels;
	abl.mBuffers[0].mData = &buf[0];
	abl.mBuffers[0].mDataByteSize = kBufSize * sizeof(float);

	int64_t framesPulled = 0;
	int64_t framesWritten = 0;
	bool done = false;
	while (!done) {
		int minn = kBufSize;
		memset(&buf[0], 0, kBufSize * numChannels);
		for (int i = 0; i < numChannels; ++i) {
			int n = kBufSize;
			bool imdone = in[i].fill(th, n, &buf[0]+i, numChannels);
			framesPulled += n;
			if (imdone) done = true;
			minn = std::min(n, minn);
		}

		abl.mBuffers[0].mDataByteSize = minn * sizeof(float);
		OSStatus err = ExtAudioFileWrite(xaf, minn, &abl);
		if (err) {
			post("ExtAudioFileWrite failed %d\n", (int)err);
			break;
		}

		framesWritten += minn;
	}

	post("wrote file '%s'  %d channels  %g secs\n", path, numChannels, framesWritten * th.rate.invSampleRate);

	ExtAudioFileDispose(xaf);

	if (openIt) {
		char cmd[1100];
		snprintf(cmd, 1100, "open \"%s\"", path);
		system(cmd);
	}
}

#elif defined(SAPF_USE_LIBSNDFILE)
// Non-Apple platforms: libsndfile implementation

#include <sndfile.h>

class SFReaderOutputChannel;

class SFReader : public Object
{
	SNDFILE* mSF;
	SF_INFO mSFInfo;
	int64_t mFramesRemaining;
	SFReaderOutputChannel* mOutputs;
	int mNumChannels;
	std::vector<double> mInterleavedBuffer;
	bool mFinished = false;

public:
	SFReader(SNDFILE* inSF, SF_INFO& inInfo, int64_t inDuration);
	~SFReader();

	virtual const char* TypeName() const override { return "SFReader"; }

	P<List> createOutputs(Thread& th);
	bool pull(Thread& th);
	void fulfillOutputs(int blockSize);
	void produceOutputs(int shrinkBy);
};

class SFReaderOutputChannel : public Gen
{
	friend class SFReader;
	P<SFReader> mSFReader;
	SFReaderOutputChannel* mNextOutput = nullptr;
	Z* mDummy = nullptr;
	Z* mOutBuffer = nullptr;

public:
	SFReaderOutputChannel(Thread& th, SFReader* inSFReader)
		: Gen(th, itemTypeZ, true), mSFReader(inSFReader)
	{
	}

	~SFReaderOutputChannel()
	{
		if (mDummy) free(mDummy);
	}

	virtual void norefs() override
	{
		mOut = nullptr;
		mSFReader = nullptr;
	}

	virtual const char* TypeName() const override { return "SFReaderOutputChannel"; }

	virtual void pull(Thread& th) override
	{
		if (mSFReader->pull(th)) {
			end();
		}
	}
};

SFReader::SFReader(SNDFILE* inSF, SF_INFO& inInfo, int64_t inDuration)
	: mSF(inSF), mSFInfo(inInfo), mNumChannels(inInfo.channels), mFramesRemaining(inDuration)
{
}

SFReader::~SFReader()
{
	if (mSF) sf_close(mSF);
	SFReaderOutputChannel* output = mOutputs;
	while (output) {
		SFReaderOutputChannel* next = output->mNextOutput;
		delete output;
		output = next;
	}
}

void SFReader::fulfillOutputs(int blockSize)
{
	mInterleavedBuffer.resize(blockSize * mNumChannels);
	SFReaderOutputChannel* output = mOutputs;
	for (int i = 0; output; ++i, output = output->mNextOutput) {
		if (output->mOut) {
			output->mOutBuffer = output->mOut->fulfillz(blockSize);
		} else {
			if (!output->mDummy)
				output->mDummy = (Z*)calloc(output->mBlockSize, sizeof(Z));
			output->mOutBuffer = output->mDummy;
		}
		memset(output->mOutBuffer, 0, blockSize * sizeof(Z));
	}
}

void SFReader::produceOutputs(int shrinkBy)
{
	SFReaderOutputChannel* output = mOutputs;
	while (output) {
		if (output->mOut)
			output->produce(shrinkBy);
		output = output->mNextOutput;
	}
}

P<List> SFReader::createOutputs(Thread& th)
{
	P<List> s = new List(itemTypeV, mNumChannels);

	SFReaderOutputChannel* last = nullptr;
	P<Array> a = s->mArray;
	for (int i = 0; i < mNumChannels; ++i) {
		SFReaderOutputChannel* c = new SFReaderOutputChannel(th, this);
		if (last) last->mNextOutput = c;
		else mOutputs = c;
		last = c;
		a->add(new List(c));
	}

	return s;
}

bool SFReader::pull(Thread& th)
{
	if (mFramesRemaining == 0)
		mFinished = true;

	if (mFinished)
		return true;

	SFReaderOutputChannel* output = mOutputs;
	int blockSize = output->mBlockSize;
	if (mFramesRemaining > 0)
		blockSize = (int)std::min(mFramesRemaining, (int64_t)blockSize);

	fulfillOutputs(blockSize);

	// Read interleaved data from file
	sf_count_t framesRead = sf_readf_double(mSF, mInterleavedBuffer.data(), blockSize);

	if (framesRead == 0) {
		mFinished = true;
	}

	// Deinterleave into output channels
	for (sf_count_t frame = 0; frame < framesRead; ++frame) {
		SFReaderOutputChannel* out = mOutputs;
		for (int ch = 0; ch < mNumChannels && out; ++ch, out = out->mNextOutput) {
			out->mOutBuffer[frame] = mInterleavedBuffer[frame * mNumChannels + ch];
		}
	}

	produceOutputs(blockSize - (int)framesRead);
	if (mFramesRemaining > 0) mFramesRemaining -= blockSize;

	return mFinished;
}

void sfread(Thread& th, Arg filename, int64_t offset, int64_t frames)
{
	const char* path = ((String*)filename.o())->s;

	SF_INFO sfinfo;
	memset(&sfinfo, 0, sizeof(sfinfo));

	SNDFILE* sf = sf_open(path, SFM_READ, &sfinfo);
	if (!sf) {
		post("sfread: failed to open file '%s': %s\n", path, sf_strerror(nullptr));
		return;
	}

	if (offset > 0) {
		sf_count_t seekResult = sf_seek(sf, offset, SEEK_SET);
		if (seekResult < 0) {
			post("sfread: seek failed for file '%s'\n", path);
			sf_close(sf);
			return;
		}
	}

	SFReader* sfr = new SFReader(sf, sfinfo, frames);
	th.push(sfr->createOutputs(th));
}

SNDFILE* sfcreate_sndfile(const char* path, int numChannels, double fileSampleRate)
{
	SF_INFO sfinfo;
	memset(&sfinfo, 0, sizeof(sfinfo));
	sfinfo.samplerate = (int)fileSampleRate;
	sfinfo.channels = numChannels;
	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

	SNDFILE* sf = sf_open(path, SFM_WRITE, &sfinfo);
	if (!sf) {
		post("sfcreate: failed to create file '%s': %s\n", path, sf_strerror(nullptr));
		return nullptr;
	}

	return sf;
}

void sfwrite(Thread& th, V& v, Arg filename, bool openIt)
{
	std::vector<ZIn> in;

	int numChannels = 0;

	if (v.isZList()) {
		if (!v.isFinite()) indefiniteOp(">sf : s - indefinite number of frames", "");
		numChannels = 1;
		in.push_back(ZIn(v));
	} else {
		if (!v.isFinite()) indefiniteOp(">sf : s - indefinite number of channels", "");
		P<List> s = (List*)v.o();
		s = s->pack(th);
		Array* a = s->mArray();
		numChannels = (int)a->size();

		if (numChannels > kMaxSFChannels)
			throw errOutOfRange;

		bool allIndefinite = true;
		for (int i = 0; i < numChannels; ++i) {
			V va = a->at(i);
			if (va.isFinite()) allIndefinite = false;
			in.push_back(ZIn(va));
			va.o = nullptr;
		}

		s = nullptr;
		a = nullptr;

		if (allIndefinite) indefiniteOp(">sf : s - all channels have indefinite number of frames", "");
	}
	v.o = nullptr;

	char path[1024];
	makeRecordingPath(filename, path, 1024);

	double fileSampleRate = th.rate.sampleRate;
	SNDFILE* sf = sfcreate_sndfile(path, numChannels, fileSampleRate);
	if (!sf) return;

	std::valarray<float> buf(0.f, numChannels * kBufSize);

	int64_t framesWritten = 0;
	bool done = false;
	while (!done) {
		int minn = kBufSize;
		memset(&buf[0], 0, kBufSize * numChannels * sizeof(float));
		for (int i = 0; i < numChannels; ++i) {
			int n = kBufSize;
			bool imdone = in[i].fill(th, n, &buf[0] + i, numChannels);
			if (imdone) done = true;
			minn = std::min(n, minn);
		}

		sf_count_t written = sf_writef_float(sf, &buf[0], minn);
		if (written != minn) {
			post("sfwrite: write error: %s\n", sf_strerror(sf));
			break;
		}

		framesWritten += minn;
	}

	post("wrote file '%s'  %d channels  %g secs\n", path, numChannels, framesWritten * th.rate.invSampleRate);

	sf_close(sf);

	if (openIt) {
		// Cross-platform file opening
#if defined(_WIN32)
		char cmd[1100];
		snprintf(cmd, 1100, "start \"\" \"%s\"", path);
		system(cmd);
#elif defined(__linux__)
		char cmd[1100];
		snprintf(cmd, 1100, "xdg-open \"%s\" &", path);
		system(cmd);
#endif
	}
}

#else
// Non-Apple platforms without libsndfile: stub implementations

void sfread(Thread& th, Arg filename, int64_t offset, int64_t frames)
{
	post("sfread: Sound file reading not available on this platform.\n");
	post("        Install libsndfile and rebuild with SAPF_USE_LIBSNDFILE=ON.\n");
}

void sfwrite(Thread& th, V& v, Arg filename, bool openIt)
{
	post("sfwrite: Sound file writing not available on this platform.\n");
	post("         Install libsndfile and rebuild with SAPF_USE_LIBSNDFILE=ON.\n");
}

#endif // __APPLE__ / SAPF_USE_LIBSNDFILE
