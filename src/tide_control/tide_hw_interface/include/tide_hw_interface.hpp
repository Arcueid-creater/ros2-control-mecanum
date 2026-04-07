/*
 * @Author: qinghuan 1484237245@qq.com
 * @Date: 2025-02-05 16:07:31
 * @FilePath: /TideControls/src/tide_control/tide_hw_interface/include/tide_hw_interface.hpp
 * @Description:
 *
 * Copyright (c) 2025 by qinghuan, All Rights Reserved.
 */
#ifndef TIDE_HARDWARE_INTERFACE_HPP
#define TIDE_HARDWARE_INTERFACE_HPP

#include "hardware_interface/system_interface.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "geometry_msgs/msg/vector3_stamped.hpp"
#include "tide_msgs/msg/rc_data.hpp"
#include "tide_motor.hpp"
#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <array>

namespace tide_hw_interface
{

// 数据包构建辅助类
class PacketBuilder
{
public:
  PacketBuilder() { data_.clear(); }
  
  // 添加浮点数
  PacketBuilder& addFloat(float value)
  {
    uint8_t* ptr = reinterpret_cast<uint8_t*>(&value);
    data_.insert(data_.end(), ptr, ptr + sizeof(float));
    return *this;
  }
  
  // 添加多个浮点数
  PacketBuilder& addFloats(const std::vector<float>& values)
  {
    for (float v : values)
    {
      addFloat(v);
    }
    return *this;
  }
  
  // 添加整数
  PacketBuilder& addInt32(int32_t value)
  {
    uint8_t* ptr = reinterpret_cast<uint8_t*>(&value);
    data_.insert(data_.end(), ptr, ptr + sizeof(int32_t));
    return *this;
  }
  
  PacketBuilder& addUInt32(uint32_t value)
  {
    uint8_t* ptr = reinterpret_cast<uint8_t*>(&value);
    data_.insert(data_.end(), ptr, ptr + sizeof(uint32_t));
    return *this;
  }
  
  PacketBuilder& addInt16(int16_t value)
  {
    uint8_t* ptr = reinterpret_cast<uint8_t*>(&value);
    data_.insert(data_.end(), ptr, ptr + sizeof(int16_t));
    return *this;
  }
  
  PacketBuilder& addUInt16(uint16_t value)
  {
    uint8_t* ptr = reinterpret_cast<uint8_t*>(&value);
    data_.insert(data_.end(), ptr, ptr + sizeof(uint16_t));
    return *this;
  }
  
  // 添加字节
  PacketBuilder& addByte(uint8_t value)
  {
    data_.push_back(value);
    return *this;
  }
  
  // 添加字节数组
  PacketBuilder& addBytes(const std::vector<uint8_t>& bytes)
  {
    data_.insert(data_.end(), bytes.begin(), bytes.end());
    return *this;
  }
  
  // 获取数据
  const std::vector<uint8_t>& getData() const { return data_; }
  
  // 清空
  void clear() { data_.clear(); }
  
  // 获取大小
  size_t size() const { return data_.size(); }
  
private:
  std::vector<uint8_t> data_;
};

class SerialDevice
{
public:
  using ReceiveCallback = std::function<void(const std::vector<uint8_t>&)>;
  using TickCallback = std::function<void()>;

  SerialDevice(const std::string& port_name, uint32_t baud_rate, ReceiveCallback callback, TickCallback tick_callback = nullptr);
  ~SerialDevice();

  void write(const std::vector<uint8_t>& data);

private:
  void run();

  std::string port_name_;
  uint32_t baud_rate_;
  ReceiveCallback receive_callback_;
  TickCallback tick_callback_;

  std::atomic<bool> running_{false};
  std::thread thread_;
  
  std::mutex write_mutex_;
  std::queue<std::vector<uint8_t>> write_queue_;
  std::condition_variable write_cv_;

  int fd_{-1};
};

class TideHardwareInterface : public hardware_interface::SystemInterface
{
public:
  RCLCPP_SHARED_PTR_DEFINITIONS(TideHardwareInterface)
  TideHardwareInterface();
  virtual ~TideHardwareInterface() = default;

  hardware_interface::CallbackReturn on_init(const hardware_interface::HardwareInfo& info) override;

  std::vector<hardware_interface::StateInterface> export_state_interfaces() override;

  std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;

  hardware_interface::return_type read(const rclcpp::Time& time,
                                       const rclcpp::Duration& period) override;

  hardware_interface::return_type write(const rclcpp::Time& time,
                                        const rclcpp::Duration& period) override;

  hardware_interface::CallbackReturn
  on_configure(const rclcpp_lifecycle::State& previous_state) override;

  hardware_interface::CallbackReturn
  on_cleanup(const rclcpp_lifecycle::State& previous_state) override;

  hardware_interface::CallbackReturn
  on_activate(const rclcpp_lifecycle::State& previous_state) override;

  hardware_interface::CallbackReturn
  on_deactivate(const rclcpp_lifecycle::State& previous_state) override;

public:
  // 自定义数据发送函数（新增）
  void sendCustomData(const std::vector<uint8_t>& data);
  void sendCustomData(const uint8_t* data, size_t length);
  void sendFloatArray(const std::vector<float>& floats);
  void sendFloatArray(const float* floats, size_t count);

private:
  void stopMotors();
  void parseMotorData(const std::vector<uint8_t>& data);
  void parseImuSerialData(const std::vector<uint8_t>& data);
  uint16_t calculateCRC16(const uint8_t* data, size_t len);
  uint32_t calculateCRC32(const uint8_t* data, size_t len);
  void parseImuData(const uint8_t* frame);
  std::vector<uint8_t> buildCustomPacket(const uint8_t* data, size_t length);

  // 电机控制协议 - 使用浮点数和CRC32
  struct MotorControlPacket {
    uint8_t header[2];     // 0xFF, 0xAA
    uint16_t data_length;  // 数据段长度（字节数）
    uint8_t motor_id;
    float position;        // 位置指令
    float velocity;        // 速度指令
    float torque;          // 力矩指令
    uint32_t crc32;        // CRC32校验
  } __attribute__((packed));

  // 电机反馈协议 - 简化版，只接收原始数据
  struct MotorFeedbackPacket {
    uint8_t header[2];     // 0xFF, 0xAA
    uint16_t data_length;  // 数据段长度（字节数）
    // 后面是data_length字节的数据
    // 最后是4字节CRC32
  } __attribute__((packed));

  std::vector<std::shared_ptr<DJI_Motor>> motors_;
  std::shared_ptr<SerialDevice> serial_device_;
  std::shared_ptr<SerialDevice> imu_serial_device_;
  std::string serial_port_;
  uint32_t baud_rate_;
  std::string imu_serial_port_;
  uint32_t imu_baud_rate_;

  size_t joint_count{ 0 };

  std::vector<double> cmd_positions_;
  std::vector<double> cmd_velocities_;
  std::vector<double> state_positions_;
  std::vector<double> state_velocities_;
  std::vector<double> state_currents_;
  std::vector<double> state_temperatures_;
  
  std::vector<uint8_t> motor_rx_buffer_;    // 电机接收缓冲区
  std::vector<uint8_t> motor_data_buffer_;  // 存储解析后的电机数据（不含帧头帧尾和CRC）
  std::vector<uint8_t> imu_rx_buffer_;

  bool need_calibration_{ false };
  bool enable_virtual_control_{ false };
  std::atomic<bool> stop_thread_{ false };
  mutable std::mutex device_mutex_;
  mutable std::mutex state_mutex_;  // 保护电机状态变量
  mutable std::mutex imu_mutex_;    // 保护IMU数据

  // IMU related
  double imu_rpy_[3] = {0.0, 0.0, 0.0};
  double imu_gyro_[3] = {0.0, 0.0, 0.0};
  double imu_accel_[3] = {0.0, 0.0, 0.0};
  double imu_orientation[4]={0.0,0.0,0.0,0.0};
  bool publish_imu_data_{true};
  bool publish_rpy_{true};
  std::string imu_frame_id_{"imu_link"};

  // 方案三：保留节点和RPY发布器，删除IMU发布器和spin线程
  rclcpp::Publisher<geometry_msgs::msg::Vector3Stamped>::SharedPtr rpy_pub_;
  rclcpp::Publisher<tide_msgs::msg::RcData>::SharedPtr rc_dbus_pub_;
  std::shared_ptr<rclcpp::Node> nh_;
};

}  // namespace tide_hw_interface

#endif  // TIDE_HARDWARE_INTERFACE_HPP
