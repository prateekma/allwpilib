#include "gtest/gtest.h"
namespace fs = std::experimental::filesystem;
#if defined(__GNUG__) && !defined(__clang__) && __GNUC__ < 8
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

class FullTest : public ::testing::test {
public:
  FullTest() {
    std::string command = "./gradlew simulatejava ";
    std::string jdk = std::getenv("HOME");
    jdk += ((jdk.back() == fs::path::preferred_separator ? "" : "/")) +
          std::string("wpilib/2021/jdk");

    if (fs::exists(fs::status(jdk))) {
      command += "-Dorg.gradle.java.home=" + jdk + " 2>&1";

    }

    auto pipe = popen(
      std::string("cd " + savePath + ";" + "chmod +x gradlew;" + command).c_str(),
      "r");

  }

}