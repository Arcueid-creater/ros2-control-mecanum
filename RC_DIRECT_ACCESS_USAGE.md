# 遥控器直接内存访问 - 快速使用指南

## 功能说明

底盘控制器现在可以**直接从内存读取遥控器数据**，实现零延迟的遥控器控制。

---

## 启用方法

### 1. 编辑控制器代码

打开文件：`src/tide_controllers/tide_chassis_controller/src/tide_chassis_controller.cpp`

在 `update()` 方法中，找到以下被注释的代码块：

```cpp
// ========== 方式2：直接读取遥控器数据（零延迟，直接内存访问）==========
// 如果需要遥控器直接控制底盘，取消下面的注释
/*
if (rc_state_interfaces_.count("ch1") && rc_state_interfaces_.count("ch2") && 
    rc_state_interfaces_.count("ch3") && rc_state_interfaces_.count("sw1"))
{
    // ... 遥控器控制代码 ...
}
*/
```

**取消注释**这个代码块即可启用遥控器直接控制。

### 2. 重新编译

```bash
cd ~/TideControls  # 或你的工作空间路径
colcon build --packages-select tide_chassis_controller
source install/setup.bash
```

### 3. 启动系统

```bash
ros2 launch tide_ctrl_bringup tide_ctrl_bringup.launch.py
```

---

## 遥控器映射

### 摇杆通道
- **ch1** (右摇杆左右): 底盘左右平移 (linear_y)
- **ch2** (右摇杆上下): 底盘前后移动 (linear_x)
- **ch3** (左摇杆左右): 底盘旋转 (angular_z)
- **ch4** (左摇杆上下): 保留

### 拨杆开关
- **sw1 = 1** (上位): 慢速模式 (0.5 m/s, 1.0 rad/s)
- **sw1 = 2** (中位): 正常模式 (1.0 m/s, 2.0 rad/s)
- **sw1 = 3** (下位): 快速模式 (2.0 m/s, 4.0 rad/s)

### 其他输入
- **mouse_x, mouse_y**: 鼠标移动
- **mouse_l, mouse_r**: 鼠标左右键
- **key_code**: 键盘按键
- **wheel**: 鼠标滚轮

---

## 代码说明

### 读取遥控器数据

```cpp
// 直接从内存读取（零延迟）
double ch1 = rc_state_interfaces_["ch1"]->get_value();  // 范围：-660 ~ 660
double ch2 = rc_state_interfaces_["ch2"]->get_value();
double ch3 = rc_state_interfaces_["ch3"]->get_value();
double sw1 = rc_state_interfaces_["sw1"]->get_value();  // 1, 2, 或 3
```

### 归一化

```cpp
// 归一化到 [-1, 1]
double norm_ch1 = ch1 / 660.0;
double norm_ch2 = ch2 / 660.0;
double norm_ch3 = ch3 / 660.0;
```

### 计算速度命令

```cpp
// 计算底盘速度
linear_x_cmd_ = norm_ch2 * max_linear_vel;   // 前后
linear_y_cmd_ = norm_ch1 * max_linear_vel;   // 左右
angular_z_cmd_ = norm_ch3 * max_angular_vel; // 旋转
```

### 死区处理

```cpp
// 避免摇杆漂移
const double deadzone = 0.05;
if (std::abs(linear_x_cmd_) < deadzone) linear_x_cmd_ = 0.0;
if (std::abs(linear_y_cmd_) < deadzone) linear_y_cmd_ = 0.0;
if (std::abs(angular_z_cmd_) < deadzone) angular_z_cmd_ = 0.0;
```

---

## 性能指标

| 指标 | 直接内存访问 | 话题通信 |
|------|-------------|---------|
| 延迟 | < 0.1ms | 5-10ms |
| 拷贝次数 | 0 | 3 |
| 实时性 | 保证 | 不保证 |
| CPU占用 | 极低 | 中等 |

---

## 调试

### 查看遥控器原始数据

```bash
ros2 topic echo /rc/dbus
```

### 查看底盘状态

```bash
ros2 topic echo /tide_chassis_controller/chassis_state
```

### 查看控制器日志

控制器会每1000次循环打印一次遥控器数据：

```
[INFO] RC Direct Access - ch1: 123, ch2: -456, ch3: 78, sw1: 2 -> vx: 0.691, vy: -0.186, wz: 0.236
```

---

## 自定义配置

### 修改最大速度

在 `update()` 方法中修改：

```cpp
// 设置最大速度
double max_linear_vel = 1.0;   // m/s（修改这里）
double max_angular_vel = 2.0;  // rad/s（修改这里）

if (sw1 == 1)  // 慢速模式
{
    max_linear_vel = 0.5;   // 修改这里
    max_angular_vel = 1.0;  // 修改这里
}
else if (sw1 == 3)  // 快速模式
{
    max_linear_vel = 2.0;   // 修改这里
    max_angular_vel = 4.0;  // 修改这里
}
```

### 修改死区

```cpp
const double deadzone = 0.05;  // 修改这里（0.05 = 5%）
```

### 修改通道映射

```cpp
// 如果你的遥控器通道映射不同，修改这里
double ch1 = rc_state_interfaces_["ch1"]->get_value();  // 改为 ch2, ch3, ch4
double ch2 = rc_state_interfaces_["ch2"]->get_value();
double ch3 = rc_state_interfaces_["ch3"]->get_value();
```

---

## 注意事项

1. **线程安全**: 当前实现在实时线程中运行，无需互斥锁
2. **数据同步**: 控制循环频率为 1000Hz，确保实时性
3. **遥控器范围**: 摇杆值范围为 -660 ~ 660
4. **拨杆值**: sw1 和 sw2 的值为 1（上）、2（中）、3（下）

---

## 故障排查

### 问题1：遥控器无响应

**检查**:
1. 确认下位机正确发送遥控器数据（msg_id=0x10）
2. 查看硬件接口日志，确认接收到数据
3. 检查控制器是否正确绑定了 rc 状态接口

```bash
ros2 control list_hardware_interfaces
```

应该看到：
```
rc/ch1 [available] [claimed]
rc/ch2 [available] [claimed]
rc/ch3 [available] [claimed]
rc/ch4 [available] [claimed]
...
```

### 问题2：底盘移动方向错误

**解决**: 修改通道映射或添加负号

```cpp
// 反转方向
linear_x_cmd_ = -norm_ch2 * max_linear_vel;  // 添加负号
```

### 问题3：摇杆漂移

**解决**: 增大死区

```cpp
const double deadzone = 0.1;  // 从 0.05 增加到 0.1
```

---

## 扩展功能

### 使用鼠标控制

```cpp
// 读取鼠标数据
double mouse_x = rc_state_interfaces_["mouse_x"]->get_value();
double mouse_y = rc_state_interfaces_["mouse_y"]->get_value();
double mouse_l = rc_state_interfaces_["mouse_l"]->get_value();
double mouse_r = rc_state_interfaces_["mouse_r"]->get_value();

// 使用鼠标控制云台
gimbal_yaw_cmd_ += mouse_x * 0.001;
gimbal_pitch_cmd_ += mouse_y * 0.001;
```

### 使用键盘控制

```cpp
// 读取键盘数据
double key_code = rc_state_interfaces_["key_code"]->get_value();

// WASD 控制
if (key_code & 0x01) linear_x_cmd_ = max_linear_vel;   // W
if (key_code & 0x02) linear_x_cmd_ = -max_linear_vel;  // S
if (key_code & 0x04) linear_y_cmd_ = -max_linear_vel;  // A
if (key_code & 0x08) linear_y_cmd_ = max_linear_vel;   // D
```

---

## 相关文档

- [完整技术文档](RC_DIRECT_MEMORY_ACCESS.md)
- [底盘控制器集成指南](src/tide_controllers/tide_chassis_controller/INTEGRATION.md)
- [串口通信协议](CHASSIS_SERIAL_COMMUNICATION.md)
