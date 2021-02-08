// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "sysid/analysis/AnalysisManager.h"

#include <algorithm>
#include <initializer_list>
#include <system_error>
#include <units/angle.h>

#include <wpi/StringMap.h>
#include <wpi/json.h>
#include <wpi/raw_istream.h>
#include <wpi/raw_ostream.h>

using namespace sysid;

/**
 * Concatenates a list of vectors to the end of a vector. The contents of the
 * source vectors are copied (not moved) into the new vector.
 */
std::vector<PreparedData> Concatenate(
    std::vector<PreparedData> dest,
    std::initializer_list<const std::vector<PreparedData>*> srcs) {
  // Copy the contents of the source vectors into the dest vector.
  for (auto ptr : srcs) {
    dest.insert(dest.end(), ptr->cbegin(), ptr->cend());
  }

  // Return the dest vector.
  return dest;
}

AnalysisManager::AnalysisManager(wpi::StringRef path, Settings settings)
    : m_settings(std::move(settings)) {
  // Read JSON from the specified path.
  std::error_code ec;
  wpi::raw_fd_istream is{path, ec};

  if (ec) {
    throw std::runtime_error("Unable to read: " + path.str());
  }

  is >> m_json;

  // Get the analysis type from the JSON.
  m_type = sysid::analysis::FromName(m_json.at("test").get<std::string>());

  // Get the rotation -> output units factor from the JSON.
  m_unit = m_json.at("units").get<std::string>();
  m_factor = m_json.at("unitsPerRotation").get<double>();

  has_trackwidth = m_json.find("trackwidth") != m_json.end();

  // Prepare data.
  PrepareData();
}

// Helper function that prepares data for drivetrain.
void AnalysisManager::PrepareDataDrivetrain(
    wpi::StringMap<std::vector<RawData>>&& data) {
  // Compute acceleration on all datasets.
  int window = *m_settings.windowSize;
  auto sfl = ComputeAcceleration(data["slow-forward"], window);
  auto sfr = ComputeAcceleration(data["slow-forward"], window, true);
  auto sbl = ComputeAcceleration(data["slow-backward"], window);
  auto sbr = ComputeAcceleration(data["slow-backward"], window, true);
  auto ffl = ComputeAcceleration(data["fast-forward"], window);
  auto ffr = ComputeAcceleration(data["fast-forward"], window, true);
  auto fbl = ComputeAcceleration(data["fast-backward"], window);
  auto fbr = ComputeAcceleration(data["fast-backward"], window, true);

  // Trim the step voltage data.
  TrimStepVoltageData(&ffl);
  TrimStepVoltageData(&ffr);
  TrimStepVoltageData(&fbl);
  TrimStepVoltageData(&fbr);

  // Create the distinct datasets and store them in our StringMap.
  auto sf = Concatenate(sfl, {&sfr});
  auto sb = Concatenate(sbl, {&sbr});
  auto ff = Concatenate(ffl, {&ffr});
  auto fb = Concatenate(fbl, {&fbr});

  m_datasets["Forward"] = std::make_tuple(sf, ff);
  m_datasets["Backward"] = std::make_tuple(sb, fb);
  m_datasets["Combined"] =
      std::make_tuple(Concatenate(sf, {&sb}), Concatenate(ff, {&fb}));

  m_datasets["Left Forward"] = std::make_tuple(sfl, ffl);
  m_datasets["Left Backward"] = std::make_tuple(sbl, fbl);
  m_datasets["Left Combined"] =
      std::make_tuple(Concatenate(sfl, {&sbl}), Concatenate(ffl, {&fbl}));

  m_datasets["Right Forward"] = std::make_tuple(sfr, ffr);
  m_datasets["Right Backward"] = std::make_tuple(sbr, fbr);
  m_datasets["Right Combined"] =
      std::make_tuple(Concatenate(sfr, {&sbr}), Concatenate(ffr, {&fbr}));
}

void AnalysisManager::PrepareData() {
  // Get the major components from the JSON and store them inside a StringMap.
  wpi::StringMap<std::vector<RawData>> data;
  for (auto&& key : kJsonDataKeys) {
    data[key] = m_json.at(key).get<std::vector<RawData>>();
  }

  // Ensure that voltage and velocity have the same sign; apply conversion
  // factor.
  for (auto it = data.begin(); it != data.end(); ++it) {
    for (auto&& pt : it->second) {
      pt[3] = std::copysign(pt[3], pt[7]);
      pt[4] = std::copysign(pt[4], pt[8]);
      pt[5] *= m_factor;
      pt[6] *= m_factor;
      pt[7] *= m_factor;
      pt[8] *= m_factor;
    }
  }

  // Trim quasistatic test data to remove all points where voltage == 0 or
  // velocity < threshold.
  TrimQuasistaticData(&data["slow-forward"], *m_settings.motionThreshold);
  TrimQuasistaticData(&data["slow-backward"], *m_settings.motionThreshold);

  // If the type is drivetrain, we can use our special function from here.
  if (m_type == analysis::kDrivetrain) {
    PrepareDataDrivetrain(std::move(data));
    return;
  }

  // Compute acceleration on all datasets.
  auto sf = ComputeAcceleration(data["slow-forward"], *m_settings.windowSize);
  auto sb = ComputeAcceleration(data["slow-backward"], *m_settings.windowSize);
  auto ff = ComputeAcceleration(data["fast-forward"], *m_settings.windowSize);
  auto fb = ComputeAcceleration(data["fast-backward"], *m_settings.windowSize);

  // Trim the step voltage data.
  TrimStepVoltageData(&ff);
  TrimStepVoltageData(&fb);

  // Create the distinct datasets and store them in our StringMap.
  m_datasets["Forward"] = std::make_tuple(sf, ff);
  m_datasets["Backward"] = std::make_tuple(sb, fb);
  m_datasets["Combined"] =
      std::make_tuple(Concatenate(sf, {&sb}), Concatenate(ff, {&fb}));
}

double AnalysisManager::GetDelta(std::vector<RawData> data, int column) {

  return data.back()[column] - data.front()[column];

}

AnalysisManager::Gains AnalysisManager::Calculate() {
  // Calculate feedforward gains from the data.
  auto ff = sysid::CalculateFeedforwardGains(
      m_datasets[kDatasets[*m_settings.dataset]], m_type);

  if (has_trackwidth) {
    auto trackwidth_data = m_json.at("trackwidth").get<std::vector<RawData>>();
    double left_distance = GetDelta(trackwidth_data, 5) * m_factor;
    double right_distance = GetDelta(trackwidth_data, 6) * m_factor;
    double angle = GetDelta(trackwidth_data, 9);

    std::get<0>(ff).push_back(sysid::CalculateTrackWidth(left_distance, right_distance, units::angle::radian_t {angle}));
  }

  // Create the struct that we need for feedback analysis.
  auto& f = std::get<0>(ff);
  FeedforwardGains gains = {f[0], f[1], f[2]};

  // Calculate the appropriate gains.
  std::tuple<double, double> fb;
  if (*m_settings.type == FeedbackControllerLoopType::kPosition) {
    fb = sysid::CalculatePositionFeedbackGains(*m_settings.preset,
                                               *m_settings.lqr, gains);
  } else {
    fb = sysid::CalculateVelocityFeedbackGains(*m_settings.preset,
                                               *m_settings.lqr, gains);
  }

  return {ff, fb};
}

void AnalysisManager::TrimQuasistaticData(std::vector<RawData>* data,
                                          double threshold, bool drivetrain) {
  data->erase(
      std::remove_if(data->begin(), data->end(),
                     [drivetrain, threshold](const auto& pt) {
                       // Calculate abs value of voltages.
                       double lvolts = std::abs(pt[3]);
                       double rvolts = std::abs(pt[4]);

                       // Calculate abs value of velocities.
                       double lvelocity = std::abs(pt[7]);
                       double rvelocity = std::abs(pt[8]);

                       // Calculate primary and secondary conditions (secondary
                       // is for drivetrain).
                       bool primary = lvolts <= 0 || lvelocity <= threshold;
                       bool secondary = rvolts <= 0 || rvelocity <= threshold;

                       // Return the condition, depending on whether we want
                       // secondary or not.
                       return primary || (drivetrain ? secondary : false);
                     }),
      data->end());
}

std::vector<PreparedData> AnalysisManager::ComputeAcceleration(
    const std::vector<RawData>& data, int window, bool right) {
  size_t step = window / 2;
  std::vector<PreparedData> prepared;

  prepared.reserve(data.size() - window);

  if (data.size() < static_cast<size_t>(window)) {
    throw std::runtime_error("The size of the data is too small.");
  }

  size_t pos = right ? 6 : 5;
  size_t vel = right ? 8 : 7;

  // Compute acceleration and add it to the vector.
  for (size_t i = step; i < data.size() - step; ++i) {
    auto& pt = data[i];
    double acc = (data[i + step][vel] - data[i - step][vel]) /
                 (data[i + step][0] - data[i - step][0]);

    // Sometimes, if the encoder velocities are the same, it will register
    // zero acceleration. Do not include these values.
    if (acc != 0) {
      prepared.push_back({pt[0], pt[3], pt[pos], pt[vel], acc, 0.0});
    }
  }

  return prepared;
}

void AnalysisManager::TrimStepVoltageData(std::vector<PreparedData>* data) {
  // We want to find the point where the acceleration data roughly stops
  // decreasing at the beginning.
  size_t idx = 0;

  // We will use this to make sure that the acceleration is decreasing for 3
  // consecutive entries in a row. This will help avoid false positives from
  // bad data.
  bool caution = false;

  for (size_t i = 0; i < data->size(); ++i) {
    // Get the current acceleration.
    double acc = data->at(i).acceleration;

    // If we are not in caution, the acceleration values are still increasing.
    if (!caution) {
      if (acc < std::abs(data->at(idx).acceleration)) {
        // We found a potential candidate. Let's mark the flag and continue
        // checking...
        caution = true;
      } else {
        // Set the current acceleration to be the highest so far.
        idx = i;
      }
    } else {
      // Check to make sure the acceleration value is still smaller. If it
      // isn't, break out of caution.
      if (acc >= std::abs(data->at(idx).acceleration)) {
        caution = false;
        idx = i;
      }
    }

    // If we were in caution for three iterations, we can exit.
    if (caution && (i - idx) == 3)
      break;
  }
}
