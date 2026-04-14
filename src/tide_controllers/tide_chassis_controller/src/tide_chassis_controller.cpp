/*
 * @Author: Kiro AI Assistant
 * @Date: 2025-01-26
 * @FilePath: /TideControls/src/tide_controllers/tide_chassis_controller/src/tide_chassis_controller.cpp
 * @Description: Chassis controller implementation
 *
 * Copyright (c) 2025, All Rights Reserved.
 */
#include "tide_chassis_controller.hpp"

namespace tide_chassis_controller
{
using controller_interface::interface_configuration_type;
using controller_interface::InterfaceConfiguration;

TideChassisController::TideChassisController() : controller_interface::ControllerInterface() {}

controller_interface::CallbackReturn TideChassisController::on_init()
{
  try
  {
    param_listener_ = std::make_shared<ParamListener>(get_node());
    params_ = param_listener_->get_params();

    auto cmd = std::make_shared<CMD>();
    cmd->linear_x = 0.0;
    cmd->linear_y = 0.0;
    cmd->angular_z = 0.0;

    recv_cmd_ptr_.initRT(cmd);
  }
  catch (const std::exception& e)
  {
    fprintf(stderr, "Exception thrown during init stage with message: %s \n", e.what());
    return controller_interface::CallbackReturn::ERROR;
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

InterfaceConfiguration TideChassisController::command_interface_configuration() const
{
  std::vector<std::string> conf_names;
  
  // 添加底盘速度命令接口（直接内存访问）
  conf_names.push_back("chassis/linear_x");
  conf_names.push_back("chassis/linear_y");
  conf_names.push_back("chassis/angular_z");

  return { interface_configuration_type::INDIVIDUAL, conf_names };
}

InterfaceConfiguration TideChassisController::state_interface_configuration() const
{
  std::vector<std::string> conf_names;

  // 添加遥控器状态接口（直接内存访问）
  conf_names.push_back("rc/ch1");
  conf_names.push_back("rc/ch2");
  conf_names.push_back("rc/ch3");
  conf_names.push_back("rc/ch4");
  conf_names.push_back("rc/sw1");
  conf_names.push_back("rc/sw2");
  conf_names.push_back("rc/mouse_x");
  conf_names.push_back("rc/mouse_y");
  conf_names.push_back("rc/mouse_l");
  conf_names.push_back("rc/mouse_r");
  conf_names.push_back("rc/key_code");
  conf_names.push_back("rc/wheel");

  return { interface_configuration_type::INDIVIDUAL, conf_names };
}

controller_interface::CallbackReturn
TideChassisController::on_configure(const rclcpp_lifecycle::State& /*previous_state*/)
{
  params_ = param_listener_->get_params();

  cmd_sub_ = get_node()->create_subscription<CMD>(
      "~/chassis_cmd", rclcpp::SystemDefaultsQoS(),
      [this](const std::shared_ptr<CMD> msg) -> void {
        recv_cmd_ptr_.writeFromNonRT(msg);
      });

  auto chassis_state_pub =
      get_node()->create_publisher<STATE>("~/chassis_state", rclcpp::SystemDefaultsQoS());
  rt_chassis_state_pub_ =
      std::make_shared<realtime_tools::RealtimePublisher<STATE>>(chassis_state_pub);

  RCLCPP_INFO(get_node()->get_logger(), "Chassis controller configured successfully");

  return controller_interface::CallbackReturn::SUCCESS;
}

void TideChassisController::update_parameters()
{
  if (!param_listener_->is_old(params_))
  {
    return;
  }
  params_ = param_listener_->get_params();
}

controller_interface::return_type TideChassisController::update(const rclcpp::Time& time,
                                                                const rclcpp::Duration& period)
{
  static int update_count = 0;
  update_count++;
  
  // 每1000次打印一次，避免日志过多
  if (update_count % 1000 == 1)
  {
    RCLCPP_INFO(get_node()->get_logger(), "Chassis Controller update() called! Count: %d", update_count);
  }

  update_parameters();

  // ========== 方式1：从话题接收命令（上层应用控制）==========
  auto cmd = *recv_cmd_ptr_.readFromRT();
  
  if (cmd != nullptr)
  {
    linear_x_cmd_ = cmd->linear_x;
    linear_y_cmd_ = cmd->linear_y;
    angular_z_cmd_ = cmd->angular_z;
    
    // 每1000次打印一次命令信息
    if (update_count % 1000 == 1)
    {
      RCLCPP_INFO(get_node()->get_logger(), 
                  "Received cmd - linear_x: %.3f, linear_y: %.3f, angular_z: %.3f",
                  linear_x_cmd_, linear_y_cmd_, angular_z_cmd_);
    }
  }
  
  // ========== 方式2：直接读取遥控器数据（零延迟，直接内存访问）==========
  // 如果需要遥控器直接控制底盘，取消下面的注释
  
  if (rc_state_interfaces_.count("ch1") && rc_state_interfaces_.count("ch2") && 
      rc_state_interfaces_.count("ch3") && rc_state_interfaces_.count("sw1"))
  {
    // 读取遥控器摇杆值（范围：-660 ~ 660）
    double ch1 = rc_state_interfaces_["ch1"]->get_value();  // 右摇杆左右（底盘Y轴）
    double ch2 = rc_state_interfaces_["ch2"]->get_value();  // 右摇杆上下（底盘X轴）
    double ch3 = rc_state_interfaces_["ch3"]->get_value();  // 左摇杆左右（底盘旋转）
    double sw1 = rc_state_interfaces_["sw1"]->get_value();  // 左拨杆（模式切换）
    
    // 归一化到 [-1, 1]
    double norm_ch1 = ch1 / 660.0;
    double norm_ch2 = ch2 / 660.0;
    double norm_ch3 = ch3 / 660.0;
    
    // 设置最大速度（根据拨杆位置调整）
    double max_linear_vel = 1.0;   // m/s
    double max_angular_vel = 2.0;  // rad/s
    
    if (sw1 == 1)  // 上位：慢速模式
    {
      max_linear_vel = 0.5;
      max_angular_vel = 1.0;
    }
    else if (sw1 == 3)  // 下位：快速模式
    {
      max_linear_vel = 2.0;
      max_angular_vel = 4.0;
    }
    // sw1 == 2：中位，使用默认速度
    
    // 计算底盘速度命令
    linear_x_cmd_ = norm_ch2 * max_linear_vel;   // 前后
    linear_y_cmd_ = norm_ch1 * max_linear_vel;   // 左右
    angular_z_cmd_ = norm_ch3 * max_angular_vel; // 旋转
    
    // 死区处理（避免摇杆漂移）
    const double deadzone = 0.05;
    if (std::abs(linear_x_cmd_) < deadzone) linear_x_cmd_ = 0.0;
    if (std::abs(linear_y_cmd_) < deadzone) linear_y_cmd_ = 0.0;
    if (std::abs(angular_z_cmd_) < deadzone) angular_z_cmd_ = 0.0;
    
  }
  
  // 直接写入底盘速度命令到硬件接口（零延迟，直接内存访问）
  if (chassis_cmd_interfaces_.count("linear_x"))
  {
    chassis_cmd_interfaces_["linear_x"]->set_value(linear_x_cmd_);
  }
  if (chassis_cmd_interfaces_.count("linear_y"))
  {
    chassis_cmd_interfaces_["linear_y"]->set_value(linear_y_cmd_);
  }
  if (chassis_cmd_interfaces_.count("angular_z"))
  {
    chassis_cmd_interfaces_["angular_z"]->set_value(angular_z_cmd_);
  }

  // Publish chassis state
  if (rt_chassis_state_pub_->trylock())
  {
    auto& chassis_state = rt_chassis_state_pub_->msg_;
    chassis_state.header.stamp = time;
    chassis_state.linear_x = linear_x_cmd_;
    chassis_state.linear_y = linear_y_cmd_;
    chassis_state.angular_z = angular_z_cmd_;
    rt_chassis_state_pub_->unlockAndPublish();
  }

  return controller_interface::return_type::OK;
}

controller_interface::CallbackReturn
TideChassisController::on_activate(const rclcpp_lifecycle::State& /*previous_state*/)
{
  // Assign command interfaces
  chassis_cmd_interfaces_.clear();
  
  for (auto& cmd_interface : command_interfaces_)
  {
    const std::string interface_name = cmd_interface.get_interface_name();
    const std::string prefix_name = cmd_interface.get_prefix_name();
    
    // 底盘速度命令接口（使用 map 统一管理）
    if (prefix_name == "chassis")
    {
      chassis_cmd_interfaces_[interface_name] = 
          std::make_unique<hardware_interface::LoanedCommandInterface>(std::move(cmd_interface));
      RCLCPP_INFO(get_node()->get_logger(), "Claimed chassis/%s command interface", 
                  interface_name.c_str());
    }
  }

  // Assign state interfaces
  rc_state_interfaces_.clear();
  
  for (auto& state_interface : state_interfaces_)
  {
    const std::string interface_name = state_interface.get_interface_name();
    const std::string prefix_name = state_interface.get_prefix_name();
    
    // 遥控器状态接口（使用 map 统一管理）
    if (prefix_name == "rc")
    {
      rc_state_interfaces_[interface_name] = 
          std::make_unique<const hardware_interface::LoanedStateInterface>(std::move(state_interface));
      RCLCPP_INFO(get_node()->get_logger(), "Claimed rc/%s state interface", 
                  interface_name.c_str());
    }
  }

  RCLCPP_INFO(get_node()->get_logger(), 
              "Chassis controller activated with %zu chassis interfaces and %zu rc interfaces", 
              chassis_cmd_interfaces_.size(), rc_state_interfaces_.size());

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn
TideChassisController::on_deactivate(const rclcpp_lifecycle::State& /*previous_state*/)
{
  chassis_cmd_interfaces_.clear();
  rc_state_interfaces_.clear();

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn
TideChassisController::on_cleanup(const rclcpp_lifecycle::State& /*previous_state*/)
{
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn
TideChassisController::on_error(const rclcpp_lifecycle::State& /*previous_state*/)
{
  return controller_interface::CallbackReturn::SUCCESS;
}

}  // namespace tide_chassis_controller

#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(tide_chassis_controller::TideChassisController,
                       controller_interface::ControllerInterface)
