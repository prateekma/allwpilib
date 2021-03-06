// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "frc/motorcontrol/Spark.h"

#include <hal/FRCUsageReporting.h>

using namespace frc;

Spark::Spark(int channel) : PWMMotorController("Spark", channel) {
  m_pwm.SetBounds(2.003, 1.55, 1.50, 1.46, 0.999);
  m_pwm.SetPeriodMultiplier(PWM::kPeriodMultiplier_1X);
  m_pwm.SetSpeed(0.0);
  m_pwm.SetZeroLatch();

  HAL_Report(HALUsageReporting::kResourceType_RevSPARK, GetChannel() + 1);
}
