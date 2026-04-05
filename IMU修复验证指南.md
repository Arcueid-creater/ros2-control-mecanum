# IMU修复验证指南

## 快速验证步骤

### 1. 重新编译（必须）

```bash
cd ~/TideControls  # 或你的工作空间路径
colcon build --packages-select tide_hw_interface
source install/setup.bash
```

### 2. 启动系统

```bash
ros2 launch tide_ctrl_bringup tide_ctrl_bringup.launch.py robot_type:=infantry mode:=real
```

### 3. 快速检查（一条命令）

```bash
# 检查两个IMU话题是否都有数据
ros2 topic hz /imu/rpy & ros2 topic hz /imu_sensor_broadcaster/imu
```

**预期输出**:
```
/imu/rpy: 100 Hz (或你的控制频率)
/imu_sensor_broadcaster/imu: 100 Hz (或你的控制频率)
```

### 4. 查看数据（验证修复成功）

```bash
# 查看完整IMU数据（这个话题之前没有数据，现在应该有了）
ros2 topic echo /imu_sensor_broadcaster/imu --once
```

**如果看到类似以下输出，说明修复成功**:
```yaml
header:
  stamp:
    sec: 1234567890
    nanosec: 123456789
  frame_id: imu_link
orientation:
  x: 0.0
  y: 0.0
  z: 0.0
  w: 1.0
angular_velocity:
  x: 0.0
  y: 0.0
  z: 0.0
linear_acceleration:
  x: 0.0
  y: 0.0
  z: 9.81
orientation_covariance: [0.0, 0.0, ...]
angular_velocity_covariance: [0.0, 0.0, ...]
linear_acceleration_covariance: [0.0, 0.0, ...]
---
```

---

## 详细验证

### 验证1: 检查控制器状态

```bash
ros2 control list_controllers
```

**预期输出**:
```
imu_sensor_broadcaster[imu_sensor_broadcaster/IMUSensorBroadcaster] active  ← 应该是 active
joint_state_broadcaster[joint_state_broadcaster/JointStateBroadcaster] active
...
```

### 验证2: 检查状态接口

```bash
ros2 control list_hardware_interfaces
```

**预期输出（应该包含13个IMU状态接口）**:
```
command interfaces:
  joint1/position [available] [claimed]
  joint2/position [available] [claimed]
  ...

state interfaces:
  joint1/position
  joint1/velocity
  joint1/effort
  ...
  imu_sensor/orientation.x          ← 新增
  imu_sensor/orientation.y          ← 新增
  imu_sensor/orientation.z          ← 新增
  imu_sensor/orientation.w          ← 新增
  imu_sensor/rpy.roll               ← 新增
  imu_sensor/rpy.pitch              ← 新增
  imu_sensor/rpy.yaw                ← 新增
  imu_sensor/angular_velocity.x     ← 新增
  imu_sensor/angular_velocity.y     ← 新增
  imu_sensor/angular_velocity.z     ← 新增
  imu_sensor/linear_acceleration.x  ← 新增
  imu_sensor/linear_acceleration.y  ← 新增
  imu_sensor/linear_acceleration.z  ← 新增
```

### 验证3: 对比两个话题的数据

```bash
# 终端1：查看欧拉角（角度）
ros2 topic echo /imu/rpy

# 终端2：查看完整IMU数据（弧度）
ros2 topic echo /imu_sensor_broadcaster/imu
```

**数据对应关系**:
- `/imu/rpy.vector.x` (度) ≈ `/imu_sensor_broadcaster/imu.orientation` 转换后的 roll
- `/imu/rpy.vector.y` (度) ≈ `/imu_sensor_broadcaster/imu.orientation` 转换后的 pitch
- `/imu/rpy.vector.z` (度) ≈ `/imu_sensor_broadcaster/imu.orientation` 转换后的 yaw

**注意**: `/imu/rpy` 是角度（度），`/imu_sensor_broadcaster/imu` 中的欧拉角需要从四元数转换。

---

## Foxglove 可视化验证

### 1. 连接 Foxglove

打开 Foxglove Studio，连接到 ROS2：
- 地址: `ws://localhost:9090` (如果使用 rosbridge)
- 或直接使用 Foxglove Bridge

### 2. 添加 3D 面板

1. 点击 "+" 添加面板
2. 选择 "3D"
3. 在左侧设置中，添加话题：
   - 话题: `/imu_sensor_broadcaster/imu`
   - 类型: `sensor_msgs/Imu`
4. 应该能看到IMU的3D姿态可视化

### 3. 添加 Plot 面板（欧拉角）

1. 点击 "+" 添加面板
2. 选择 "Plot"
3. 添加以下路径：
   - `/imu/rpy.vector.x` (Roll, 度)
   - `/imu/rpy.vector.y` (Pitch, 度)
   - `/imu/rpy.vector.z` (Yaw, 度)

### 4. 添加 Plot 面板（角速度）

1. 点击 "+" 添加面板
2. 选择 "Plot"
3. 添加以下路径：
   - `/imu_sensor_broadcaster/imu.angular_velocity.x`
   - `/imu_sensor_broadcaster/imu.angular_velocity.y`
   - `/imu_sensor_broadcaster/imu.angular_velocity.z`

### 5. 添加 Plot 面板（加速度）

1. 点击 "+" 添加面板
2. 选择 "Plot"
3. 添加以下路径：
   - `/imu_sensor_broadcaster/imu.linear_acceleration.x`
   - `/imu_sensor_broadcaster/imu.linear_acceleration.y`
   - `/imu_sensor_broadcaster/imu.linear_acceleration.z`

**预期**: 静止时，Z轴加速度应该约为 9.81 m/s²（重力加速度）

---

## 故障排查

### 问题1: `/imu_sensor_broadcaster/imu` 仍然没有数据

**检查步骤**:

1. **检查 `/imu/rpy` 是否有数据**:
   ```bash
   ros2 topic echo /imu/rpy --once
   ```
   - 如果有数据：说明IMU硬件和解析正常，问题在broadcaster
   - 如果没有数据：说明IMU硬件或解析有问题

2. **检查控制器状态**:
   ```bash
   ros2 control list_controllers
   ```
   - 确保 `imu_sensor_broadcaster` 是 `active` 状态

3. **检查状态接口**:
   ```bash
   ros2 control list_hardware_interfaces | grep imu_sensor
   ```
   - 应该看到13个 `imu_sensor/*` 状态接口

4. **重启broadcaster**:
   ```bash
   ros2 control set_controller_state imu_sensor_broadcaster inactive
   ros2 control set_controller_state imu_sensor_broadcaster active
   ```

### 问题2: 编译错误 - `'sensors' is not a member`

**错误信息**:
```
error: 'struct hardware_interface::HardwareInfo' has no member named 'sensors'
```

**原因**: ROS2版本太旧（需要 Humble 或更高版本）

**解决方法**:
```bash
# 检查ROS2版本
echo $ROS_DISTRO

# 如果不是 humble 或更高版本，需要升级ROS2
```

### 问题3: 四元数全为0

**现象**:
```yaml
orientation:
  x: 0.0
  y: 0.0
  z: 0.0
  w: 0.0  # ← 应该至少 w=1.0
```

**原因**: IMU设备没有发送四元数数据（rid != 0x04）

**检查方法**:
```bash
# 查看原始IMU数据日志
ros2 topic echo /imu/rpy
# 如果有数据，说明欧拉角正常，但四元数可能没有接收到
```

**临时解决方法**: 从欧拉角计算四元数（需要修改代码）

### 问题4: 数据频率太低

**现象**:
```bash
ros2 topic hz /imu_sensor_broadcaster/imu
# 输出: 10 Hz (期望 100 Hz)
```

**原因**: 控制器更新频率配置太低

**解决方法**: 检查 `controller_manager` 的更新频率配置

---

## 成功标志

✅ **修复成功的标志**:

1. `ros2 topic list` 显示 `/imu_sensor_broadcaster/imu` 话题
2. `ros2 topic echo /imu_sensor_broadcaster/imu` 有数据输出
3. `ros2 control list_hardware_interfaces` 显示13个 `imu_sensor/*` 状态接口
4. Foxglove 3D面板可以显示IMU姿态
5. 两个话题的数据频率相同（通常 100 Hz）

---

## 性能验证

### 检查数据频率

```bash
# 检查两个话题的频率
ros2 topic hz /imu/rpy
ros2 topic hz /imu_sensor_broadcaster/imu
```

**预期**: 两个话题的频率应该相同（通常 100 Hz）

### 检查延迟

```bash
# 检查话题延迟
ros2 topic delay /imu_sensor_broadcaster/imu
```

**预期**: 延迟应该很小（< 10ms）

### 检查CPU使用率

```bash
# 查看 ros2_control_node 的CPU使用率
top -p $(pgrep -f ros2_control_node)
```

**预期**: CPU使用率应该正常（< 50%）

---

## 总结

### 修复前
- ❌ `/imu_sensor_broadcaster/imu` 没有数据
- ❌ 状态接口没有导出
- ✅ `/imu/rpy` 有数据（直接发布）

### 修复后
- ✅ `/imu_sensor_broadcaster/imu` 有数据
- ✅ 13个IMU状态接口已导出
- ✅ `/imu/rpy` 有数据（直接发布）
- ✅ Foxglove 可以可视化完整IMU数据

### 关键改动
在 `export_state_interfaces()` 中添加了13个IMU传感器状态接口的导出代码。

### 相关文档
- `IMU话题发布者说明.md` - 详细的话题发布者说明
- `IMU状态接口导出修复说明.md` - 修复的详细说明
- `Foxglove查看IMU数据指南.md` - Foxglove可视化指南
