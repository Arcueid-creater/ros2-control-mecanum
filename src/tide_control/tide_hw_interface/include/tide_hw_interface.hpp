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

class SerialDevice
{
public:
  using ReceiveCallback = std::function<void(const std::vector<uint8_t>&)>;

  SerialDevice(const std::string& port_name, uint32_t baud_rate, ReceiveCallback callback);
  ~SerialDevice();

  void write(const std::vector<uint8_t>& data);

private:
  void run();

  std::string port_name_;
  uint32_t baud_rate_;
  ReceiveCallback receive_callback_;

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

private:
  void stopMotors();
  void parseMotorData(const std::vector<uint8_t>& data);
  void parseImuSerialData(const std::vector<uint8_t>& data);
  void parseSharedData(const std::vector<uint8_t>& data);
  uint16_t calculateCRC16(const uint8_t* data, size_t len);
  void parseImuData(const uint8_t* frame);

  struct ControlPacket {
    uint8_t header = 0xA5;
    uint8_t motor_id;
    int16_t command;
  } __attribute__((packed));

  struct FeedbackPacket {
    uint8_t header = 0x5A;
    uint8_t motor_id;
    int16_t ecd;
    int16_t speed_rpm;
    int16_t given_current;
    uint8_t temperature;
  } __attribute__((packed));

  struct ImuPacket {
    uint8_t header[2]; // 0x55, 0xAA
    uint8_t dummy;
    uint8_t rid;       // 0x01: RPY, 0x02: Gyro, 0x03: Accel
    float data[3];     // Generic data
    uint16_t crc;
    uint8_t tail;      // 0x0A
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
  
  std::vector<uint8_t> motor_rx_buffer_;
  std::vector<uint8_t> imu_rx_buffer_;

  bool need_calibration_{ false };
  bool enable_virtual_control_{ false };
  std::atomic<bool> stop_thread_{ false };
  mutable std::mutex device_mutex_;

  // IMU related
  double imu_rpy_[3] = {0.0, 0.0, 0.0};
  double imu_gyro_[3] = {0.0, 0.0, 0.0};
  double imu_accel_[3] = {0.0, 0.0, 0.0};
  double imu_orientation[4]={0.0,0.0,0.0,0.0};
  bool publish_imu_data_{true};
  bool publish_rpy_{true};
  std::string imu_frame_id_{"imu_link"};

  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
  rclcpp::Publisher<geometry_msgs::msg::Vector3Stamped>::SharedPtr rpy_pub_;
  std::shared_ptr<rclcpp::Node> nh_;
  std::thread imu_spin_thread_;
};

}  // namespace tide_hw_interface

#endif  // TIDE_HARDWARE_INTERFACE_HPP
