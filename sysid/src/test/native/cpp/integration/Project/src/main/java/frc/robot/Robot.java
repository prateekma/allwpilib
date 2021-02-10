// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

package frc.robot;

import edu.wpi.first.wpilibj.GenericHID;
import edu.wpi.first.wpilibj.RobotController;
import edu.wpi.first.wpilibj.SlewRateLimiter;
import edu.wpi.first.wpilibj.TimedRobot;
import edu.wpi.first.wpilibj.Timer;
import edu.wpi.first.wpilibj.XboxController;
import edu.wpi.first.wpilibj.controller.RamseteController;
import edu.wpi.first.wpilibj.geometry.Pose2d;
import edu.wpi.first.wpilibj.geometry.Rotation2d;
import edu.wpi.first.wpilibj.kinematics.ChassisSpeeds;
import edu.wpi.first.wpilibj.simulation.DriverStationSim;
import edu.wpi.first.wpilibj.smartdashboard.SmartDashboard;
import edu.wpi.first.wpilibj.trajectory.Trajectory;
import edu.wpi.first.wpilibj.trajectory.TrajectoryConfig;
import edu.wpi.first.wpilibj.trajectory.TrajectoryGenerator;
import java.util.List;

public class Robot extends TimedRobot {
  private final XboxController m_controller = new XboxController(0);

  // Slew rate limiters to make joystick inputs more gentle; 1/3 sec from 0
  // to 1.
  private final SlewRateLimiter m_speedLimiter = new SlewRateLimiter(3);
  private final SlewRateLimiter m_rotLimiter = new SlewRateLimiter(3);

  private final Drivetrain m_drive = new Drivetrain();
  private final RamseteController m_ramsete = new RamseteController();
  private final Timer m_timer = new Timer();
  private Trajectory m_trajectory;

  @Override
  public void robotInit() {
    // Flush NetworkTables every loop. This ensures that robot pose and other values
    // are sent during every iteration.
    setNetworkTablesFlushEnabled(true);

    m_trajectory =
        TrajectoryGenerator.generateTrajectory(
            new Pose2d(2, 2, new Rotation2d()),
            List.of(),
            new Pose2d(6, 4, new Rotation2d()),
            new TrajectoryConfig(2, 2));

            m_drive.resetOdometry(new Pose2d(2, 2, new Rotation2d()));
  }

  @Override
  public void robotPeriodic() {
    m_drive.periodic();
  }

  @Override
  public void autonomousInit() {
    
  }

  @Override
  public void autonomousPeriodic() {
    double speed = SmartDashboard.getNumber("SysIdAutoSpeed", 0.0);
    boolean rotate = SmartDashboard.getBoolean("SysIdRotate", false);
    m_drive.drivePercent((rotate ? -1 : 1) * speed, speed);
    
    double voltage = RobotController.getInputVoltage();
    SmartDashboard.putNumberArray("SysIdTelemetry", new double[] {
      Timer.getFPGATimestamp(),
      voltage,
      speed,
      speed * voltage, speed * voltage,
      m_drive.m_leftEncoder.getDistance(), m_drive.m_rightEncoder.getDistance(),
      m_drive.m_leftEncoder.getRate(), m_drive.m_rightEncoder.getRate(),
      m_drive.getGyro()
    });
  }

  @Override
  @SuppressWarnings("LocalVariableName")
  public void teleopPeriodic() {
    // Get the x speed. We are inverting this because Xbox controllers return
    // negative values when we push forward.
    double xSpeed =
        -m_speedLimiter.calculate(m_controller.getY(GenericHID.Hand.kLeft)) * Drivetrain.kMaxSpeed;

    // Get the rate of angular rotation. We are inverting this because we want a
    // positive value when we pull to the left (remember, CCW is positive in
    // mathematics). Xbox controllers return positive values when you pull to
    // the right by default.
    double rot =
        -m_rotLimiter.calculate(m_controller.getX(GenericHID.Hand.kRight))
            * Drivetrain.kMaxAngularSpeed;
    m_drive.drive(xSpeed, rot);
  }

  @Override
  public void disabledInit() {
    m_drive.drivePercent(0, 0);
  }

  @Override
  public void simulationPeriodic() {
    m_drive.simulationPeriodic();
    boolean enable = SmartDashboard.getBoolean("SysIdRun", false);
    if (enable) {
      DriverStationSim.setAutonomous(true);
      DriverStationSim.setEnabled(true);
    } else {
      DriverStationSim.setEnabled(false);
    }
    
    if (SmartDashboard.getBoolean("SysIdKill", false)) {
      System.exit(0);
    }
  }
}

