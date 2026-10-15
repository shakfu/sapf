// SapfEngineConfig::openFiles: a host can stop sapf handing written files to a viewer with system().
#include <gtest/gtest.h>
#include "test_common.hpp"
#include "VM.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

class OpenFilesTest : public SapfTestBase {
protected:
    void SetUp() override
    {
        dir_ = fs::temp_directory_path() / ("sapf-open-" + std::to_string(::getpid()));
        fs::create_directories(dir_);
        // `open` and `xdg-open` on PATH record their argument instead of opening anything
        for (const char* name : { "open", "xdg-open" }) {
            fs::path stub = dir_ / name;
            std::ofstream(stub) << "#!/bin/sh\necho \"$1\" >> '" << (dir_ / "opened").string() << "'\n";
            fs::permissions(stub, fs::perms::owner_all);
        }
        oldPath_ = getenv("PATH");
        setenv("PATH", (dir_.string() + ":" + oldPath_).c_str(), 1);
        setenv("TMPDIR", dir_.c_str(), 1);  // where sgram writes
    }

    void TearDown() override
    {
        setenv("PATH", oldPath_.c_str(), 1);
        unsetenv("TMPDIR");
        vm.openFiles = true;
        fs::remove_all(dir_);
    }

    bool opened() { return fs::exists(dir_ / "opened"); }

    void sgram()
    {
        P<Fun> fun;
        ASSERT_TRUE(th_.compile("440 0 sinosc .05 T 100 0 sgram", fun, true));
        fun->run(th_);
    }

    fs::path dir_;
    std::string oldPath_;
    Thread th_;
};

TEST_F(OpenFilesTest, OffWritesWithoutOpening) {
    vm.openFiles = false;
    sgram();
    EXPECT_FALSE(opened());
}

TEST_F(OpenFilesTest, OnOpensTheFile) {
    vm.openFiles = true;
    sgram();
    EXPECT_TRUE(opened());
}
