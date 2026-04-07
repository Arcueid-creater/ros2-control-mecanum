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
#include <sstream>

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

// CRC32查找表
static const uint32_t CRC32_TABLE[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
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

uint32_t TideHardwareInterface::calculateCRC32(const uint8_t* data, size_t len)
{
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < len; i++)
  {
    uint8_t index = (crc ^ data[i]) & 0xFF;
    crc = (crc >> 8) ^ CRC32_TABLE[index];
  }
  return ~crc;
}

SerialDevice::SerialDevice(const std::string& port_name, uint32_t baud_rate, ReceiveCallback callback, TickCallback tick_callback)
  : port_name_(port_name), baud_rate_(baud_rate), receive_callback_(std::move(callback)), tick_callback_(std::move(tick_callback))
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
          const auto& data = write_queue_.front();
          ::write(fd_, data.data(), data.size());
          write_queue_.pop();
        }
      }
    }

    // Check for data to read
    int ret = poll(fds, 1, 5);
    if (ret > 0 && (fds[0].revents & POLLIN))
    {
      ssize_t n = ::read(fd_, rx_buffer.data(), rx_buffer.size());
      if (n > 0)
      {
        std::vector<uint8_t> data(rx_buffer.begin(), rx_buffer.begin() + n);
        receive_callback_(data);
      }
    }

    // New: Tick callback to reuse this thread for other tasks (e.g., spin ROS2 node)
    if (tick_callback_)
    {
      tick_callback_();
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

  // 方案三：保留节点用于发布RPY，但不需要额外线程
  // 恢复参数服务以支持 Foxglove 调试和其他线程访问
  rclcpp::NodeOptions node_options;
  node_options.start_parameter_services(true);
  node_options.start_parameter_event_publisher(true);
  nh_ = std::make_shared<rclcpp::Node>("tide_hw_imu_node", node_options);
  
  // 只保留RPY发布器（IMU数据由IMUSensorBroadcaster发布）
  if (publish_rpy_) {
    rpy_pub_ = nh_->create_publisher<geometry_msgs::msg::Vector3Stamped>("imu/rpy", 10);
  }

  rc_dbus_pub_ = nh_->create_publisher<tide_msgs::msg::RcData>("rc/dbus", 10);

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
    // 初始化独立的串口设备（电机和IMU必须使用不同的串口）
    auto motor_receive_callback = [this](const std::vector<uint8_t>& data) {
      this->parseMotorData(data);
    };
    serial_device_ = std::make_shared<SerialDevice>(serial_port_, baud_rate_, motor_receive_callback);

    auto imu_receive_callback = [this](const std::vector<uint8_t>& data) {
      this->parseImuSerialData(data);
    };
    // 复用 IMU 串口线程来 spin ROS2 节点，这样既不需要额外线程，也能支持参数服务和 Foxglove 调试
    auto imu_tick_callback = [this]() {
      if (rclcpp::ok()) {
        rclcpp::spin_some(this->nh_);
      }
    };
    imu_serial_device_ = std::make_shared<SerialDevice>(imu_serial_port_, imu_baud_rate_, imu_receive_callback, imu_tick_callback);
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

  // 导出关节状态接口
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
  
  // 导出IMU传感器状态接口
  for (const auto& sensor : info_.sensors)
  {
    for (const auto& state_interface : sensor.state_interfaces)
    {
      // Quaternion (四元数): W, X, Y, Z
      if (state_interface.name == "orientation.x")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_orientation[1]); // X
      else if (state_interface.name == "orientation.y")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_orientation[2]); // Y
      else if (state_interface.name == "orientation.z")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_orientation[3]); // Z
      else if (state_interface.name == "orientation.w")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_orientation[0]); // W
      
      // Euler angles (欧拉角，弧度)
      else if (state_interface.name == "rpy.roll")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_rpy_[0]);
      else if (state_interface.name == "rpy.pitch")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_rpy_[1]);
      else if (state_interface.name == "rpy.yaw")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_rpy_[2]);
      
      // Angular velocity (角速度，rad/s)
      else if (state_interface.name == "angular_velocity.x")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_gyro_[0]);
      else if (state_interface.name == "angular_velocity.y")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_gyro_[1]);
      else if (state_interface.name == "angular_velocity.z")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_gyro_[2]);
      
      // Linear acceleration (线性加速度，m/s²)
      else if (state_interface.name == "linear_acceleration.x")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_accel_[0]);
      else if (state_interface.name == "linear_acceleration.y")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_accel_[1]);
      else if (state_interface.name == "linear_acceleration.z")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_accel_[2]);
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
    // 方案三：不需要清理spin线程（已删除）
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

  // 查找帧头 0xFF 0xAA
  while (motor_rx_buffer_.size() >= 8) // 最小包：帧头(2) + 长度(2) + CRC32(4) = 8字节
  {
    if (motor_rx_buffer_[0] == 0xFF && motor_rx_buffer_[1] == 0xAA)
    {
      // 解析数据长度
      uint16_t data_length;
      std::memcpy(&data_length, motor_rx_buffer_.data() + 2, sizeof(uint16_t));
      
      // 计算完整包大小：帧头(2) + 长度字段(2) + 数据(data_length) + CRC32(4)
      size_t total_packet_size = 4 + data_length + 4;
      
      // 检查是否接收到完整的包
      if (motor_rx_buffer_.size() < total_packet_size)
      {
        break; // 数据不足，等待更多数据
      }
      
      // 验证CRC32（计算范围：从帧头到数据结束，不包括CRC32本身）
      size_t crc_offset = total_packet_size - 4;
      uint32_t received_crc;
      std::memcpy(&received_crc, motor_rx_buffer_.data() + crc_offset, sizeof(uint32_t));
      
      uint32_t calculated_crc = calculateCRC32(motor_rx_buffer_.data(), crc_offset);
      
      if (received_crc == calculated_crc)
      {
        // CRC校验通过，提取有效数据（不含帧头、长度字段和CRC32）
        motor_data_buffer_.clear();
        motor_data_buffer_.insert(motor_data_buffer_.end(),
                                  motor_rx_buffer_.begin() + 4,  // 跳过帧头(2)和长度(2)
                                  motor_rx_buffer_.begin() + crc_offset); // 到CRC32之前

        // ====== 协议分发：根据 msg_id 解析不同数据 ======
        // 下位机遥控器DBUS上发格式：
        // [msg_id=0x10]
        // [ch1][ch2][ch3][ch4] (int16)
        // [sw1][sw2] (uint8)
        // [mouse.x][mouse.y] (int16)
        // [mouse.l][mouse.r] (uint8)
        // [kb.key_code] (uint16)
        // [wheel] (int16)
        if (motor_data_buffer_.size() >= 1 && motor_data_buffer_[0] == 0x10)
        {
          // payload length should be 19 bytes for RC_DBUS
          if (motor_data_buffer_.size() >= 19)
          {
            size_t off = 1;

            int16_t ch1, ch2, ch3, ch4;
            uint8_t sw1, sw2;
            int16_t mouse_x, mouse_y;
            uint8_t mouse_l, mouse_r;
            uint16_t key_code;
            int16_t wheel;

            std::memcpy(&ch1, motor_data_buffer_.data() + off, sizeof(int16_t)); off += sizeof(int16_t);
            std::memcpy(&ch2, motor_data_buffer_.data() + off, sizeof(int16_t)); off += sizeof(int16_t);
            std::memcpy(&ch3, motor_data_buffer_.data() + off, sizeof(int16_t)); off += sizeof(int16_t);
            std::memcpy(&ch4, motor_data_buffer_.data() + off, sizeof(int16_t)); off += sizeof(int16_t);

            sw1 = motor_data_buffer_[off++];
            sw2 = motor_data_buffer_[off++];

            std::memcpy(&mouse_x, motor_data_buffer_.data() + off, sizeof(int16_t)); off += sizeof(int16_t);
            std::memcpy(&mouse_y, motor_data_buffer_.data() + off, sizeof(int16_t)); off += sizeof(int16_t);
            mouse_l = motor_data_buffer_[off++];
            mouse_r = motor_data_buffer_[off++];

            std::memcpy(&key_code, motor_data_buffer_.data() + off, sizeof(uint16_t)); off += sizeof(uint16_t);
            std::memcpy(&wheel, motor_data_buffer_.data() + off, sizeof(int16_t)); off += sizeof(int16_t);

            if (rc_dbus_pub_)
            {
              tide_msgs::msg::RcData msg;
              msg.ch1 = ch1;
              msg.ch2 = ch2;
              msg.ch3 = ch3;
              msg.ch4 = ch4;
              msg.sw1 = sw1;
              msg.sw2 = sw2;
              msg.mouse_x = mouse_x;
              msg.mouse_y = mouse_y;
              msg.mouse_l = mouse_l;
              msg.mouse_r = mouse_r;
              msg.key_code = key_code;
              msg.wheel = wheel;
              rc_dbus_pub_->publish(msg);
            }
          }

          // 移除已处理的数据包
          motor_rx_buffer_.erase(motor_rx_buffer_.begin(), motor_rx_buffer_.begin() + total_packet_size);
          continue;
        }
        
        // 打印纯净数据（十六进制格式）- 使用限流避免刷屏
        if (!motor_data_buffer_.empty())
        {
          std::stringstream ss;
          ss << "Motor data [" << motor_data_buffer_.size() << " bytes]: ";
          for (size_t i = 0; i < motor_data_buffer_.size(); ++i)
          {
            ss << std::hex << std::setfill('0') << std::setw(2) 
               << static_cast<int>(motor_data_buffer_[i]) << " ";
            
            // 每16字节换行，方便查看
            if ((i + 1) % 16 == 0 && i + 1 < motor_data_buffer_.size())
            {
              ss << "\n                                      ";
            }
          }
          // 使用THROTTLE限流，每1000ms最多打印一次
          // RCLCPP_INFO(rclcpp::get_logger("TideHardwareInterface"), "%s", ss.str().c_str());
        }
        
        // 移除已处理的数据包
        // 打印纯净数据（十六进制格式）
        // if (!motor_data_buffer_.empty())
        // {
        //   std::stringstream ss;
        //   ss << "Motor data [" << motor_data_buffer_.size() << " bytes]: ";
        //   for (size_t i = 0; i < motor_data_buffer_.size(); ++i)
        //   {
        //     ss << std::hex << std::setfill('0') << std::setw(2) 
        //        << static_cast<int>(motor_data_buffer_[i]) << " ";
            
        //     // 每16字节换行，方便查看
        //     if ((i + 1) % 16 == 0 && i + 1 < motor_data_buffer_.size())
        //     {
        //       ss << "\n                                      ";
        //     }
        //   }
        //   RCLCPP_INFO(rclcpp::get_logger("TideHardwareInterface"), "%s", ss.str().c_str());
        // }
        
        // 解析电机数据（每个电机17字节：ID(1) + position(4) + velocity(4) + current(4) + temperature(4)）
        const size_t motor_data_size = 17; // 每个电机数据包大小
        size_t offset = 0;
        
        while (offset + motor_data_size <= motor_data_buffer_.size())
        {
          // 读取电机ID
          uint8_t motor_id = motor_data_buffer_[offset++];
          
          // 读取电机数据
          float position, velocity, current, temperature;
          std::memcpy(&position, motor_data_buffer_.data() + offset, sizeof(float));
          offset += sizeof(float);
          std::memcpy(&velocity, motor_data_buffer_.data() + offset, sizeof(float));
          offset += sizeof(float);
          std::memcpy(&current, motor_data_buffer_.data() + offset, sizeof(float));
          offset += sizeof(float);
          std::memcpy(&temperature, motor_data_buffer_.data() + offset, sizeof(float));
          offset += sizeof(float);
          
          // 查找对应的电机并更新状态
          for (size_t i = 0; i < motors_.size(); ++i)
          {
            if (motors_[i]->config_.tx_id == motor_id)
            {
              // 更新电机状态
              motors_[i]->angle_current = position;
              motors_[i]->measure.speed_aps = velocity;
              motors_[i]->measure.real_current = static_cast<int16_t>(current);
              motors_[i]->measure.temperature = static_cast<uint8_t>(temperature);
              
              // 更新状态接口变量（供控制器读取）- 使用互斥锁保护
              {
                std::lock_guard<std::mutex> lock(state_mutex_);
                state_positions_[i] = position;
                state_velocities_[i] = velocity;
                state_currents_[i] = current;
                state_temperatures_[i] = temperature;
              }
              
              // 更新通信时间戳
              motors_[i]->update_timestamp(rclcpp::Clock().now());
              motors_[i]->status = MOTOR_OK;
              
              break;
            }
          }
        }

        // 移除已处理的数据包
        motor_rx_buffer_.erase(motor_rx_buffer_.begin(), motor_rx_buffer_.begin() + total_packet_size);
      }
      else
      {
        // CRC校验失败，丢弃帧头，继续查找下一个有效包
        // RCLCPP_WARN_THROTTLE(rclcpp::get_logger("TideHardwareInterface"), 
        //                     rclcpp::Clock(),1000,
        //                     "Motor CRC32 mismatch: received=0x%08X, calculated=0x%08X", 
        //                     received_crc, calculated_crc);
        motor_rx_buffer_.erase(motor_rx_buffer_.begin(), motor_rx_buffer_.begin() + 2);
      }
    }
    else
    {
      // 不是有效帧头，丢弃一个字节
      motor_rx_buffer_.erase(motor_rx_buffer_.begin());
    }
  }
}

void TideHardwareInterface::parseImuSerialData(const std::vector<uint8_t>& data)
{
  imu_rx_buffer_.insert(imu_rx_buffer_.end(), data.begin(), data.end());

  while (imu_rx_buffer_.size() >= 4) // At least Header(2), ID(1), rid(1)
  {
    if (imu_rx_buffer_[0] == 0x55 && imu_rx_buffer_[1] == 0xAA)
    {
      uint8_t rid = imu_rx_buffer_[3];
      size_t packet_len = 0;
      size_t crc_data_len = 0;

      if (rid >= 0x01 && rid <= 0x03) // Accel, Gyro, RPY
      {
        packet_len = 19;
        crc_data_len = 16;
      }
      else if (rid == 0x04) // Quaternion
      {
        packet_len = 23;
        crc_data_len = 20;
      }
      else
      {
        // Invalid rid, drop header and continue
        imu_rx_buffer_.erase(imu_rx_buffer_.begin(), imu_rx_buffer_.begin() + 2);
        continue;
      }

      if (imu_rx_buffer_.size() < packet_len)
      {
        break; // Wait for more data
      }

      // Check tail
      if (imu_rx_buffer_[packet_len - 1] == 0x0A)
      {
        uint16_t received_crc;
        std::memcpy(&received_crc, imu_rx_buffer_.data() + packet_len - 3, sizeof(uint16_t));

        uint16_t crc_calc = calculateCRC16(imu_rx_buffer_.data(), crc_data_len);
        if (crc_calc == received_crc)
        {
          parseImuData(imu_rx_buffer_.data());
        }
        else
        {
          // Try without header (skip 0x55 0xAA)
          uint16_t crc_calc_no_hdr = calculateCRC16(imu_rx_buffer_.data() + 2, crc_data_len - 2);
          if (crc_calc_no_hdr == received_crc)
          {
            parseImuData(imu_rx_buffer_.data());
          }
        }
      }
      imu_rx_buffer_.erase(imu_rx_buffer_.begin(), imu_rx_buffer_.begin() + packet_len);
    }
    else
    {
      imu_rx_buffer_.erase(imu_rx_buffer_.begin());
    }
  }
}

void TideHardwareInterface::parseImuData(const uint8_t* frame)
{
  uint8_t rid = frame[3];
  
  std::lock_guard<std::mutex> lock(imu_mutex_);
  
  if (rid == 0x01) // Accel (加速度)
  {
    float accel[3];
    std::memcpy(accel, frame + 4, 3 * sizeof(float));
    imu_accel_[0] = static_cast<double>(accel[0]); // m/s^2
    imu_accel_[1] = static_cast<double>(accel[1]);
    imu_accel_[2] = static_cast<double>(accel[2]);
  }
  else if (rid == 0x02) // Gyro (角速度)
  {
    float gyro[3];
    std::memcpy(gyro, frame + 4, 3 * sizeof(float));
    imu_gyro_[0] = static_cast<double>(gyro[0]) * M_PI / 180.0; // 转换为 rad/s
    imu_gyro_[1] = static_cast<double>(gyro[1]) * M_PI / 180.0;
    imu_gyro_[2] = static_cast<double>(gyro[2]) * M_PI / 180.0;
  }
  else if (rid == 0x03) // RPY (姿态角)
  {
    float rpy[3];
    std::memcpy(rpy, frame + 4, 3 * sizeof(float));
    imu_rpy_[0] = static_cast<double>(rpy[0]) * M_PI / 180.0; // 转换为弧度
    imu_rpy_[1] = static_cast<double>(rpy[1]) * M_PI / 180.0;
    imu_rpy_[2] = static_cast<double>(rpy[2]) * M_PI / 180.0;
  }
  else if (rid == 0x04) // Quaternion (四元数)
  {
    float quat[4];
    std::memcpy(quat, frame + 4, 4 * sizeof(float));
    imu_orientation[0] = static_cast<double>(quat[0]); // W
    imu_orientation[1] = static_cast<double>(quat[1]); // X
    imu_orientation[2] = static_cast<double>(quat[2]); // Y
    imu_orientation[3] = static_cast<double>(quat[3]); // Z
  }
}

void TideHardwareInterface::stopMotors()
{
  for (auto& motor : motors_)
  {
    motor->stop();
    
    // 使用新协议发送停止指令
    MotorControlPacket packet;
    packet.header[0] = 0xFF;
    packet.header[1] = 0xAA;
    packet.data_length = sizeof(uint8_t) + 3 * sizeof(float);
    packet.motor_id = static_cast<uint8_t>(motor->config_.tx_id);
    packet.position = 0.0f;
    packet.velocity = 0.0f;
    packet.torque = 0.0f;

    size_t crc_data_size = sizeof(MotorControlPacket) - sizeof(uint32_t);
    packet.crc32 = calculateCRC32(reinterpret_cast<uint8_t*>(&packet), crc_data_size);

    std::vector<uint8_t> tx_data(sizeof(MotorControlPacket));
    std::memcpy(tx_data.data(), &packet, sizeof(MotorControlPacket));

    if (serial_device_)
    {
      serial_device_->write(tx_data);
    }
  }
}

// ==================== 自定义数据发送函数（新增）====================

/**
 * @brief 构建自定义数据包（内部函数）
 * @param data 要发送的数据
 * @param length 数据长度
 * @return 完整的数据包（包含帧头、长度、数据、CRC32）
 */
std::vector<uint8_t> TideHardwareInterface::buildCustomPacket(const uint8_t* data, size_t length)
{
  // 计算总包大小：帧头(2) + 长度(2) + 数据(length) + CRC32(4)
  size_t total_size = 2 + 2 + length + 4;
  std::vector<uint8_t> packet(total_size);
  
  size_t offset = 0;
  
  // 1. 帧头
  packet[offset++] = 0xFF;
  packet[offset++] = 0xAA;
  
  // 2. 数据长度（小端序）
  uint16_t data_length = static_cast<uint16_t>(length);
  std::memcpy(packet.data() + offset, &data_length, sizeof(uint16_t));
  offset += sizeof(uint16_t);
  
  // 3. 数据
  std::memcpy(packet.data() + offset, data, length);
  offset += length;
  
  // 4. 计算CRC32（从帧头到数据结束）
  uint32_t crc = calculateCRC32(packet.data(), offset);
  std::memcpy(packet.data() + offset, &crc, sizeof(uint32_t));
  
  return packet;
}

/**
 * @brief 发送自定义数据（vector版本）
 * @param data 要发送的数据
 * 
 * 使用示例：
 *   std::vector<uint8_t> my_data = {0x01, 0x02, 0x03, 0x04};
 *   sendCustomData(my_data);
 */
void TideHardwareInterface::sendCustomData(const std::vector<uint8_t>& data)
{
  if (data.empty())
  {
    RCLCPP_WARN(rclcpp::get_logger("TideHardwareInterface"), 
                "Attempted to send empty data");
    return;
  }
  
  auto packet = buildCustomPacket(data.data(), data.size());
  
  if (serial_device_)
  {
    serial_device_->write(packet);
    RCLCPP_DEBUG(rclcpp::get_logger("TideHardwareInterface"), 
                 "Sent custom data: %zu bytes (total packet: %zu bytes)", 
                 data.size(), packet.size());
  }
  else
  {
    RCLCPP_ERROR(rclcpp::get_logger("TideHardwareInterface"), 
                 "Serial device not initialized");
  }
}

/**
 * @brief 发送自定义数据（指针版本）
 * @param data 数据指针
 * @param length 数据长度
 * 
 * 使用示例：
 *   uint8_t my_data[] = {0x01, 0x02, 0x03, 0x04};
 *   sendCustomData(my_data, sizeof(my_data));
 */
void TideHardwareInterface::sendCustomData(const uint8_t* data, size_t length)
{
  if (data == nullptr || length == 0)
  {
    RCLCPP_WARN(rclcpp::get_logger("TideHardwareInterface"), 
                "Attempted to send null or empty data");
    return;
  }
  
  auto packet = buildCustomPacket(data, length);
  
  if (serial_device_)
  {
    serial_device_->write(packet);
    RCLCPP_DEBUG(rclcpp::get_logger("TideHardwareInterface"), 
                 "Sent custom data: %zu bytes (total packet: %zu bytes)", 
                 length, packet.size());
  }
  else
  {
    RCLCPP_ERROR(rclcpp::get_logger("TideHardwareInterface"), 
                 "Serial device not initialized");
  }
}

/**
 * @brief 发送浮点数数组（vector版本）
 * @param floats 浮点数数组
 * 
 * 使用示例：
 *   std::vector<float> data = {1.23f, 4.56f, 7.89f};
 *   sendFloatArray(data);
 */
void TideHardwareInterface::sendFloatArray(const std::vector<float>& floats)
{
  if (floats.empty())
  {
    RCLCPP_WARN(rclcpp::get_logger("TideHardwareInterface"), 
                "Attempted to send empty float array");
    return;
  }
  
  size_t data_size = floats.size() * sizeof(float);
  auto packet = buildCustomPacket(reinterpret_cast<const uint8_t*>(floats.data()), data_size);
  
  if (serial_device_)
  {
    serial_device_->write(packet);
    RCLCPP_DEBUG(rclcpp::get_logger("TideHardwareInterface"), 
                 "Sent %zu floats (%zu bytes, total packet: %zu bytes)", 
                 floats.size(), data_size, packet.size());
  }
  else
  {
    RCLCPP_ERROR(rclcpp::get_logger("TideHardwareInterface"), 
                 "Serial device not initialized");
  }
}

/**
 * @brief 发送浮点数数组（指针版本）
 * @param floats 浮点数指针
 * @param count 浮点数个数
 * 
 * 使用示例：
 *   float data[] = {1.23f, 4.56f, 7.89f};
 *   sendFloatArray(data, 3);
 */
void TideHardwareInterface::sendFloatArray(const float* floats, size_t count)
{
  if (floats == nullptr || count == 0)
  {
    RCLCPP_WARN(rclcpp::get_logger("TideHardwareInterface"), 
                "Attempted to send null or empty float array");
    return;
  }
  
  size_t data_size = count * sizeof(float);
  auto packet = buildCustomPacket(reinterpret_cast<const uint8_t*>(floats), data_size);
  
  if (serial_device_)
  {
    serial_device_->write(packet);
    RCLCPP_DEBUG(rclcpp::get_logger("TideHardwareInterface"), 
                 "Sent %zu floats (%zu bytes, total packet: %zu bytes)", 
                 count, data_size, packet.size());
  }
  else
  {
    RCLCPP_ERROR(rclcpp::get_logger("TideHardwareInterface"), 
                 "Serial device not initialized");
  }
}

// ==================== 自定义数据发送函数结束 ====================

hardware_interface::return_type TideHardwareInterface::read(const rclcpp::Time& time,
                                                            const rclcpp::Duration& period)
{
  auto current_time = time;

  // 读取电机状态（使用互斥锁保护）
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
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
  }
  
  // 方案三：在主线程中发布RPY话题
  // IMU数据已经在 parseImuData() 中更新到内存变量
  // 状态接口会自动读取这些变量
  if (publish_rpy_ && rpy_pub_)
  {
    auto rpy_msg = geometry_msgs::msg::Vector3Stamped();
    rpy_msg.header.stamp = nh_->now();
    rpy_msg.header.frame_id = imu_frame_id_;
    
    // 读取IMU数据（使用互斥锁保护）
    {
      std::lock_guard<std::mutex> lock(imu_mutex_);
      // 转为角度（方便Foxglove显示）
      rpy_msg.vector.x = imu_rpy_[0] * 180.0 / M_PI;  // roll (degrees)
      rpy_msg.vector.y = imu_rpy_[1] * 180.0 / M_PI;  // pitch (degrees)
      rpy_msg.vector.z = imu_rpy_[2] * 180.0 / M_PI;  // yaw (degrees)
    }
    
    rpy_pub_->publish(rpy_msg);
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

  // ========== 发送自定义数据给下位机 ==========
  
  // 方式1：只发送浮点数（最简单）
  std::vector<float> float_data;
  
  // 添加所有关节的位置和速度
  for (size_t i = 0; i < joint_count; i++)
  {
    double position_cmd = 0.0;
    double velocity_cmd = 0.0;
    
    if (!std::isnan(cmd_positions_[i]) && info_.joints[i].command_interfaces[0].name == "position")
    {
      position_cmd = cmd_positions_[i];
    }
    if (!std::isnan(cmd_velocities_[i]) && info_.joints[i].command_interfaces[0].name == "velocity")
    {
      velocity_cmd = cmd_velocities_[i];
    }
    
    float_data.push_back(static_cast<float>(position_cmd));
    float_data.push_back(static_cast<float>(velocity_cmd));
  }
  
  // 发送浮点数数组
  sendFloatArray(float_data);
  
  // 方式2：发送混合数据（浮点数 + 整数）
  // PacketBuilder builder;
  // 
  // // 添加数据包类型
  // builder.addByte(0x01);
  // 
  // // 添加时间戳
  // builder.addUInt32(static_cast<uint32_t>(
  //   std::chrono::system_clock::now().time_since_epoch().count() / 1000000
  // ));
  // 
  // // 添加关节数据
  // for (size_t i = 0; i < joint_count; i++)
  // {
  //   double position_cmd = 0.0;
  //   double velocity_cmd = 0.0;
  //   
  //   if (!std::isnan(cmd_positions_[i]) && info_.joints[i].command_interfaces[0].name == "position")
  //   {
  //     position_cmd = cmd_positions_[i];
  //   }
  //   if (!std::isnan(cmd_velocities_[i]) && info_.joints[i].command_interfaces[0].name == "velocity")
  //   {
  //     velocity_cmd = cmd_velocities_[i];
  //   }
  //   
  //   builder.addFloat(static_cast<float>(position_cmd))
  //          .addFloat(static_cast<float>(velocity_cmd));
  // }
  // 
  // // 添加状态标志
  // builder.addUInt16(0x0001);
  // 
  // // 发送
  // sendCustomData(builder.getData());

  return hardware_interface::return_type::OK;
}

}  // namespace tide_hw_interface

#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(tide_hw_interface::TideHardwareInterface, hardware_interface::SystemInterface)
