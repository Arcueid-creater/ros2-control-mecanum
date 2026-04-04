/*
 * @Author: qinghuan 1484237245@qq.com
 * @Date: 2025-02-08 11:29:40
 * @FilePath: /TideControls/src/tide_control/tide_hw_interface/src/tide_hw_interface.cpp
 * @Description:
 *
 * Copyright (c) 2025 by qinghuan, All Rights Reserved.
 */
#include "tide_hw_interface.hpp"
#include <chrono>
#include <cmath>
#include <limits>
#include <vector>
#include <string>
#include <set>
#include <iomanip>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>

namespace tide_hw_interface
{

static const uint16_t CRC16_TABLE[256] = {
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50A5,0x60C6,0x70E7,0x8108,0x9129,0xA14A,0xB16B,0xC18C,0xD1AD,0xE1CE,0xF1EF,
    0x1231,0x0210,0x3273,0x2252,0x52B5,0x4294,0x72F7,0x62D6,0x9339,0x8318,0xB37B,0xA35A,0xD3BD,0xC39C,0xF3FF,0xE3DE,
    0x2462,0x3443,0x0420,0x1401,0x64E6,0x74C7,0x44A4,0x5485,0xA56A,0xB54B,0x8528,0x9509,0xE5EE,0xF5CF,0xC5AC,0xD58D,
    0x3653,0x2672,0x1611,0x0630,0x76D7,0x66F6,0x5695,0x46B4,0xB75B,0xA77A,0x9719,0x8738,0xF7DF,0xE7FE,0xD79D,0xC7BC,
    0x48C4,0x58E5,0x6886,0x78A7,0x0840,0x1861,0x2802,0x3823,0xC9CC,0xD9ED,0xE98E,0xF9AF,0x8948,0x9969,0xA90A,0xB92B,
    0x5AF5,0x4AD4,0x7AB7,0x6A96,0x1A71,0x0A50,0x3A33,0x2A12,0xDBFD,0xCBDC,0xFBBF,0xEB9E,0x9B79,0x8B58,0xBB3B,0xAB1A,
    0x6CA6,0x7C87,0x4CE4,0x5CC5,0x2C22,0x3C03,0x0C60,0x1C41,0xEDAE,0xFD8F,0xCDEC,0xDDCD,0xAD2A,0xBD0B,0x8D68,0x9D49,
    0x7E97,0x6EB6,0x5ED5,0x4EF4,0x3E13,0x2E32,0x1E51,0x0E70,0xFF9F,0xEFBE,0xDFDD,0xCFFC,0xBF1B,0xAF3A,0x9F59,0x8F78,
    0x9188,0x81A9,0xB1CA,0xA1EB,0xD10C,0xC12D,0xF14E,0xE16F,0x1080,0x00A1,0x30C2,0x20E3,0x5004,0x4025,0x7046,0x6067,
    0x83B9,0x9398,0xA3FB,0xB3DA,0xC33D,0xD31C,0xE37F,0xF35E,0x02B1,0x1290,0x22F3,0x32D2,0x4235,0x5214,0x6277,0x7256,
    0xB5EA,0xA5CB,0x95A8,0x8589,0xF56E,0xE54F,0xD52C,0xC50D,0x34E2,0x24C3,0x14A0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xA7DB,0xB7FA,0x8799,0x97B8,0xE75F,0xF77E,0xC71D,0xD73C,0x26D3,0x36F2,0x0691,0x16B0,0x6657,0x7676,0x4615,0x5634,
    0xD94C,0xC96D,0xF90E,0xE92F,0x99C8,0x89E9,0xB98A,0xA9AB,0x5844,0x4865,0x7806,0x6827,0x18C0,0x08E1,0x3882,0x28A3,
    0xCB7D,0xDB5C,0xEB3F,0xFB1E,0x8BF9,0x9BD8,0xABBB,0xBB9A,0x4A75,0x5A54,0x6A37,0x7A16,0x0AF1,0x1AD0,0x2AB3,0x3A92,
    0xFD2E,0xED0F,0xDD6C,0xCD4D,0xBDAA,0xAD8B,0x9DE8,0x8DC9,0x7C26,0x6C07,0x5C64,0x4C45,0x3CA2,0x2C83,0x1CE0,0x0CC1,
    0xEF1F,0xFF3E,0xCF5D,0xDF7C,0xAF9B,0xBFBA,0x8FD9,0x9FF8,0x6E17,0x7E36,0x4E55,0x5E74,0x2E93,0x3EB2,0x0ED1,0x1EF0
};

uint16_t TideHardwareInterface::calculateCRC16(const uint8_t* data, size_t len)
{
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++)
  {
    uint8_t index = ((crc >> 8) ^ data[i]) & 0xFF;
    // NOTE: Matching the Python dm_imu implementation which uses << 1
    // Standard CRC-16 table-driven would use << 8, but we must match the device/Python logic
    crc = ((crc << 1) ^ CRC16_TABLE[index]) & 0xFFFF;
  }
  return crc;
}

SerialDevice::SerialDevice(const std::string& port_name, uint32_t baud_rate, ReceiveCallback callback)
  : port_name_(port_name), baud_rate_(baud_rate), receive_callback_(std::move(callback))
{
  fd_ = open(port_name_.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
  if (fd_ < 0)
  {
    throw std::runtime_error("Failed to open serial port: " + port_name_);
  }

  struct termios tty;
  if (tcgetattr(fd_, &tty) != 0)
  {
    throw std::runtime_error("Failed to get serial attributes");
  }

  speed_t speed;
  switch (baud_rate_)
  {
    case 115200: speed = B115200; break;
    case 921600: speed = B921600; break;
    default: speed = B115200; break;
  }

  cfsetospeed(&tty, speed);
  cfsetispeed(&tty, speed);

  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
  tty.c_iflag &= ~IGNBRK;
  tty.c_lflag = 0;
  tty.c_oflag = 0;
  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 5;

  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_cflag |= (CLOCAL | CREAD);
  tty.c_cflag &= ~(PARENB | PARODD);
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr(fd_, TCSANOW, &tty) != 0)
  {
    throw std::runtime_error("Failed to set serial attributes");
  }

  running_ = true;
  thread_ = std::thread(&SerialDevice::run, this);
}

SerialDevice::~SerialDevice()
{
  running_ = false;
  write_cv_.notify_all();
  if (thread_.joinable())
  {
    thread_.join();
  }
  if (fd_ >= 0)
  {
    close(fd_);
  }
}

void SerialDevice::write(const std::vector<uint8_t>& data)
{
  std::lock_guard<std::mutex> lock(write_mutex_);
  write_queue_.push(data);
  write_cv_.notify_one();
}

void SerialDevice::run()
{
  std::vector<uint8_t> rx_buffer(1024);
  struct pollfd fds[1];
  fds[0].fd = fd_;
  fds[0].events = POLLIN;

  while (running_ && rclcpp::ok())
  {
    // Check for data to write
    {
      std::unique_lock<std::mutex> lock(write_mutex_);
      if (write_cv_.wait_for(lock, std::chrono::milliseconds(1), [this] { return !write_queue_.empty() || !running_; }))
      {
        if (!running_) break;
        while (!write_queue_.empty())
        {
          // const auto& data = write_queue_.front();
          // ::write(fd_, data.data(), data.size());
          // write_queue_.pop();
        }
      }
    }

    // Check for data to read
    int ret = poll(fds, 1, 10);
    if (ret > 0 && (fds[0].revents & POLLIN))
    {
      ssize_t n = ::read(fd_, rx_buffer.data(), rx_buffer.size());
      if (n > 0)
      {
        std::vector<uint8_t> data(rx_buffer.begin(), rx_buffer.begin() + n);
        receive_callback_(data);
      }
    }
  }
}

TideHardwareInterface::TideHardwareInterface() : hardware_interface::SystemInterface() {}

hardware_interface::CallbackReturn
TideHardwareInterface::on_init(const hardware_interface::HardwareInfo& info)
{
  if (hardware_interface::SystemInterface::on_init(info) !=
      hardware_interface::CallbackReturn::SUCCESS)
  {
    return hardware_interface::CallbackReturn::ERROR;
  }

  joint_count = info_.joints.size();
  state_positions_.resize(joint_count, std::numeric_limits<double>::quiet_NaN());
  state_velocities_.resize(joint_count, std::numeric_limits<double>::quiet_NaN());
  state_currents_.resize(joint_count, std::numeric_limits<double>::quiet_NaN());
  state_temperatures_.resize(joint_count, std::numeric_limits<double>::quiet_NaN());

  cmd_positions_.resize(joint_count, std::numeric_limits<double>::quiet_NaN());
  cmd_velocities_.resize(joint_count, std::numeric_limits<double>::quiet_NaN());

  enable_virtual_control_ = info_.hardware_parameters.at("enable_virtual_control") == "true";
  need_calibration_ = info_.hardware_parameters.at("need_calibration") == "true";
  serial_port_ = info_.hardware_parameters.at("serial_port");
  baud_rate_ = std::stoi(info_.hardware_parameters.at("baud_rate"));

  // IMU Parameters from URDF/ros2_control tag
  if (info_.hardware_parameters.find("imu_serial_port") != info_.hardware_parameters.end()) {
    imu_serial_port_ = info_.hardware_parameters.at("imu_serial_port");
  } else {
    imu_serial_port_ = serial_port_; // Default to same port if not specified
  }
  
  if (info_.hardware_parameters.find("imu_baud_rate") != info_.hardware_parameters.end()) {
    imu_baud_rate_ = std::stoi(info_.hardware_parameters.at("imu_baud_rate"));
  } else {
    imu_baud_rate_ = baud_rate_;
  }

  if (info_.hardware_parameters.find("publish_imu_data") != info_.hardware_parameters.end()) {
    publish_imu_data_ = info_.hardware_parameters.at("publish_imu_data") == "true";
  }
  if (info_.hardware_parameters.find("publish_rpy") != info_.hardware_parameters.end()) {
    publish_rpy_ = info_.hardware_parameters.at("publish_rpy") == "true";
  }
  if (info_.hardware_parameters.find("imu_frame_id") != info_.hardware_parameters.end()) {
    imu_frame_id_ = info_.hardware_parameters.at("imu_frame_id");
  }

  nh_ = std::make_shared<rclcpp::Node>("tide_hw_imu_node");
  if (publish_imu_data_) {
    imu_pub_ = nh_->create_publisher<sensor_msgs::msg::Imu>("imu/data", 10);
  }
  if (publish_rpy_) {
    rpy_pub_ = nh_->create_publisher<geometry_msgs::msg::Vector3Stamped>("imu/rpy", 10);
  }

  // Start IMU node spinning in a separate thread
  imu_spin_thread_ = std::thread([this]() {
    rclcpp::spin(nh_);
  });

  for (const auto& joint : info_.joints)
  {
    Motor_Config_t config;
    config.motor_name = joint.name;

    for (const auto& [key, value] : joint.parameters)
    {
      if (key == "can_bus")
        config.can_bus = value;
      else if (key == "tx_id")
      {
        config.tx_id = std::stoi(value);
      }
      else if (key == "rx_id")
      {
        config.rx_id = std::stoi(value);
      }
      else if (key == "motor_type")
      {
        if (value == "M2006")
        {
          config.motor_type = M2006;
        }
        else if (value == "M3508")
        {
          config.motor_type = M3508;
        }
        else if (value == "GM6020")
        {
          config.motor_type = GM6020;
        }
        else if (value == "VIRTUAL_JOINT")
        {
          config.motor_type = VIRTUAL_JOINT;
        }
        else
        {
          RCLCPP_ERROR(rclcpp::get_logger("TideHardwareInterface"), "Unknown motor type: %s",
                       value.c_str());
        }
      }
      else if (key == "offset")
        config.offset = std::stoi(value);
    }

    auto motor = std::make_shared<DJI_Motor>(config);
    // configureMotorCan(motor); // Removed CAN configuration
    motors_.push_back(motor);
  }

  // NOTE: Even if virtual control is enabled, we still want to read IMU data
  // if real IMU hardware is connected.
  // if (enable_virtual_control_)
  // {
  //   return hardware_interface::CallbackReturn::SUCCESS;
  // }

  try
  {
    // Initialize separate serial devices
    if (imu_serial_port_ != serial_port_) {
      // 1. Independent Ports (Recommended): Different threads, different callbacks
      auto motor_receive_callback = [this](const std::vector<uint8_t>& data) {
        this->parseMotorData(data);
      };
      serial_device_ = std::make_shared<SerialDevice>(serial_port_, baud_rate_, motor_receive_callback);

      auto imu_receive_callback = [this](const std::vector<uint8_t>& data) {
        this->parseImuSerialData(data);
      };
      imu_serial_device_ = std::make_shared<SerialDevice>(imu_serial_port_, imu_baud_rate_, imu_receive_callback);
      // RCLCPP_INFO(rclcpp::get_logger("TideHardwareInterface"), "Independent ports: Motor (%s), IMU (%s)", serial_port_.c_str(), imu_serial_port_.c_str());
    } else {
      // 2. Shared Port: One thread, one callback handles both
      auto shared_receive_callback = [this](const std::vector<uint8_t>& data) {
        this->parseSharedData(data);
      };
      serial_device_ = std::make_shared<SerialDevice>(serial_port_, baud_rate_, shared_receive_callback);
      imu_serial_device_ = nullptr; // Explicitly null to indicate sharing
      // RCLCPP_INFO(rclcpp::get_logger("TideHardwareInterface"), "Shared port: Motor and IMU on %s", serial_port_.c_str());
    }
  }
  catch (const std::exception& e)
  {
    // RCLCPP_ERROR(rclcpp::get_logger("TideHardwareInterface"), "Failed to initialize serial device: %s", e.what());
    return hardware_interface::CallbackReturn::ERROR;
  }

  // RCLCPP_INFO(rclcpp::get_logger("TideHardwareInterface"),
              // "Successful loaded %ld motors and serial port %s", joint_count, serial_port_.c_str());

  return hardware_interface::CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> TideHardwareInterface::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> interfaces;

  for (size_t i = 0; i < joint_count; i++)
  {
    for (const auto& state_interface : info_.joints[i].state_interfaces)
    {
      if (state_interface.name == "position")
      {
        interfaces.emplace_back(info_.joints[i].name, state_interface.name, &state_positions_[i]);
      }
      else if (state_interface.name == "velocity")
      {
        interfaces.emplace_back(info_.joints[i].name, state_interface.name, &state_velocities_[i]);
      }
      else if (state_interface.name == "effort")
      {
        interfaces.emplace_back(info_.joints[i].name, state_interface.name, &state_currents_[i]);
      }
    }
  }
  return interfaces;
}

std::vector<hardware_interface::CommandInterface> TideHardwareInterface::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> interfaces;

  for (size_t i = 0; i < joint_count; i++)
  {
    for (const auto& command_interface : info_.joints[i].command_interfaces)
    {
      if (command_interface.name == "position")
      {
        interfaces.emplace_back(info_.joints[i].name, command_interface.name, &cmd_positions_[i]);
      }
      else if (command_interface.name == "velocity")
      {
        interfaces.emplace_back(info_.joints[i].name, command_interface.name, &cmd_velocities_[i]);
      }
    }
  }
  return interfaces;
}

hardware_interface::CallbackReturn
TideHardwareInterface::on_configure(const rclcpp_lifecycle::State& /*previous_state*/)
{
  std::fill(state_positions_.begin(), state_positions_.end(), 0.0);
  std::fill(state_velocities_.begin(), state_velocities_.end(), 0.0);
  std::fill(state_currents_.begin(), state_currents_.end(), 0.0);
  std::fill(state_temperatures_.begin(), state_temperatures_.end(), 0.0);

  std::fill(cmd_positions_.begin(), cmd_positions_.end(), 0.0);
  std::fill(cmd_velocities_.begin(), cmd_velocities_.end(), 0.0);

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn
TideHardwareInterface::on_activate(const rclcpp_lifecycle::State& /*previous_state*/)
{
  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn
TideHardwareInterface::on_deactivate(const rclcpp_lifecycle::State& /*previous_state*/)
{
  try
  {
    stopMotors();
    return hardware_interface::CallbackReturn::SUCCESS;
  }
  catch (const std::exception& e)
  {
    RCLCPP_ERROR(rclcpp::get_logger("TideHardwareInterface"), "Error in on_deactivate: %s",
                 e.what());
    return hardware_interface::CallbackReturn::ERROR;
  }
}

hardware_interface::CallbackReturn
TideHardwareInterface::on_cleanup(const rclcpp_lifecycle::State& /*previous_state*/)
{
  try
  {
    stop_thread_ = true;
    if (imu_spin_thread_.joinable())
    {
      // Since we don't want to call rclcpp::shutdown() which affects the whole process,
      // we'll just let the thread be joined. 
      // In many cases, the node will be destroyed and rclcpp::spin will return.
      // imu_spin_thread_.join(); 
      // Actually, joining a spinning thread without shutdown is tricky.
      // We'll detach it or just not join it if we can't stop it cleanly.
      imu_spin_thread_.detach();
    }
    serial_device_.reset();
    imu_serial_device_.reset();
    motors_.clear();
    return hardware_interface::CallbackReturn::SUCCESS;
  }
  catch (const std::exception& e)
  {
    RCLCPP_ERROR(rclcpp::get_logger("TideHardwareInterface"), "Error in on_cleanup: %s", e.what());
    return hardware_interface::CallbackReturn::ERROR;
  }
}

void TideHardwareInterface::parseMotorData(const std::vector<uint8_t>& data)
{
  motor_rx_buffer_.insert(motor_rx_buffer_.end(), data.begin(), data.end());

  while (motor_rx_buffer_.size() > 0)
  {
    if (motor_rx_buffer_[0] == 0x5A) // Motor feedback ONLY
    {
      if (motor_rx_buffer_.size() < sizeof(FeedbackPacket)) break;
      FeedbackPacket packet;
      std::memcpy(&packet, motor_rx_buffer_.data(), sizeof(FeedbackPacket));
      
      rclcpp::Time current_time = rclcpp::Clock().now();
      for (auto& motor : motors_)
      {
        if (motor->config_.tx_id == packet.motor_id)
        {
          motor->status = MOTOR_OK;
          motor->measure.ecd = packet.ecd;
          motor->measure.real_current = packet.given_current;
          motor->measure.temperature = packet.temperature;
          
          motor->decode_feedback();
          motor->update_timestamp(current_time);
          break;
        }
      }
      motor_rx_buffer_.erase(motor_rx_buffer_.begin(), motor_rx_buffer_.begin() + sizeof(FeedbackPacket));
    }
    else
    {
      // Not a motor packet, discard one byte
      motor_rx_buffer_.erase(motor_rx_buffer_.begin());
    }
  }
}

void TideHardwareInterface::parseSharedData(const std::vector<uint8_t>& data)
{
  // When sharing a port, we route data to both buffers but with strict header checks
  motor_rx_buffer_.insert(motor_rx_buffer_.end(), data.begin(), data.end());
  imu_rx_buffer_.insert(imu_rx_buffer_.end(), data.begin(), data.end());

  // 1. Process Motor Data from shared buffer
  while (motor_rx_buffer_.size() > 0)
  {
    if (motor_rx_buffer_[0] == 0x5A) {
      if (motor_rx_buffer_.size() < sizeof(FeedbackPacket)) break;
      FeedbackPacket packet;
      std::memcpy(&packet, motor_rx_buffer_.data(), sizeof(FeedbackPacket));
      for (auto& motor : motors_) {
        if (motor->config_.tx_id == packet.motor_id) {
          motor->status = MOTOR_OK;
          motor->measure.ecd = packet.ecd;
          motor->measure.real_current = packet.given_current;
          motor->measure.temperature = packet.temperature;
          motor->decode_feedback();
          motor->update_timestamp(rclcpp::Clock().now());
          break;
        }
      }
      motor_rx_buffer_.erase(motor_rx_buffer_.begin(), motor_rx_buffer_.begin() + sizeof(FeedbackPacket));
    } else {
      motor_rx_buffer_.erase(motor_rx_buffer_.begin());
    }
  }

  // 2. Process IMU Data from shared buffer
  while (imu_rx_buffer_.size() >= sizeof(ImuPacket))
  {
    if (imu_rx_buffer_[0] == 0x55 && imu_rx_buffer_[1] == 0xAA) {
      ImuPacket packet;
      std::memcpy(&packet, imu_rx_buffer_.data(), sizeof(ImuPacket));
      if (packet.tail == 0x0A) {
        uint16_t crc_calc = calculateCRC16(imu_rx_buffer_.data(), 16);
        if (crc_calc == packet.crc || calculateCRC16(imu_rx_buffer_.data() + 2, 14) == packet.crc) {
          parseImuData(imu_rx_buffer_.data());
        }
      }
      imu_rx_buffer_.erase(imu_rx_buffer_.begin(), imu_rx_buffer_.begin() + sizeof(ImuPacket));
    } else {
      imu_rx_buffer_.erase(imu_rx_buffer_.begin());
    }
  }
}

void TideHardwareInterface::parseImuSerialData(const std::vector<uint8_t>& data)
{
  imu_rx_buffer_.insert(imu_rx_buffer_.end(), data.begin(), data.end());

  while (imu_rx_buffer_.size() >= sizeof(ImuPacket))
  {
    if (imu_rx_buffer_[0] == 0x55 && imu_rx_buffer_[1] == 0xAA)
    {
      ImuPacket packet;
      std::memcpy(&packet, imu_rx_buffer_.data(), sizeof(ImuPacket));
      
      if (packet.tail == 0x0A)
      {
        uint16_t crc_calc = calculateCRC16(imu_rx_buffer_.data(), 16);
        if (crc_calc == packet.crc)
        {
          parseImuData(imu_rx_buffer_.data());
        }
        else
        {
          // Try without header (skip 0x55 0xAA)
          uint16_t crc_calc_no_hdr = calculateCRC16(imu_rx_buffer_.data() + 2, 14);
          if (crc_calc_no_hdr == packet.crc)
          {
            parseImuData(imu_rx_buffer_.data());
          }
        }
      }
      imu_rx_buffer_.erase(imu_rx_buffer_.begin(), imu_rx_buffer_.begin() + sizeof(ImuPacket));
    }
    else
    {
      imu_rx_buffer_.erase(imu_rx_buffer_.begin());
    }
  }
}

void TideHardwareInterface::parseImuData(const uint8_t* frame)
{
  ImuPacket packet;
  std::memcpy(&packet, frame, sizeof(ImuPacket));

  if (packet.rid == 0x03) // RPY
  {
    imu_rpy_[0] = packet.data[0] * M_PI / 180.0;
    imu_rpy_[1] = packet.data[1] * M_PI / 180.0;
    imu_rpy_[2] = packet.data[2] * M_PI / 180.0;

    // Publish RPY
    if (publish_rpy_)
    {
      auto rpy_msg = geometry_msgs::msg::Vector3Stamped();
      rpy_msg.header.stamp = nh_->now();
      rpy_msg.header.frame_id = imu_frame_id_;
      rpy_msg.vector.x = packet.data[0]; // degrees
      rpy_msg.vector.y = packet.data[1];
      rpy_msg.vector.z = packet.data[2];
      rpy_pub_->publish(rpy_msg);
    }
  }
  else if (packet.rid == 0x02) // Gyro
  {
    imu_gyro_[0] = packet.data[0] * M_PI / 180.0;
    imu_gyro_[1] = packet.data[1] * M_PI / 180.0;
    imu_gyro_[2] = packet.data[2] * M_PI / 180.0;
  }
  else if (packet.rid == 0x01) // Accel
  
  {
    imu_accel_[0] = packet.data[0]; // Assuming m/s^2
    imu_accel_[1] = packet.data[1];
    imu_accel_[2] = packet.data[2];
  }
  else if (packet.rid == 0x04)
  {
    /* code */
    imu_orientation[0] = packet.data[0];
    imu_orientation[1] = packet.data[1];
    imu_orientation[2] = packet.data[2];
    imu_orientation[3] = packet.data[3];
  }
  
  // Publish IMU Data whenever any packet arrives
  if (publish_imu_data_)
  {
    auto imu_msg = sensor_msgs::msg::Imu();
    imu_msg.header.stamp = nh_->now();
    imu_msg.header.frame_id = imu_frame_id_;

    // Orientation (Euler to Quaternion ZYX)
    // double cy = cos(imu_rpy_[2] * 0.5);
    // double sy = sin(imu_rpy_[2] * 0.5);
    // double cp = cos(imu_rpy_[1] * 0.5);
    // double sp = sin(imu_rpy_[1] * 0.5);
    // double cr = cos(imu_rpy_[0] * 0.5);
    // double sr = sin(imu_rpy_[0] * 0.5);

    imu_msg.orientation.w = imu_orientation[0];
    imu_msg.orientation.x = imu_orientation[1];
    imu_msg.orientation.y = imu_orientation[2];
    imu_msg.orientation.z = imu_orientation[3];

    // Angular Velocity
    imu_msg.angular_velocity.x = imu_gyro_[0];
    imu_msg.angular_velocity.y = imu_gyro_[1];
    imu_msg.angular_velocity.z = imu_gyro_[2];

    // Linear Acceleration
    imu_msg.linear_acceleration.x = imu_accel_[0];
    imu_msg.linear_acceleration.y = imu_accel_[1];
    imu_msg.linear_acceleration.z = imu_accel_[2];

    imu_pub_->publish(imu_msg);
  }
}

void TideHardwareInterface::stopMotors()
{
  for (auto& motor : motors_)
  {
    motor->stop();
    
    ControlPacket packet;
    packet.motor_id = static_cast<uint8_t>(motor->config_.tx_id);
    packet.command = 0;

    std::vector<uint8_t> tx_data(sizeof(ControlPacket));
    std::memcpy(tx_data.data(), &packet, sizeof(ControlPacket));

    if (serial_device_)
    {
      serial_device_->write(tx_data);
    }
  }
}

hardware_interface::return_type TideHardwareInterface::read(const rclcpp::Time& time,
                                                            const rclcpp::Duration& period)
{
  auto current_time = time;

  for (size_t i = 0; i < joint_count; i++)
  {
    auto& motor = motors_[i];
    if (enable_virtual_control_ || motor->config_.motor_type == VIRTUAL_JOINT)
    {
      if (!std::isnan(cmd_positions_[i]) &&
          info_.joints[i].command_interfaces[0].name == "position")
      {
        state_positions_[i] = cmd_positions_[i];
      }
      else if (!std::isnan(cmd_velocities_[i]) &&
               info_.joints[i].command_interfaces[0].name == "velocity")
      {
        state_positions_[i] += cmd_velocities_[i] * period.seconds();
        state_velocities_[i] = cmd_velocities_[i];
      }
    }
    else
    {
      motor->check_connection(current_time);

      state_positions_[i] = motor->angle_current;
      state_velocities_[i] = motor->measure.speed_aps;
      state_currents_[i] = motor->measure.real_current;
      state_temperatures_[i] = motor->measure.temperature;
    }
  }
  return hardware_interface::return_type::OK;
}

hardware_interface::return_type TideHardwareInterface::write(const rclcpp::Time& /*time*/,
                                                             const rclcpp::Duration& /*period*/)
{
  if (need_calibration_)
  {
    for (const auto& motor : motors_)
    {
      std::stringstream ss;
      ss << std::left << std::setw(15) << motor->config_.motor_name << "Encoder: " << std::setw(6)
         << motor->measure.ecd;
      // RCLCPP_INFO(rclcpp::get_logger("TideHardwareInterface"), "%s", ss.str().c_str());
    }
    return hardware_interface::return_type::OK;
  }

  if (enable_virtual_control_)
  {
    return hardware_interface::return_type::OK;
  }

  // Send control packets for each motor
  for (size_t i = 0; i < joint_count; i++)
  {
    double command = 0.0;
    if (!std::isnan(cmd_positions_[i]) && info_.joints[i].command_interfaces[0].name == "position")
    {
      command = cmd_positions_[i];
    }
    else if (!std::isnan(cmd_velocities_[i]) &&
             info_.joints[i].command_interfaces[0].name == "velocity")
    {
      command = cmd_velocities_[i];
    }

    auto motor = motors_[i];
    motor->output = static_cast<int16_t>(std::clamp(command, -30000.0, 30000.0));

    ControlPacket packet;
    packet.motor_id = static_cast<uint8_t>(motor->config_.tx_id);
    packet.command = motor->output;

    std::vector<uint8_t> tx_data(sizeof(ControlPacket));
    std::memcpy(tx_data.data(), &packet, sizeof(ControlPacket));

    // ALWAYS send to motor serial device, NEVER to IMU device
    if (serial_device_)
    {
      // serial_device_->write(tx_data);
    }
  }

  return hardware_interface::return_type::OK;
}

}  // namespace tide_hw_interface

#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(tide_hw_interface::TideHardwareInterface, hardware_interface::SystemInterface)
