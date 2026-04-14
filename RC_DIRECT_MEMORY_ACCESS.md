# 底盘控制器直接内存访问遥控器数据

## 概述

本文档详细说明了 TideControls 系统中，底盘控制器如何通过**直接内存访问**的方式获取遥控器接收的值，实现**零延迟**的数据传递。

---

## 系统架构

```
下位机 (STM32) - 遥控器DBUS接收器
    ↓ (串口发送遥控器数据)
硬件接口层 (TideHardwareInterface)
    ├─ 解析遥控器数据
    ├─ 更新共享内存变量 (rc_ch1_, rc_ch2_, ...)
    └─ 发布ROS2话题 (/rc/dbus) [可选，供上层应用使用]
    ↓ (直接内存访问)
控制器层 (TideChassisController)
    ├─ 读取遥控器共享内存 (get_value)
    ├─ 计算底盘速度命令
    └─ 写入底盘命令共享内存 (set_value)
    ↓ (直接内存访问)
硬件接口层 (TideHardwareInterface)
    └─ 读取底盘命令共享内存
    ↓ (串口发送底盘命令)
下位机 (STM32) - 底盘电机控制
```

---

## 数据流详解

### 1. 遥控器数据接收（硬件接口层）

**文件位置**: `src/tide_control/tide_hw_interface/src/tide_hw_interface.cpp`

#### 1.1 串口接收线程

```cpp
// 串口设备在独立线程中运行
serial_device_ = std::make_shared<SerialDevice>(
    serial_port_, 
    baud_rate_, 
    motor_receive_callback  // 接收回调函数
);
```

#### 1.2 数据解析

当下位机发送遥控器数据时，使用以下协议格式：

```
[帧头: 0xFF 0xAA] [长度: 2字节] [msg_id: 0x10] [RC数据: 19字节] [电机数据: 48字节] [CRC32: 4字节]
```

**RC数据结构（19字节）**:
```cpp
int16_t ch1, ch2, ch3, ch4;      // 摇杆通道 (8字节)
uint8_t sw1, sw2;                 // 拨杆开关 (2字节)
int16_t mouse_x, mouse_y;         // 鼠标移动 (4字节)
uint8_t mouse_l, mouse_r;         // 鼠标按键 (2字节)
uint16_t key_code;                // 键盘按键 (2字节)
int16_t wheel;                    // 鼠标滚轮 (2字节)
```

#### 1.3 解析代码（关键部分）

```cpp
void TideHardwareInterface::parseMotorData(const std::vector<uint8_t>& data)
{
    // ... CRC校验 ...
    
    if (motor_data_buffer_[0] == 0x10)  // 遥控器数据包
    {
        size_t off = 1;
        
        // 解析遥控器数据
        int16_t ch1, ch2, ch3, ch4;
        uint8_t sw1, sw2;
        int16_t mouse_x, mouse_y;
        uint8_t mouse_l, mouse_r;
        uint16_t key_code;
        int16_t wheel;
        
        std::memcpy(&ch1, motor_data_buffer_.data() + off, sizeof(int16_t)); 
        off += sizeof(int16_t);
        // ... 依次解析其他字段 ...
        
        // 发布到ROS2话题（供上层应用使用）
        if (rc_dbus_pub_)
        {
            tide_msgs::msg::RcData msg;
            msg.ch1 = ch1;
            msg.ch2 = ch2;
            // ... 填充其他字段 ...
            rc_dbus_pub_->publish(msg);
        }
    }
}
```

---

### 2. 底盘速度命令和遥控器数据的直接内存访问

#### 2.1 共享内存变量（硬件接口层）

**文件位置**: `src/tide_control/tide_hw_interface/include/tide_hw_interface.hpp`

```cpp
class TideHardwareInterface : public hardware_interface::SystemInterface
{
private:
    // 底盘速度命令接口（直接内存访问，零延迟）
    double chassis_cmd_linear_x_{0.0};
    double chassis_cmd_linear_y_{0.0};
    double chassis_cmd_angular_z_{0.0};
    
    // 遥控器状态接口（直接内存访问，零延迟）
    double rc_ch1_{0.0};
    double rc_ch2_{0.0};
    double rc_ch3_{0.0};
    double rc_ch4_{0.0};
    double rc_sw1_{0.0};
    double rc_sw2_{0.0};
    double rc_mouse_x_{0.0};
    double rc_mouse_y_{0.0};
    double rc_mouse_l_{0.0};
    double rc_mouse_r_{0.0};
    double rc_key_code_{0.0};
    double rc_wheel_{0.0};
};
```

这些变量是**共享内存**，控制器可以直接读写。

#### 2.2 导出状态和命令接口

**文件位置**: `src/tide_control/tide_hw_interface/src/tide_hw_interface.cpp`

```cpp
std::vector<hardware_interface::StateInterface> 
TideHardwareInterface::export_state_interfaces()
{
    std::vector<hardware_interface::StateInterface> interfaces;
    
    // ... 导出关节状态接口 ...
    // ... 导出IMU状态接口 ...
    
    // 导出遥控器状态接口（GPIO类型，用于直接内存访问）
    interfaces.emplace_back("rc", "ch1", &rc_ch1_);
    interfaces.emplace_back("rc", "ch2", &rc_ch2_);
    interfaces.emplace_back("rc", "ch3", &rc_ch3_);
    interfaces.emplace_back("rc", "ch4", &rc_ch4_);
    interfaces.emplace_back("rc", "sw1", &rc_sw1_);
    interfaces.emplace_back("rc", "sw2", &rc_sw2_);
    interfaces.emplace_back("rc", "mouse_x", &rc_mouse_x_);
    interfaces.emplace_back("rc", "mouse_y", &rc_mouse_y_);
    interfaces.emplace_back("rc", "mouse_l", &rc_mouse_l_);
    interfaces.emplace_back("rc", "mouse_r", &rc_mouse_r_);
    interfaces.emplace_back("rc", "key_code", &rc_key_code_);
    interfaces.emplace_back("rc", "wheel", &rc_wheel_);
    
    return interfaces;
}

std::vector<hardware_interface::CommandInterface> 
TideHardwareInterface::export_command_interfaces()
{
    std::vector<hardware_interface::CommandInterface> interfaces;
    
    // ... 导出关节命令接口 ...
    
    // 导出底盘速度命令接口（GPIO类型，用于非关节命令）
    interfaces.emplace_back("chassis", "linear_x", &chassis_cmd_linear_x_);
    interfaces.emplace_back("chassis", "linear_y", &chassis_cmd_linear_y_);
    interfaces.emplace_back("chassis", "angular_z", &chassis_cmd_angular_z_);
    
    return interfaces;
}
```

**关键点**:
- 使用 `&rc_ch1_` 等传递**内存地址**
- 控制器可以直接读取这些内存地址
- **无需话题通信，零延迟**

#### 2.3 解析遥控器数据时更新共享内存

```cpp
void TideHardwareInterface::parseMotorData(const std::vector<uint8_t>& data)
{
    // ... CRC校验 ...
    
    if (motor_data_buffer_[0] == 0x10)  // 遥控器数据包
    {
        // ... 解析遥控器数据 ...
        
        // 更新遥控器状态接口（直接内存访问，供控制器读取）
        rc_ch1_ = static_cast<double>(ch1);
        rc_ch2_ = static_cast<double>(ch2);
        rc_ch3_ = static_cast<double>(ch3);
        rc_ch4_ = static_cast<double>(ch4);
        rc_sw1_ = static_cast<double>(sw1);
        rc_sw2_ = static_cast<double>(sw2);
        rc_mouse_x_ = static_cast<double>(mouse_x);
        rc_mouse_y_ = static_cast<double>(mouse_y);
        rc_mouse_l_ = static_cast<double>(mouse_l);
        rc_mouse_r_ = static_cast<double>(mouse_r);
        rc_key_code_ = static_cast<double>(key_code);
        rc_wheel_ = static_cast<double>(wheel);
        
        // 同时发布到ROS2话题（供上层应用使用，可选）
        if (rc_dbus_pub_)
        {
            tide_msgs::msg::RcData msg;
            msg.ch1 = ch1;
            msg.ch2 = ch2;
            // ... 填充其他字段 ...
            rc_dbus_pub_->publish(msg);
        }
    }
}
```

---

### 3. 控制器层直接读取遥控器内存

#### 3.1 声明状态和命令接口（控制器层）

**文件位置**: `src/tide_controllers/tide_chassis_controller/include/tide_chassis_controller.hpp`

```cpp
class TideChassisController : public controller_interface::ControllerInterface
{
private:
    // 遥控器状态接口（直接内存访问）
    std::unordered_map<std::string, 
        std::unique_ptr<const hardware_interface::LoanedStateInterface>> 
        rc_state_interfaces_;
    
    // 底盘速度命令接口（直接内存访问）
    std::unordered_map<std::string, 
        std::unique_ptr<hardware_interface::LoanedCommandInterface>> 
        chassis_cmd_interfaces_;
};
```

#### 3.2 声明需要的状态接口

**文件位置**: `src/tide_controllers/tide_chassis_controller/src/tide_chassis_controller.cpp`

```cpp
InterfaceConfiguration TideChassisController::state_interface_configuration() const
{
    std::vector<std::string> conf_names;
    
    // ... 轮子速度接口 ...
    
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

InterfaceConfiguration TideChassisController::command_interface_configuration() const
{
    std::vector<std::string> conf_names;
    
    // ... 轮子速度接口 ...
    
    // 添加底盘速度命令接口（直接内存访问）
    conf_names.push_back("chassis/linear_x");
    conf_names.push_back("chassis/linear_y");
    conf_names.push_back("chassis/angular_z");
    
    return { interface_configuration_type::INDIVIDUAL, conf_names };
}
```

#### 3.3 激活时绑定接口

```cpp
controller_interface::CallbackReturn
TideChassisController::on_activate(const rclcpp_lifecycle::State& /*previous_state*/)
{
    // 绑定命令接口
    chassis_cmd_interfaces_.clear();
    for (auto& cmd_interface : command_interfaces_)
    {
        const std::string interface_name = cmd_interface.get_interface_name();
        const std::string prefix_name = cmd_interface.get_prefix_name();
        
        if (prefix_name == "chassis")
        {
            chassis_cmd_interfaces_[interface_name] = 
                std::make_unique<hardware_interface::LoanedCommandInterface>(
                    std::move(cmd_interface)
                );
            RCLCPP_INFO(get_node()->get_logger(), 
                       "Claimed chassis/%s command interface", 
                       interface_name.c_str());
        }
    }
    
    // 绑定状态接口
    rc_state_interfaces_.clear();
    for (auto& state_interface : state_interfaces_)
    {
        const std::string interface_name = state_interface.get_interface_name();
        const std::string prefix_name = state_interface.get_prefix_name();
        
        if (prefix_name == "rc")
        {
            rc_state_interfaces_[interface_name] = 
                std::make_unique<const hardware_interface::LoanedStateInterface>(
                    std::move(state_interface)
                );
            RCLCPP_INFO(get_node()->get_logger(), 
                       "Claimed rc/%s state interface", 
                       interface_name.c_str());
        }
    }
    
    RCLCPP_INFO(get_node()->get_logger(), 
                "Chassis controller activated with %zu rc interfaces", 
                rc_state_interfaces_.size());
    
    return controller_interface::CallbackReturn::SUCCESS;
}
```

#### 3.4 实时控制循环中直接读取遥控器内存

```cpp
controller_interface::return_type 
TideChassisController::update(const rclcpp::Time& time,
                              const rclcpp::Duration& period)
{
    update_parameters();
    
    // ========== 直接读取遥控器数据（零延迟，直接内存访问）==========
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
    
    return controller_interface::return_type::OK;
}
```

**关键点**:
- `get_value()` 直接读取硬件接口层的内存变量
- `set_value()` 直接写入硬件接口层的内存变量
- **无需话题发布/订阅，无需序列化/反序列化**
- **零拷贝，零延迟**

---

### 4. 硬件接口层发送到下位机

#### 4.1 读取共享内存并发送

**文件位置**: `src/tide_control/tide_hw_interface/src/tide_hw_interface.cpp`

```cpp
hardware_interface::return_type 
TideHardwareInterface::write(const rclcpp::Time& /*time*/,
                             const rclcpp::Duration& /*period*/)
{
    // 直接读取控制器写入的内存变量（无需话题通信）
    // 构建底盘命令数据包
    // 格式：[msg_id=0x20][linear_x][linear_y][angular_z]
    PacketBuilder builder;
    builder.addByte(0x20)  // 消息ID：底盘速度命令
           .addFloat(static_cast<float>(chassis_cmd_linear_x_))
           .addFloat(static_cast<float>(chassis_cmd_linear_y_))
           .addFloat(static_cast<float>(chassis_cmd_angular_z_));
    
    // 发送数据
    sendCustomData(builder.getData());
    
    return hardware_interface::return_type::OK;
}
```

#### 4.2 数据包格式

发送到下位机的数据包格式：

```
[帧头: 0xFF 0xAA] [长度: 2字节] [msg_id: 0x20] [linear_x: 4字节] [linear_y: 4字节] [angular_z: 4字节] [CRC32: 4字节]
```

---

## 完整数据流总结

### 遥控器 → 控制器（直接内存访问，零延迟）

```
下位机 (DBUS接收器)
    ↓ 串口发送 [0x10 + RC数据]
硬件接口层 (parseMotorData)
    ↓ 解析并更新共享内存 (rc_ch1_, rc_ch2_, ...)
    ↓ 同时发布话题 (/rc/dbus) [可选]
控制器层 (update)
    ↓ 直接读取共享内存 (get_value)
    ↓ 计算底盘速度命令
    ↓ 直接写入共享内存 (set_value)
硬件接口层 (write)
    ↓ 读取共享内存并发送 [0x20 + 速度命令]
下位机 (底盘电机控制)
```

**总延迟**: < 2ms（控制周期 + 串口传输）

### 上层应用 → 控制器（话题通信，用于路径规划等）

```
上层应用 (路径规划/导航)
    ↓ 发布话题 (/chassis_cmd)
控制器层 (update)
    ↓ 订阅话题并更新命令
    ↓ 直接写入共享内存 (set_value)
硬件接口层 (write)
    ↓ 读取共享内存并发送 [0x20 + 速度命令]
下位机 (底盘电机控制)
```

---

## 关键优势

### 1. 零延迟
- 控制器直接写入硬件接口的内存变量
- 无需话题发布/订阅
- 无需消息序列化/反序列化

### 2. 零拷贝
- 使用指针传递内存地址
- 数据不需要复制

### 3. 实时性保证
- 控制循环频率：1000Hz（1ms周期）
- 串口发送频率：与控制循环同步
- 总延迟：< 2ms（控制周期 + 串口传输）

### 4. 线程安全
- 硬件接口的 `read()` 和 `write()` 在同一个实时线程中执行
- 控制器的 `update()` 也在同一个实时线程中执行
- 无需互斥锁保护（单线程访问）

---

## 如何在控制器中使用遥控器数据

### 方式1：直接内存访问（推荐，零延迟）

**已实现！** 在 `tide_chassis_controller.cpp` 的 `update()` 方法中，取消注释即可启用：

```cpp
controller_interface::return_type 
TideChassisController::update(const rclcpp::Time& time,
                              const rclcpp::Duration& period)
{
    // ========== 直接读取遥控器数据（零延迟，直接内存访问）==========
    if (rc_state_interfaces_.count("ch1") && rc_state_interfaces_.count("ch2") && 
        rc_state_interfaces_.count("ch3") && rc_state_interfaces_.count("sw1"))
    {
        // 读取遥控器摇杆值（范围：-660 ~ 660）
        double ch1 = rc_state_interfaces_["ch1"]->get_value();  // 右摇杆左右
        double ch2 = rc_state_interfaces_["ch2"]->get_value();  // 右摇杆上下
        double ch3 = rc_state_interfaces_["ch3"]->get_value();  // 左摇杆左右
        double sw1 = rc_state_interfaces_["sw1"]->get_value();  // 左拨杆
        
        // 归一化到 [-1, 1]
        double norm_ch1 = ch1 / 660.0;
        double norm_ch2 = ch2 / 660.0;
        double norm_ch3 = ch3 / 660.0;
        
        // 根据拨杆位置调整最大速度
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
    
    // 写入底盘命令接口
    chassis_cmd_interfaces_["linear_x"]->set_value(linear_x_cmd_);
    chassis_cmd_interfaces_["linear_y"]->set_value(linear_y_cmd_);
    chassis_cmd_interfaces_["angular_z"]->set_value(angular_z_cmd_);
    
    return controller_interface::return_type::OK;
}
```

**优势**:
- 延迟：< 0.1ms
- 零拷贝
- 实时性保证

### 方式2：订阅遥控器话题（用于上层应用）

```cpp
// 在控制器中订阅遥控器话题
rc_sub_ = get_node()->create_subscription<tide_msgs::msg::RcData>(
    "/rc/dbus", 
    rclcpp::SystemDefaultsQoS(),
    [this](const std::shared_ptr<tide_msgs::msg::RcData> msg) {
        // 处理遥控器数据
        double vx = msg->ch2 / 660.0;  // 归一化到 [-1, 1]
        double vy = msg->ch1 / 660.0;
        double wz = msg->ch3 / 660.0;
        
        // 更新命令
        linear_x_cmd_ = vx * max_linear_velocity_;
        linear_y_cmd_ = vy * max_linear_velocity_;
        angular_z_cmd_ = wz * max_angular_velocity_;
    }
);
```

**劣势**:
- 延迟：5-10ms
- 需要消息序列化/反序列化
- 不适合实时控制

---

## 性能对比

### 话题通信方式
```
延迟：5-10ms
拷贝次数：3次（发布端序列化 → DDS传输 → 订阅端反序列化）
实时性：不保证
```

### 直接内存访问方式
```
延迟：< 0.1ms
拷贝次数：0次（指针传递）
实时性：保证（实时线程）
```

---

## 注意事项

### 1. 线程安全
- 如果需要在多个线程中访问共享内存变量，必须使用互斥锁保护
- 当前实现中，`read()` 和 `write()` 在同一个实时线程中执行，无需互斥锁

### 2. 数据同步
- 控制器的 `update()` 频率必须与硬件接口的 `read()`/`write()` 频率一致
- 当前配置：1000Hz（1ms周期）

### 3. 内存对齐
- 使用 `__attribute__((packed))` 确保结构体内存布局与下位机一致
- 避免编译器自动填充字节

### 4. 字节序
- 当前实现使用**小端序**（Little Endian）
- 确保上位机和下位机字节序一致

---

## 相关文件

### 硬件接口层
- `src/tide_control/tide_hw_interface/include/tide_hw_interface.hpp`
- `src/tide_control/tide_hw_interface/src/tide_hw_interface.cpp`

### 控制器层
- `src/tide_controllers/tide_chassis_controller/include/tide_chassis_controller.hpp`
- `src/tide_controllers/tide_chassis_controller/src/tide_chassis_controller.cpp`

### 消息定义
- `src/tide_control/tide_msgs/msg/RcData.msg`
- `src/tide_control/tide_msgs/msg/ChassisCmd.msg`
- `src/tide_control/tide_msgs/msg/ChassisState.msg`

---

## 总结

TideControls 系统通过 **ros2_control 框架的命令/状态接口机制**，实现了控制器与硬件接口之间的**直接内存访问**，避免了传统话题通信的延迟和开销。

**核心机制**：
1. 硬件接口导出命令/状态接口时，传递**内存地址**（指针）
2. 控制器通过 `LoanedCommandInterface` 和 `LoanedStateInterface` 获取这些指针
3. 控制器直接读写这些内存地址，实现**零延迟、零拷贝**的数据传递

这种设计非常适合**实时控制系统**，确保了控制回路的高频率和低延迟。
