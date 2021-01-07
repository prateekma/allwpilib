// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glass/View.h>
#include <portable-file-dialogs.h>
#include <wpi/StringMap.h>

#include "sysid/analysis/AnalysisManager.h"
#include "sysid/analysis/AnalysisType.h"
#include "sysid/analysis/FeedbackAnalysis.h"
#include "sysid/analysis/FeedbackControllerPreset.h"

namespace sysid {
class Analyzer : public glass::View {
 public:
  static constexpr const char* kPresetNames[] = {
      "Default",    "WPILib (2020-)",  "WPILib (Pre-2020)", "CTRE (New)",
      "CTRE (Old)", "REV (Brushless)", "REV (Brushed)"};

  static constexpr const char* kLoopTypes[] = {"Position", "Velocity"};

  Analyzer();

  void Display() override;

 private:
  void SelectFile();
  void Calculate();

  bool first = true;
  std::string m_exception;

  // Everything related to feedback controller calculations.
  wpi::StringMap<FeedbackControllerPreset> m_presets;
  FeedbackControllerPreset m_preset = presets::kDefault;
  FeedbackControllerLoopType m_loopType = FeedbackControllerLoopType::kVelocity;
  LQRParameters m_params{1, 1.5, 7};

  int m_selectedDataset = 0;
  int m_selectedLoopType = 1;
  int m_selectedPreset = 0;

  // Feedforward and feedback gains.
  std::vector<double> m_ff;
  double m_rs;
  double m_Kp;
  double m_Kd;

  // Units
  double m_factor;
  std::string m_unit;

  // Data analysis
  std::unique_ptr<AnalysisManager> m_manager;
  AnalysisType m_type;

  // File manipulation
  std::unique_ptr<pfd::open_file> m_selector;
  std::string* m_location;
};
}  // namespace sysid
