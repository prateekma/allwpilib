#include "gtest/gtest.h"
#include <stdio.h>
#include <memory>
#include <chrono>
#include <thread>

#include <wpi/StringRef.h>

#include "sysid/telemetry/TelemetryManager.h"
#include "sysid/analysis/AnalysisManager.h"

#if defined(__GNUG__) && !defined(__clang__) && __GNUC__ < 8
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include <ntcore_cpp.h>
#include <glass/networktables/NetworkTablesHelper.h>

using namespace sysid;
using namespace std::chrono_literals;

std::string getCodePath() {
  std::string wpilib_name = "allwpilib";

  // find build path
  std::string curr_path {fs::current_path()};
  std::size_t wpilib_dir = curr_path.find(wpilib_name);
  wpilib_dir += (wpilib_name.length());
  std::string build_path = 
    curr_path.substr(0, wpilib_dir) + 
    "'/sysid/src/test/native/cpp/integration/Project'";
  return build_path;
}

void RunTest(TelemetryManager& m_manager, NT_Entry m_enable, wpi::StringRef test) {
  nt::SetEntryValue(m_enable, nt::Value::MakeBoolean(true));
  m_manager.BeginTest(test);

  auto start = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start < std::chrono::seconds(10)) {
    m_manager.Update();
  }
  m_manager.EndTest();

  nt::SetEntryValue(m_enable, nt::Value::MakeBoolean(false));
  std::this_thread::sleep_for(5s);
  

}

// Logger Constants
double m_quasistatic = .25;
double m_step = 5.0;

// Analyzer Constants
FeedbackControllerPreset m_preset = presets::kDefault;
FeedbackControllerLoopType m_loopType = FeedbackControllerLoopType::kVelocity;
LQRParameters m_params {0.05f, 0.05f, 0.1f};
double m_threshold = 0.2; 
int m_window = 8;
int m_selectedDataset = 0;


class FullTest : public ::testing::Test {
public:
  
  
  FullTest() 
    : m_client(nt::CreateInstance()),
    m_nt(m_client),
    m_kill(m_nt.GetEntry("/SmartDashboard/SysIdKill")),
    m_enable(m_nt.GetEntry("/SmartDashboard/SysIdRun"))
  {
      std::string command = "./gradlew simulatejava ";
      std::string jdk = std::getenv("HOME");
      jdk += ((jdk.back() == fs::path::preferred_separator ? "" : "/")) +
            std::string("wpilib/2021/jdk");

      if (fs::exists(fs::status(jdk))) {
        command += "-Dorg.gradle.java.home=" + jdk + " 2>&1";
      }

      std::string savePath = getCodePath();

      auto pipe = popen(
        std::string("cd " + savePath + ";" + "chmod +x gradlew;" + command).c_str(),
        "w");
      pclose(pipe);

      nt::StartClient(m_client, "localhost", NT_DEFAULT_PORT);

      
      NT_ConnectionListenerPoller poller =
      nt::CreateConnectionListenerPoller(m_client);
      nt::AddPolledConnectionListener(poller, false);
      bool timed_out = false;
      if (nt::PollConnectionListener(poller, 1.0, &timed_out).empty()) {
        
      }
      std::this_thread::sleep_for(5s);
      
      nt::SetEntryValue(m_kill, nt::Value::MakeBoolean(false));
      nt::Flush(m_client);
    }

  ~FullTest() {
    nt::SetEntryValue(m_kill, nt::Value::MakeBoolean(true));
    std::this_thread::sleep_for(1s);
  } 


  NT_Inst m_client;
  glass::NetworkTablesHelper m_nt;

  NT_Entry m_kill;
  NT_Entry m_enable;
  
  
  

};


TEST_F (FullTest, IntegrationTestDrive) {
  
  
  TelemetryManager m_manager {TelemetryManager::Settings{&m_quasistatic, &m_step}, m_client};

  //std::this_thread::sleep_for(20s);
  
  RunTest(m_manager, m_enable, "slow-forward");
  RunTest(m_manager, m_enable, "slow-backward");
  RunTest(m_manager, m_enable, "fast-forward");
  RunTest(m_manager, m_enable, "fast-backward");
  RunTest(m_manager, m_enable, "trackwidth");

  std::string save_path {fs::current_path()};

  std::string out_path {m_manager.SaveJSON(save_path)};

  //wpi::StringRef json_path {m_manager.SaveJSON(save_path)};
  std::cout << save_path << " " << out_path << std::endl;
  
  AnalysisManager m_analyzer {out_path, AnalysisManager::Settings{
                             &m_preset, &m_loopType, &m_params, &m_threshold,
                             &m_window, &m_selectedDataset}};
  
  auto gains = m_analyzer.Calculate();
  auto feedforward = std::get<0>(gains.ff);
  std::cout << feedforward[2] << "\n";


  EXPECT_NEAR(feedforward[1], 1.98, 5E-2);
  EXPECT_NEAR(feedforward[2], 0.2, 5E-2);
  EXPECT_NEAR(feedforward[3], .762, 1E-3);
}

// TEST_F (FullTest, IntegrationTestArm) {
//   ASSERT_EQ(0, 0);
// }

// TEST_F (FullTest, IntegrationTestSimpleMotor) {
//   ASSERT_EQ(0, 0);
// }

// TEST_F (FullTest, IntegrationTestElevator) {
//   ASSERT_EQ(0, 0);
// }