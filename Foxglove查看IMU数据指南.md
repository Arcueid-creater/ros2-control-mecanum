# Foxglove 查看 IMU 数据指南

## 方案三提供的两个话题

### 话题1：`/imu/rpy` （推荐用于查看欧拉角）

**消息类型**：`geometry_msgs/msg/Vector3Stamped`

**数据内容**：
- `vector.x` = Roll（横滚角，单位：度）
- `vector.y` = Pitch（俯仰角，单位：度）
- `vector.z` = Yaw（偏航角，单位：度）

**优点**：
- ✅ 直接是欧拉角，易读
- ✅ 单位是角度（不是弧度），方便人类理解
- ✅ 数据结构简单，适合绘图

**查看命令**：
```bash
ros2 topic echo /imu/rpy
```

**输出示例**：
```yaml
header:
  stamp:
    sec: 1234567890
    nanosec: 123456789
  frame_id: imu_link
vector:
  x: 15.5    # Roll: 15.5度
  y: -8.2    # Pitch: -8.2度
  z: 90.3    # Yaw: 90.3度
```

---

### 话题2：`/imu_sensor_broadcaster/imu`

**消息类型**：`sensor_msgs/msg/Imu`

**数据内容**：
- `orientation` = 四元数（x, y, z, w）
- `angular_velocity` = 角速度（x, y, z，单位：rad/s）
- `linear_acceleration` = 线性加速度（x, y, z，单位：m/s²）

**优点**：
- ✅ 标准 ROS2 IMU 消息格式
- ✅ 包含完整的 IMU 数据
- ✅ 可以用 Foxglove 的 3D 面板显示姿态

**缺点**：
- ❌ 没有直接的欧拉角（只有四元数）
- ❌ 需要转换才能看到欧拉角

**查看命令**：
```bash
ros2 topic echo /imu_sensor_broadcaster/imu
```

---

## Foxglove 配置建议

### 方案A：查看欧拉角曲线（推荐）

**使用话题**：`/imu/rpy`

**步骤**：
1. 打开 Foxglove Studio
2. 添加 **Plot** 面板
3. 点击 "Add series"，添加以下数据源：
   - `/imu/rpy.vector.x` （Roll）
   - `/imu/rpy.vector.y` （Pitch）
   - `/imu/rpy.vector.z` （Yaw）
4. 可以看到三条曲线，分别代表三个欧拉角

**效果**：实时显示欧拉角变化曲线，单位是度

---

### 方案B：3D 姿态可视化

**使用话题**：`/imu_sensor_broadcaster/imu`

**步骤**：
1. 打开 Foxglove Studio
2. 添加 **3D** 面板
3. 在左侧话题列表中找到 `/imu_sensor_broadcaster/imu`
4. 勾选显示
5. 可以看到 IMU 的 3D 姿态可视化

**效果**：3D 模型显示 IMU 当前姿态（基于四元数）

---

### 方案C：同时查看（最全面）

**配置**：
1. **Plot 面板1**：订阅 `/imu/rpy`，显示欧拉角曲线
2. **Plot 面板2**：订阅 `/imu_sensor_broadcaster/imu.angular_velocity`，显示角速度
3. **Plot 面板3**：订阅 `/imu_sensor_broadcaster/imu.linear_acceleration`，显示加速度
4. **3D 面板**：订阅 `/imu_sensor_broadcaster/imu`，显示 3D 姿态

---

## 快速对比

| 特性 | `/imu/rpy` | `/imu_sensor_broadcaster/imu` |
|------|-----------|-------------------------------|
| 欧拉角 | ✅ 直接提供（度） | ❌ 需要从四元数转换 |
| 四元数 | ❌ 无 | ✅ 提供 |
| 角速度 | ❌ 无 | ✅ 提供 |
| 加速度 | ❌ 无 | ✅ 提供 |
| 易读性 | ✅ 高（角度） | ⚠️ 中（弧度） |
| 3D显示 | ❌ 不支持 | ✅ 支持 |
| 数据完整性 | ⚠️ 只有欧拉角 | ✅ 完整IMU数据 |

---

## 推荐配置

### 如果你只关心欧拉角：
```
使用 /imu/rpy
→ Plot 面板显示 Roll, Pitch, Yaw 曲线
```

### 如果你需要完整的 IMU 数据：
```
使用 /imu_sensor_broadcaster/imu
→ 3D 面板显示姿态
→ Plot 面板显示角速度和加速度
```

### 如果你需要欧拉角 + 完整数据：
```
同时订阅两个话题
→ /imu/rpy 用于欧拉角曲线
→ /imu_sensor_broadcaster/imu 用于其他数据
```

---

## 验证话题是否存在

启动系统后，运行：
```bash
ros2 topic list
```

应该看到：
```
/imu/rpy
/imu_sensor_broadcaster/imu
/joint_states
...
```

如果没有看到这些话题，检查：
1. 系统是否正常启动
2. IMU 串口是否连接
3. 配置文件中 `publish_rpy` 是否为 `true`

---

## 总结

**查看欧拉角最简单的方法**：
```
订阅 /imu/rpy 话题
在 Foxglove 的 Plot 面板中添加：
- /imu/rpy.vector.x (Roll)
- /imu/rpy.vector.y (Pitch)  
- /imu/rpy.vector.z (Yaw)
```

这样你就能实时看到欧拉角的变化曲线了！🎉
