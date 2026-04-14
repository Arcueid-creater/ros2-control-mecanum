# Chassis 控制器 - 无轮子关节架构

## 架构说明

**不需要定义底盘轮子关节！** 

当前实现采用**直接速度控制**架构，控制器直接发送底盘速度命令（vx, vy, wz）到硬件接口，由下位机负责运动学解算和轮子控制。

---

## 数据流

```
遥控器/上层应用
    ↓
Chassis Controller
    ├─ 读取遥控器数据 (rc/ch1, rc/ch2, rc/ch3, ...)
    ├─ 计算底盘速度 (linear_x, linear_y, angular_z)
    └─ 写入命令接口 (chassis/linear_x, chassis/linear_y, chassis/angular_z)
    ↓ (直接内存访问)
Hardware Interface
    ├─ 读取底盘速度命令
    └─ 打包发送到下位机 (msg_id=0x20)
    ↓ (串口)
下位机 (STM32)
    ├─ 接收底盘速度命令
    ├─ 麦克纳姆轮运动学解算
    ├─ 计算各轮子速度
    └─ 控制电机
```

---

## 优势

### 1. 简化上位机

- ✅ **无需定义轮子关节**：URDF 中不需要 front_left_wheel_joint 等
- ✅ **无需运动学解算**：不需要在上位机实现麦克纳姆轮逆运动学
- ✅ **配置更简单**：只需配置速度限制，无需轮子参数

### 2. 下位机负责运动学

- ✅ **实时性更好**：运动学解算在下位机实时完成
- ✅ **更灵活**：修改运动学算法只需更新下位机代码
- ✅ **更可靠**：下位机直接控制电机，响应更快

### 3. 直接内存访问

- ✅ **零延迟**：控制器 → 硬件接口 < 0.1ms
- ✅ **零拷贝**：使用指针传递，无数据复制
- ✅ **实时性保证**：在同一个实时线程中执行

---

## 配置文件

### sentry_real.yaml / sentry_sim.yaml

```yaml
chassis_controller:
  ros__parameters:
    # 底盘速度限制
    max_linear_velocity: 3.0   # m/s
    max_angular_velocity: 3.0  # rad/s
    
    # 注意：运动学解算在下位机完成，上位机只发送速度命令
```

**不需要**：
- ❌ wheel_joints
- ❌ wheel_radius
- ❌ wheel_base
- ❌ wheel_track

---

## URDF 配置

### sentry_real.xacro / sentry_sim.xacro

**不需要定义轮子关节**：

```xml
<ros2_control name="TideHardwareInterface" type="system">
    <hardware>
        <plugin>tide_hw_interface/TideHardwareInterface</plugin>
        <!-- ... -->
    </hardware>

    <!-- 云台关节 -->
    <joint name="pitch1_joint">...</joint>
    <joint name="yaw1_joint">...</joint>
    <!-- ... -->
    
    <!-- 发射机构关节 -->
    <joint name="friction_wheel1_joint">...</joint>
    <joint name="loader1_joint">...</joint>
    <!-- ... -->
    
    <!-- IMU 传感器 -->
    <sensor name="imu_sensor">...</sensor>
    
    <!-- ❌ 不需要定义底盘轮子关节 -->
</ros2_control>
```

---

## 控制器实现

### 命令接口

```cpp
InterfaceConfiguration TideChassisController::command_interface_configuration() const
{
    std::vector<std::string> conf_names;
    
    // 只需要底盘速度命令接口
    conf_names.push_back("chassis/linear_x");
    conf_names.push_back("chassis/linear_y");
    conf_names.push_back("chassis/angular_z");
    
    // ❌ 不需要轮子速度接口
    // conf_names.push_back("front_left_wheel_joint/velocity");
    // ...
    
    return { interface_configuration_type::INDIVIDUAL, conf_names };
}
```

### 状态接口

```cpp
InterfaceConfiguration TideChassisController::state_interface_configuration() const
{
    std::vector<std::string> conf_names;
    
    // 只需要遥控器状态接口
    conf_names.push_back("rc/ch1");
    conf_names.push_back("rc/ch2");
    // ...
    
    // ❌ 不需要轮子状态接口
    // conf_names.push_back("front_left_wheel_joint/velocity");
    // ...
    
    return { interface_configuration_type::INDIVIDUAL, conf_names };
}
```

### 控制循环

```cpp
controller_interface::return_type 
TideChassisController::update(const rclcpp::Time& time,
                              const rclcpp::Duration& period)
{
    // 方式1：从话题接收命令
    auto cmd = *recv_cmd_ptr_.readFromRT();
    if (cmd != nullptr)
    {
        linear_x_cmd_ = cmd->linear_x;
        linear_y_cmd_ = cmd->linear_y;
        angular_z_cmd_ = cmd->angular_z;
    }
    
    // 方式2：直接读取遥控器数据
    if (rc_state_interfaces_.count("ch1"))
    {
        double ch1 = rc_state_interfaces_["ch1"]->get_value();
        double ch2 = rc_state_interfaces_["ch2"]->get_value();
        double ch3 = rc_state_interfaces_["ch3"]->get_value();
        
        // 计算底盘速度
        linear_x_cmd_ = ch2 / 660.0 * max_linear_vel;
        linear_y_cmd_ = ch1 / 660.0 * max_linear_vel;
        angular_z_cmd_ = ch3 / 660.0 * max_angular_vel;
    }
    
    // 直接写入底盘速度命令（不需要计算轮子速度）
    chassis_cmd_interfaces_["linear_x"]->set_value(linear_x_cmd_);
    chassis_cmd_interfaces_["linear_y"]->set_value(linear_y_cmd_);
    chassis_cmd_interfaces_["angular_z"]->set_value(angular_z_cmd_);
    
    // ❌ 不需要设置轮子速度
    // wheel_command_interfaces_[0]->set_value(fl_vel);
    // wheel_command_interfaces_[1]->set_value(fr_vel);
    // ...
    
    return controller_interface::return_type::OK;
}
```

---

## 硬件接口实现

### 导出命令接口

```cpp
std::vector<hardware_interface::CommandInterface> 
TideHardwareInterface::export_command_interfaces()
{
    std::vector<hardware_interface::CommandInterface> interfaces;
    
    // 导出关节命令接口（云台、发射机构等）
    for (size_t i = 0; i < joint_count; i++)
    {
        // ...
    }
    
    // 导出底盘速度命令接口（GPIO类型）
    interfaces.emplace_back("chassis", "linear_x", &chassis_cmd_linear_x_);
    interfaces.emplace_back("chassis", "linear_y", &chassis_cmd_linear_y_);
    interfaces.emplace_back("chassis", "angular_z", &chassis_cmd_angular_z_);
    
    return interfaces;
}
```

### 发送到下位机

```cpp
hardware_interface::return_type 
TideHardwareInterface::write(const rclcpp::Time& /*time*/,
                             const rclcpp::Duration& /*period*/)
{
    // 读取底盘速度命令（直接访问成员变量）
    float vx = static_cast<float>(chassis_cmd_linear_x_);
    float vy = static_cast<float>(chassis_cmd_linear_y_);
    float wz = static_cast<float>(chassis_cmd_angular_z_);
    
    // 构建数据包：[msg_id=0x20][vx][vy][wz]
    PacketBuilder builder;
    builder.addByte(0x20)
           .addFloat(vx)
           .addFloat(vy)
           .addFloat(wz);
    
    // 发送到下位机
    sendCustomData(builder.getData());
    
    return hardware_interface::return_type::OK;
}
```

---

## 下位机实现（参考）

### 接收底盘速度命令

```c
// 下位机代码（STM32）
void parse_chassis_cmd(uint8_t* data, uint16_t len)
{
    if (data[0] == 0x20)  // 底盘速度命令
    {
        float vx, vy, wz;
        memcpy(&vx, data + 1, sizeof(float));
        memcpy(&vy, data + 5, sizeof(float));
        memcpy(&wz, data + 9, sizeof(float));
        
        // 麦克纳姆轮逆运动学解算
        float fl_vel = vx - vy - wz * (L + W);
        float fr_vel = vx + vy + wz * (L + W);
        float rl_vel = vx + vy - wz * (L + W);
        float rr_vel = vx - vy + wz * (L + W);
        
        // 控制电机
        set_motor_velocity(MOTOR_FL, fl_vel);
        set_motor_velocity(MOTOR_FR, fr_vel);
        set_motor_velocity(MOTOR_RL, rl_vel);
        set_motor_velocity(MOTOR_RR, rr_vel);
    }
}
```

---

## 对比：有轮子关节 vs 无轮子关节

| 特性 | 有轮子关节 | 无轮子关节（当前） |
|------|-----------|------------------|
| **URDF 定义** | 需要定义 4 个轮子关节 | ❌ 不需要 |
| **配置参数** | wheel_joints, wheel_radius, wheel_base, wheel_track | 只需速度限制 |
| **运动学解算** | 上位机实现 | 下位机实现 ✅ |
| **控制器复杂度** | 高（需要逆运动学） | 低（直接速度控制）✅ |
| **实时性** | 中等 | 高 ✅ |
| **灵活性** | 低（修改需重新编译） | 高（修改下位机即可）✅ |
| **调试难度** | 高 | 低 ✅ |
| **轮子监控** | 可以单独监控每个轮子 | 无法监控单个轮子 |
| **里程计** | 可以从轮子编码器计算 | 需要其他方式（IMU、视觉等） |

---

## 启动和验证

### 启动系统

```bash
ros2 launch tide_ctrl_bringup tide_ctrl_bringup.launch.py robot_type:=sentry sim_mode:=false
```

### 检查控制器

```bash
ros2 control list_controllers
```

**期望输出**：
```
chassis_controller[tide_chassis_controller/TideChassisController] active
```

### 检查接口

```bash
ros2 control list_hardware_interfaces
```

**期望输出**：
```
command interfaces
    chassis/angular_z [available] [claimed]
    chassis/linear_x [available] [claimed]
    chassis/linear_y [available] [claimed]

state interfaces
    rc/ch1 [available] [claimed]
    rc/ch2 [available] [claimed]
    rc/ch3 [available] [claimed]
    ...
```

**不应该看到**：
```
❌ front_left_wheel_joint/velocity
❌ front_right_wheel_joint/velocity
❌ rear_left_wheel_joint/velocity
❌ rear_right_wheel_joint/velocity
```

### 发送测试命令

```bash
ros2 topic pub /chassis_controller/chassis_cmd tide_msgs/msg/ChassisCmd "{linear_x: 1.0, linear_y: 0.0, angular_z: 0.0}" --once
```

### 查看底盘状态

```bash
ros2 topic echo /chassis_controller/chassis_state
```

---

## 总结

✅ **无需定义底盘轮子关节！**

当前架构采用**直接速度控制**方式：
- 控制器只发送底盘速度命令（vx, vy, wz）
- 硬件接口直接转发到下位机
- 下位机负责运动学解算和轮子控制

**优势**：
- 更简单的配置
- 更低的复杂度
- 更好的实时性
- 更灵活的修改

**适用场景**：
- 下位机有足够的计算能力
- 运动学算法在下位机实现
- 不需要单独监控每个轮子
- 追求简单和实时性

---

## 相关文档

- [遥控器直接内存访问技术文档](RC_DIRECT_MEMORY_ACCESS.md)
- [遥控器直接访问快速使用指南](RC_DIRECT_ACCESS_USAGE.md)
- [底盘控制器集成状态](CHASSIS_CONTROLLER_INTEGRATION_STATUS.md)
- [串口通信协议](CHASSIS_SERIAL_COMMUNICATION.md)
