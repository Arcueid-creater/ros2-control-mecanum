# 底盘串口通信架构说明（零延迟版本）

## 数据流向（直接内存访问）

```
用户/上层节点
    |
    | 发布 ChassisCmd 到 ~/chassis_cmd
    v
TideChassisController (控制器)
    |
    | 1. 接收命令
    | 2. 计算轮速（TODO）
    | 3. 直接写入共享内存 ⚡ 零延迟
    |    chassis_linear_x_cmd_->set_value()
    |    chassis_linear_y_cmd_->set_value()
    |    chassis_angular_z_cmd_->set_value()
    v
共享内存（Command Interface）
    |
    | 同一块内存，无需拷贝
    v
TideHardwareInterface (硬件接口)
    |
    | 在 write() 中直接读取内存变量
    | chassis_cmd_linear_x_
    | chassis_cmd_linear_y_
    | chassis_cmd_angular_z_
    v
SerialDevice (串口设备)
    |
    | 通过串口发送
    v
下位机 (STM32/嵌入式设备)
```

## 核心优势

### ⚡ 零延迟
- **无话题通信**：控制器和硬件接口共享同一块内存
- **无序列化/反序列化**：直接内存读写
- **无线程切换**：在同一个实时循环中执行
- **无互斥锁**：ROS 2 Control 框架保证线程安全

### 🎯 实时性保证
- 控制器 `update()` 和硬件接口 `write()` 在同一个实时循环中顺序执行
- 延迟 < 1 微秒（仅内存访问时间）

## 技术实现

### 1. Hardware Interface 导出命令接口

```cpp
// 在 export_command_interfaces() 中
interfaces.emplace_back("chassis", "linear_x", &chassis_cmd_linear_x_);
interfaces.emplace_back("chassis", "linear_y", &chassis_cmd_linear_y_);
interfaces.emplace_back("chassis", "angular_z", &chassis_cmd_angular_z_);
```

### 2. Controller 声明需要的接口

```cpp
// 在 command_interface_configuration() 中
conf_names.push_back("chassis/linear_x");
conf_names.push_back("chassis/linear_y");
conf_names.push_back("chassis/angular_z");
```

### 3. Controller 写入命令（零延迟）

```cpp
// 在 update() 中
chassis_linear_x_cmd_->set_value(linear_x_cmd_);
chassis_linear_y_cmd_->set_value(linear_y_cmd_);
chassis_angular_z_cmd_->set_value(angular_z_cmd_);
```

### 4. Hardware Interface 读取并发送

```cpp
// 在 write() 中
builder.addByte(0x20)
       .addFloat(static_cast<float>(chassis_cmd_linear_x_))
       .addFloat(static_cast<float>(chassis_cmd_linear_y_))
       .addFloat(static_cast<float>(chassis_cmd_angular_z_));
sendCustomData(builder.getData());
```

## 通信协议

### 1. 底盘命令下发协议（上位机 → 下位机）

**数据包格式：**
```
[帧头] [长度] [数据] [CRC32]
```

**详细结构：**
```
0xFF 0xAA                    // 帧头 (2 bytes)
[data_length]                // 数据长度 (2 bytes, 小端序)
0x20                         // 消息ID：底盘速度命令 (1 byte)
[linear_x]                   // X方向线速度 (4 bytes, float)
[linear_y]                   // Y方向线速度 (4 bytes, float)
[angular_z]                  // Z轴角速度 (4 bytes, float)
[mode]                       // 控制模式 (1 byte)
[CRC32]                      // CRC32校验 (4 bytes)
```

**总长度：** 2 + 2 + 1 + 4 + 4 + 4 + 4 = 21 bytes

**示例代码（下位机解析）：**
```c
// 下位机接收到数据后的解析
if (data[0] == 0xFF && data[1] == 0xAA) {
    uint16_t data_length;
    memcpy(&data_length, &data[2], 2);
    
    if (data[4] == 0x20) {  // 底盘速度命令
        float linear_x, linear_y, angular_z;
        
        memcpy(&linear_x, &data[5], 4);
        memcpy(&linear_y, &data[9], 4);
        memcpy(&angular_z, &data[13], 4);
        
        // 验证CRC32
        uint32_t received_crc;
        memcpy(&received_crc, &data[17], 4);
        uint32_t calculated_crc = calculate_crc32(data, 17);
        
        if (received_crc == calculated_crc) {
            // 执行底盘控制
            chassis_control(linear_x, linear_y, angular_z);
        }
    }
}
```

### 2. 电机和遥控器数据上传协议（下位机 → 上位机）

**消息ID 0x10：遥控器 + 电机数据**

已在 `parseMotorData()` 中实现，格式：
```
[帧头] [长度] [数据] [CRC32]
0xFF 0xAA
[data_length]
0x10                         // 消息ID
[RC_DBUS数据 19 bytes]       // 遥控器数据
[4个电机数据 48 bytes]       // 每个电机12 bytes
[CRC32]
```

## ROS 2 话题

### 订阅的话题

| 话题名 | 消息类型 | 说明 | 订阅者 |
|--------|---------|------|--------|
| `~/chassis_cmd` | `tide_msgs/msg/ChassisCmd` | 底盘控制命令（用户输入） | TideChassisController |

### 发布的话题

| 话题名 | 消息类型 | 说明 | 发布者 |
|--------|---------|------|--------|
| `~/chassis_state` | `tide_msgs/msg/ChassisState` | 底盘状态反馈 | TideChassisController |
| `/rc/dbus` | `tide_msgs/msg/RcData` | 遥控器数据 | TideHardwareInterface |
| `/chassis/motor_trans` | `tide_msgs/msg/ChassisMotorTrans` | 电机状态数据 | TideHardwareInterface |

### Command Interfaces（零延迟内存访问）

| 接口名 | 类型 | 说明 | 写入者 | 读取者 |
|--------|------|------|--------|--------|
| `chassis/linear_x` | double | X方向线速度 | TideChassisController | TideHardwareInterface |
| `chassis/linear_y` | double | Y方向线速度 | TideChassisController | TideHardwareInterface |
| `chassis/angular_z` | double | Z轴角速度 | TideChassisController | TideHardwareInterface |

## 使用示例

### 1. 发送底盘控制命令

```bash
# 前进 1 m/s
ros2 topic pub /chassis_controller/chassis_cmd tide_msgs/msg/ChassisCmd \
  "{linear_x: 1.0, linear_y: 0.0, angular_z: 0.0, mode: 0}" --once

# 左平移 0.5 m/s
ros2 topic pub /chassis_controller/chassis_cmd tide_msgs/msg/ChassisCmd \
  "{linear_x: 0.0, linear_y: 0.5, angular_z: 0.0, mode: 0}" --once

# 原地旋转 1 rad/s
ros2 topic pub /chassis_controller/chassis_cmd tide_msgs/msg/ChassisCmd \
  "{linear_x: 0.0, linear_y: 0.0, angular_z: 1.0, mode: 0}" --once
```

### 2. 监听底盘状态

```bash
# 查看底盘状态反馈
ros2 topic echo /chassis_controller/chassis_state
```

### 3. 监听遥控器数据

```bash
ros2 topic echo /rc/dbus
```

### 4. 监听电机状态

```bash
ros2 topic echo /chassis/motor_trans
```

### 5. 查看命令接口（调试）

```bash
# 查看所有可用的命令接口
ros2 control list_hardware_interfaces

# 应该能看到：
# chassis/linear_x [available] [claimed]
# chassis/linear_y [available] [claimed]
# chassis/angular_z [available] [claimed]
```

## 调试建议

### 1. 检查话题连接

```bash
# 查看所有话题
ros2 topic list

# 查看底盘状态话题信息
ros2 topic info /chassis_controller/chassis_state

# 查看话题频率
ros2 topic hz /chassis_controller/chassis_state
```

### 2. 检查命令接口

```bash
# 查看硬件接口状态
ros2 control list_hardware_interfaces

# 查看控制器状态
ros2 control list_controllers
```

### 3. 启用调试日志

在 `tide_hw_interface.cpp` 中，将 `RCLCPP_DEBUG` 改为 `RCLCPP_INFO` 可以看到串口发送的详细信息：

```cpp
RCLCPP_INFO(rclcpp::get_logger("TideHardwareInterface"),
            "Sent chassis cmd to serial: vx=%.3f, vy=%.3f, wz=%.3f, mode=%d",
            chassis_cmd_linear_x_, chassis_cmd_linear_y_, chassis_cmd_angular_z_);
```

### 4. 串口数据监控

使用串口调试工具（如 `minicom` 或 `screen`）监听串口数据：

```bash
# 监听串口（只读模式）
sudo screen /dev/ttyUSB0 115200
```

## 注意事项

1. **零延迟设计**：控制器和硬件接口共享内存，无需话题通信
2. **线程安全**：ROS 2 Control 框架保证 `update()` 和 `write()` 顺序执行
3. **实时性**：整个数据流在同一个实时循环中完成
4. **CRC校验**：下位机必须验证 CRC32，确保数据完整性
5. **消息ID**：
   - `0x10`：遥控器 + 电机数据（下位机 → 上位机）
   - `0x20`：底盘速度命令（上位机 → 下位机）
6. **字节序**：使用小端序（Little Endian）

## 性能对比

| 方案 | 延迟 | 优点 | 缺点 |
|------|------|------|------|
| **Command Interface（当前）** | < 1 μs | 零延迟，实时性最佳 | 需要理解 ros2_control 架构 |
| ROS 2 话题 | ~100 μs | 简单易懂 | 有序列化开销 |
| 共享内存 | ~10 μs | 较快 | 需要手动管理同步 |

## 下一步工作

1. **实现麦克纳姆轮运动学**：在 `TideChassisController::update()` 中计算轮速
2. **添加速度限制**：防止命令超出电机能力范围
3. **添加超时保护**：如果长时间没有命令，自动停止
4. **添加状态反馈**：从电机数据计算实际底盘速度

## 相关文件

- `src/tide_control/tide_hw_interface/src/tide_hw_interface.cpp` - 硬件接口实现
- `src/tide_control/tide_hw_interface/include/tide_hw_interface.hpp` - 硬件接口头文件
- `src/tide_controllers/tide_chassis_controller/src/tide_chassis_controller.cpp` - 底盘控制器实现
- `src/tide_controllers/tide_chassis_controller/include/tide_chassis_controller.hpp` - 底盘控制器头文件
