// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "sysid/analysis/AnalysisManager.h"

#include <algorithm>
#include <system_error>

#include <wpi/StringMap.h>
#include <wpi/raw_istream.h>

using namespace sysid;

AnalysisManager::AnalysisManager(wpi::StringRef path) {
  // Read JSON from the specified path.
  std::error_code ec;
  wpi::raw_fd_istream is{path, ec};

  if (ec) {
    throw std::runtime_error("Unable to read: " + path.str());
  }

  is >> m_data;

  // Get the analysis type from the JSON.
  m_type = sysid::analysis::FromName(m_data.at("test").get<std::string>());

  // Get the rotation -> output units factor from the JSON.
  m_factor = m_data.at("unitsPerRotation").get<double>();

  // Prepare the data.
  PrepareData();
}

void AnalysisManager::PrepareData() {
  using Data = std::vector<std::array<double, 10>>;
  wpi::StringMap<Data> data;

  // Store the test data inside a string map. We don't pool all of it together
  // just yet because we need to distinguish between the slow and fast tests.
  for (const char* key : kJsonDataKeys) {
    data[key] = m_data.at(key).get<Data>();
  }

  // Ensure that voltage and velocity have the same sign; apply conversion
  // factor.
  for (auto it = data.begin(); it != data.end(); ++it) {
    for (auto&& pt : it->second) {
      pt[3] = std::copysign(pt[3], pt[7]);
      pt[5] *= m_factor;
      pt[7] *= m_factor;
    }
  }

  // Trim quasistatic data before computing acceleration.
  auto TrimQuasistatic = [](Data* d) {
    d->erase(std::remove_if(d->begin(), d->end(),
                            [](const auto& pt) {
                              return std::abs(pt[3]) <= 0 ||
                                     std::abs(pt[7]) <= kMotionThreshold;
                            }),
             d->end());
  };
  TrimQuasistatic(&data["slow-forward"]);
  TrimQuasistatic(&data["slow-backward"]);

  // Compute acceleration from quasistatic data.
  auto ComputeAcceleration = [](Data* d) {
    std::vector<PreparedData> prepared;
    prepared.reserve(d->size() - 2);

    if (d->size() < 3) {
      throw std::runtime_error("The size of the data is too small.");
    }

    // Compute acceleration and add it to the vector.
    for (size_t i = 1; i < d->size() - 1; ++i) {
      auto& pt = d->at(i);
      double acc = (d->at(i + 1)[7] - d->at(i - 1)[7]) /
                   (d->at(i + 1)[0] - d->at(i - 1)[0]);
      prepared.push_back({pt[0], pt[3], pt[5], pt[7], acc, 0.0});
    }

    return prepared;
  };

  auto sf = ComputeAcceleration(&data["slow-forward"]);
  auto sb = ComputeAcceleration(&data["slow-backward"]);
  auto ff = ComputeAcceleration(&data["fast-forward"]);
  auto fb = ComputeAcceleration(&data["fast-backward"]);

  // Trim dynamic (step) test data.
  auto TrimStepData = [](std::vector<PreparedData>* d) {
    // We want to find the point where the acceleration data roughly stops
    // decreasing at the beginning.
    size_t idx = 0;

    // We will use this to make sure that the acceleration is decreasing for 3
    // consecutive entries in a row. This will help avoid false positives from
    // bad data.
    bool caution = false;

    for (size_t i = 0; i < d->size(); ++i) {
      // Get the current acceleration.
      double acc = d->at(i).acceleration;

      // If we are not in caution, the acceleration values are still increasing.
      if (!caution) {
        if (acc < std::abs(d->at(idx).acceleration)) {
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
        if (acc >= std::abs(d->at(idx).acceleration)) {
          caution = false;
          idx = i;
        }
      }

      // If we were in caution for three iterations, we can exit.
      if (caution && (i - idx) == 3)
        break;
    }
  };
  TrimStepData(&ff);
  TrimStepData(&fb);

  m_dataset["Forward"] = std::make_tuple(ff, fb);
  m_dataset["Backward"] = std::make_tuple(sb, fb);
}