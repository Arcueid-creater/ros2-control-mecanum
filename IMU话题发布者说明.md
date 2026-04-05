# IMU话题发布者说明

## 概述

目前系统中有**两个**IMU相关的ROS2话题，它们由**不同的发布者**发布：

---

## 1. `/imu/rpy` 话题

### 发布者
**TideHardwareInterface** (在 `tide_hw_interface.cpp` 中)

### 发布位置
`TideHardwareInterface::read()` 函数（主线程）

### 代码位置
```cpp
// 文件：src/tide_control/tide_hw_interface/src/tide_hw_interface.cpp
// 函数：hardware_interface::return_type TideHardwareInterface::read(...)

// 方案三：在主线程中发布RPY话题
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
```

### 消息类型
`geometry_msgs/msg/Vector3Stamped`

### 数据内容
- **x**: Roll（横滚角，单位：度）
- **y**: Pitch（俯仰角，单位：度）
- **z**: Yaw（偏航角，单位：度）

### 发布频率
与 `read()` 函数调用频率相同（通常是控制器更新频率，如 100Hz）

### 节点名称
`tide_hw_imu_node`（在 `on_init()` 中创建）

### 配置参数
在URDF的 `<ros2_control>` 标签中配置：
```xml
<param name="publish_rpy">true</param>
<param name="imu_frame_id">imu_link</param>
```

---

## 2. `/imu_sensor_broadcaster/imu` 话题

### 发布者
**IMUSensorBroadcaster** (ros2_control 控制器)

### 发布机制
IMUSensorBroadcaster 是一个 ros2_control 控制器，它：
1. 读取 TideHardwareInterface 导出的**状态接口**（State Interfaces）
2. 将这些状态接口的数据组装成标准的 `sensor_msgs/Imu` 消息
3. 发布到 `/imu_sensor_broadcaster/imu` 话题

### 状态接口定义
在URDF中定义（`infantry_real.xacro` 等）：
```xml
<sensor name="imu_sensor">
    <!-- Quaternion (4) -->
    <state_interface name="orientation.x"/>
    <state_interface name="orientation.y"/>
    <state_interface name="orientation.z"/>
    <state_interface name="orientation.w"/>
    
    <!-- Euler angles (3) -->
    <state_interface name="rpy.roll"/>
    <state_interface name="rpy.pitch"/>
    <state_interface name="rpy.yaw"/>
    
    <!-- Angular velocity (3) -->
    <state_interface name="angular_velocity.x"/>
    <state_interface name="angular_velocity.y"/>
    <state_interface name="angular_velocity.z"/>
    
    <!-- Linear acceleration (3) -->
    <state_interface name="linear_acceleration.x"/>
    <state_interface name="linear_acceleration.y"/>
    <state_interface name="linear_acceleration.z"/>
</sensor>
```

### ⚠️ 重要发现：状态接口导出缺失

**当前问题：** `TideHardwareInterface::export_state_interfaces()` 函数中**没有导出IMU传感器的状态接口**！

当前代码只导出了关节（joint）的状态接口：
```cpp
std::vector<hardware_interface::StateInterface> TideHardwareInterface::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> interfaces;

  // 只导出了关节状态接口
  for (size_t i = 0; i < joint_count; i++)
  {
    // ... 导出 position, velocity, effort
  }
  
  // ❌ 缺少：IMU传感器状态接口的导出！
  
  return interfaces;
}
```

**需要添加的代码：**
```cpp
std::vector<hardware_interface::StateInterface> TideHardwareInterface::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> interfaces;

  // 导出关节状态接口
  for (size_t i = 0; i < joint_count; i++)
  {
    // ... 现有代码 ...
  }
  
  // ✅ 添加：导出IMU传感器状态接口
  for (const auto& sensor : info_.sensors)
  {
    for (const auto& state_interface : sensor.state_interfaces)
    {
      if (state_interface.name == "orientation.x")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_orientation[1]); // X
      else if (state_interface.name == "orientation.y")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_orientation[2]); // Y
      else if (state_interface.name == "orientation.z")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_orientation[3]); // Z
      else if (state_interface.name == "orientation.w")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_orientation[0]); // W
      
      else if (state_interface.name == "rpy.roll")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_rpy_[0]);
      else if (state_interface.name == "rpy.pitch")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_rpy_[1]);
      else if (state_interface.name == "rpy.yaw")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_rpy_[2]);
      
      else if (state_interface.name == "angular_velocity.x")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_gyro_[0]);
      else if (state_interface.name == "angular_velocity.y")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_gyro_[1]);
      else if (state_interface.name == "angular_velocity.z")
        interfaces.emplace_back(sensor.name, state_interface.name, &imu_gyro_[2]);
      
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
```

### 消息类型
`sensor_msgs/msg/Imu`

### 数据内容
完整的IMU数据：
- **orientation**: 四元数姿态（x, y, z, w）
- **angular_velocity**: 角速度（x, y, z，单位：rad/s）
- **linear_acceleration**: 线性加速度（x, y, z，单位：m/s²）

### 发布频率
由 IMUSensorBroadcaster 配置决定（通常与控制器更新频率相同）

### 配置文件
`src/tide_control/tide_ctrl_bringup/config/imu_sensor_broadcaster.yaml`：
```yaml
imu_sensor_broadcaster:
    ros__parameters:
        sensor_name: imu_sensor
        frame_id: imu_link
```

### 启动方式
在 launch 文件中通过 `spawner` 启动：
```python
Node(
    package='controller_manager',
    executable='spawner',
    arguments=['imu_sensor_broadcaster',
               '--param-file', imu_broadcaster_config],
)
```

---

## 数据流程图

```
┌─────────────────────────────────────────────────────────────────┐
│                    串口线程（后台）                               │
│                                                                   │
│  SerialDevice::run()                                             │
│       ↓                                                          │
│  接收IMU原始数据                                                  │
│       ↓                                                          │
│  parseImuSerialData()  ← 解析数据包                              │
│       ↓                                                          │
│  parseImuData()        ← 提取IMU数据                             │
│       ↓                                                          │
│  更新内存变量（加锁）：                                           │
│    - imu_orientation[4]  (四元数)                                │
│    - imu_rpy_[3]         (欧拉角，弧度)                          │
│    - imu_gyro_[3]        (角速度，rad/s)                         │
│    - imu_accel_[3]       (加速度，m/s²)                          │
└─────────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────────┐
│                    主线程（ros2_control）                         │
│                                                                   │
│  TideHardwareInterface::read()                                   │
│       ↓                                                          │
│  【路径1】直接发布 /imu/rpy 话题                                  │
│       ↓                                                          │
│  读取 imu_rpy_[3]（加锁）                                        │
│       ↓                                                          │
│  转换为角度（* 180/π）                                           │
│       ↓                                                          │
│  rpy_pub_->publish()  → /imu/rpy                                │
│                                                                   │
│  【路径2】通过状态接口导出                                        │
│       ↓                                                          │
│  export_state_interfaces() ← ⚠️ 当前缺失！需要添加               │
│       ↓                                                          │
│  导出13个状态接口：                                               │
│    - imu_sensor/orientation.{x,y,z,w}                           │
│    - imu_sensor/rpy.{roll,pitch,yaw}                            │
│    - imu_sensor/angular_velocity.{x,y,z}                        │
│    - imu_sensor/linear_acceleration.{x,y,z}                     │
└─────────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────────┐
│              IMUSensorBroadcaster（控制器）                       │
│                                                                   │
│  update()                                                        │
│       ↓                                                          │
│  读取状态接口（零延迟）                                           │
│       ↓                                                          │
│  组装 sensor_msgs::Imu 消息                                      │
│       ↓                                                          │
│  imu_pub_->publish()  → /imu_sensor_broadcaster/imu             │
└─────────────────────────────────────────────────────────────────┘
```

---

## 两个话题的对比

| 特性 | `/imu/rpy` | `/imu_sensor_broadcaster/imu` |
|------|-----------|-------------------------------|
| **发布者** | TideHardwareInterface | IMUSensorBroadcaster |
| **发布位置** | read() 函数 | broadcaster update() |
| **消息类型** | Vector3Stamped | Imu |
| **数据内容** | 欧拉角（度） | 完整IMU数据 |
| **单位** | 度（°） | 弧度（rad）和 m/s² |
| **用途** | 易读的欧拉角 | 标准IMU数据 |
| **Foxglove** | 可直接显示角度 | 可显示3D姿态 |
| **当前状态** | ✅ 正常工作 | ⚠️ 需要添加状态接口导出 |

---

## 修复建议

### 问题
IMUSensorBroadcaster 无法发布数据，因为 `export_state_interfaces()` 没有导出IMU传感器的状态接口。

### 解决方案
在 `tide_hw_interface.cpp` 的 `export_state_interfaces()` 函数中添加IMU传感器状态接口的导出代码（见上文"需要添加的代码"部分）。

### 验证步骤
1. 添加代码后重新编译
2. 启动系统
3. 检查话题：
   ```bash
   ros2 topic list
   # 应该看到：
   # /imu/rpy
   # /imu_sensor_broadcaster/imu
   ```
4. 查看数据：
   ```bash
   ros2 topic echo /imu_sensor_broadcaster/imu
   ```

---

## 总结

1. **`/imu/rpy`** 由 **TideHardwareInterface** 在 `read()` 函数中直接发布
   - ✅ 当前正常工作
   - 数据来源：直接读取 `imu_rpy_[]` 内存变量

2. **`/imu_sensor_broadcaster/imu`** 由 **IMUSensorBroadcaster** 控制器发布
   - ⚠️ 当前无法工作（缺少状态接口导出）
   - 数据来源：通过状态接口读取 `imu_orientation[]`, `imu_rpy_[]`, `imu_gyro_[]`, `imu_accel_[]`

3. **根本原因**：IMU数据由串口线程接收并解析，存储在内存变量中，但这些变量没有通过状态接口导出给 IMUSensorBroadcaster。

4. **修复方法**：在 `export_state_interfaces()` 中添加IMU传感器状态接口的导出代码。
