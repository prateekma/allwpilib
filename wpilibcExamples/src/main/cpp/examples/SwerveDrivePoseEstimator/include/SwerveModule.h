// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <frc/Encoder.h>
#include <frc/PWMSparkMax.h>
#include <frc/controller/PIDController.h>
#include <frc/controller/ProfiledPIDController.h>
#include <frc/controller/SimpleMotorFeedforward.h>
#include <frc/kinematics/SwerveModuleState.h>
#include <units/angular_velocity.h>
#include <units/time.h>
#include <units/velocity.h>
#include <units/voltage.h>
#include <limits>
#include <wpi/math>
#include "frc/filters/LinearDigitalFilter.h"
#include "frc/simulation/EncoderSim.h"
#include "frc/simulation/FlywheelSim.h"
#include "frc/simulation/SingleJointedArmSim.h"
#include "frc/system/plant/DCMotor.h"
#include "frc/system/plant/LinearSystemId.h"
#include "units/angle.h"

class SwerveModule {
 public:
  SwerveModule(int driveMotorChannel, int turningMotorChannel,
               int encoderChannel);
  frc::SwerveModuleState GetState() const;
  void SetDesiredState(const frc::SwerveModuleState& state);

  void SimulationPeriodic();

 private:
  static constexpr auto kWheelRadius = 0.0508_m;
  static constexpr int kEncoderResolution = 4096;

  static constexpr auto kModuleMaxAngularVelocity =
      wpi::math::pi * 1_rad_per_s;  // radians per second
  static constexpr auto kModuleMaxAngularAcceleration =
      wpi::math::pi * 2_rad_per_s / 1_s;  // radians per second^2

  frc::PWMSparkMax m_driveMotor;
  frc::PWMSparkMax m_turningMotor;

  frc::Encoder m_driveEncoder;
  frc::Encoder m_turningEncoder;

  frc::sim::EncoderSim m_driveEncoderSim{m_driveEncoder};
  frc::sim::EncoderSim m_turningEncoderSim{m_turningEncoder};

  // We can use a single-jointed arm sim (without gravity) to emulate how the
  // swerve module will rotate.
  frc::sim::SingleJointedArmSim m_moduleRotationSim{
      frc::DCMotor::NEO(1),
      28.38,
      1_kg_sq_m,
      6_in,
      -std::numeric_limits<double>::max() * 1_rad,
      std::numeric_limits<double>::max() * 1_rad,
      3_kg,
      false};

  // We can use a flywheel sim to emulate the driving portion.
  frc::sim::FlywheelSim m_driveSim{
      frc::LinearSystemId::IdentifyVelocitySystem<units::meter>(
          1.98_V / 1_mps, 0.1_V / 1_mps_sq),
      frc::DCMotor::NEO(1), 7.29};

  frc2::PIDController m_drivePIDController{1.0, 0, 0};
  frc::ProfiledPIDController<units::radians> m_turningPIDController{
      1.0,
      0.0,
      0.0,
      {kModuleMaxAngularVelocity, kModuleMaxAngularAcceleration}};

  frc::SimpleMotorFeedforward<units::meters> m_driveFeedforward{1_V,
                                                                3_V / 1_mps};
  frc::SimpleMotorFeedforward<units::radians> m_turnFeedforward{
      1_V, 0.5_V / 1_rad_per_s};
};
