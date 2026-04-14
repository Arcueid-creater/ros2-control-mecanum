/*
 * @Author: Kiro AI Assistant
 * @Date: 2025-01-26
 * @FilePath: /TideControls/src/tide_controllers/tide_chassis_controller/include/tide_chassis_controller.hpp
 * @Description: Chassis controller for tide robot
 *
 * Copyright (c) 2025, All Rights Reserved.
 */

#ifndef TIDE_CHASSIS_CONTROLLER_HPP_
#define TIDE_CHASSIS_CONTROLLER_HPP_

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "controller_interface/controller_interface.hpp"
#include "hardware_interface/handle.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_buffer.hpp"
#include "realtime_tools/realtime_publisher.hpp"

#include <geometry_msgs/msg/twist.hpp>
#include "tide_msgs/msg/chassis_cmd.hpp"
#include "tide_msgs/msg/chassis_state.hpp"

#include "chassis_controller_parameters.hpp"

namespace tide_chassis_controller
{
class TideChassisController : public controller_interface::ControllerInterface
{
public:
  TideChassisController();

  controller_interface::CallbackReturn on_init() override;

  controller_interface::InterfaceConfiguration command_interface_configuration() const override;

  controller_interface::InterfaceConfiguration state_interface_configuration() const override;

  controller_interface::return_type update(const rclcpp::Time& time,
                                           const rclcpp::Duration& period) override;

  controller_interface::CallbackReturn
  on_configure(const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn
  on_activate(const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn
  on_deactivate(const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn
  on_cleanup(const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn
  on_error(const rclcpp_lifecycle::State& previous_state) override;

private:
  using CMD = tide_msgs::msg::ChassisCmd;
  using STATE = tide_msgs::msg::ChassisState;

  std::shared_ptr<ParamListener> param_listener_;
  Params params_;
  
  // 遥控器状态接口（直接内存访问）
  std::unordered_map<std::string, std::unique_ptr<const hardware_interface::LoanedStateInterface>> 
      rc_state_interfaces_;
  
  // 底盘速度命令接口（直接内存访问）- 使用 map 统一管理
  std::unordered_map<std::string, std::unique_ptr<hardware_interface::LoanedCommandInterface>> 
      chassis_cmd_interfaces_;

  // Command and state variables
  double linear_x_cmd_{ 0.0 };
  double linear_y_cmd_{ 0.0 };
  double angular_z_cmd_{ 0.0 };

  // Subscription and publisher
  rclcpp::Subscription<CMD>::SharedPtr cmd_sub_ = nullptr;
  realtime_tools::RealtimeBuffer<std::shared_ptr<CMD>> recv_cmd_ptr_{ nullptr };

  std::shared_ptr<realtime_tools::RealtimePublisher<STATE>> rt_chassis_state_pub_{ nullptr };

  void update_parameters();
};

}  // namespace tide_chassis_controller

#endif  // TIDE_CHASSIS_CONTROLLER_HPP_
