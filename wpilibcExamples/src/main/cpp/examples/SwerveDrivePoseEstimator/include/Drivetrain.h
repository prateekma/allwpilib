// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <frc/AnalogGyro.h>
#include <frc/estimator/SwerveDrivePoseEstimator.h>
#include <frc/geometry/Translation2d.h>
#include <frc/kinematics/SwerveDriveKinematics.h>
#include <frc/kinematics/SwerveDriveOdometry.h>
#include <wpi/math>

#include "SwerveModule.h"
#include "frc/smartdashboard/Field2d.h"

/**
 * Represents a swerve drive style drivetrain.
 */
class Drivetrain {
 public:
  Drivetrain() { m_gyro.Reset(); }

  void Drive(units::meters_per_second_t xSpeed,
             units::meters_per_second_t ySpeed, units::radians_per_second_t rot,
             bool fieldRelative);
  void UpdateOdometry();
  void SimulationPeriodic();

  static constexpr auto kMaxSpeed = 3.0_mps;  // 3 meters per second
  static constexpr units::radians_per_second_t kMaxAngularSpeed{
      wpi::math::pi};  // 1/2 rotation per second

 private:
  frc::Translation2d m_frontLeftLocation{+0.381_m, +0.381_m};
  frc::Translation2d m_frontRightLocation{+0.381_m, -0.381_m};
  frc::Translation2d m_backLeftLocation{-0.381_m, +0.381_m};
  frc::Translation2d m_backRightLocation{-0.381_m, -0.381_m};

  SwerveModule m_frontLeft{1, 2, 0};
  SwerveModule m_frontRight{2, 3, 4};
  SwerveModule m_backLeft{5, 6, 8};
  SwerveModule m_backRight{7, 8, 12};

  frc::Field2d m_field;

  frc::AnalogGyro m_gyro{0};

  frc::SwerveDriveKinematics<4> m_kinematics{
      m_frontLeftLocation, m_frontRightLocation, m_backLeftLocation,
      m_backRightLocation};

  // Gains are for example purposes only - must be determined for your own
  // robot!
  frc::SwerveDrivePoseEstimator<4> m_poseEstimator{
      frc::Rotation2d(), frc::Pose2d(), m_kinematics,
      {0.1, 0.1, 0.1},   {0.05},        {0.1, 0.1, 0.1}};
};
