# Chassis Controller 集成更新日志

## 已完成的集成工作

### 1. 更新了 Sentry 配置文件

#### sentry_real.yaml
- 在 `controller_manager` 中添加了 `chassis_controller` 类型声明
- 添加了完整的 `chassis_controller` 参数配置
  - 轮子关节名称：front_left_wheel_joint, front_right_wheel_joint, rear_left_wheel_joint, rear_right_wheel_joint
  - 轮子半径：0.076m
  - 轮距参数：wheel_base=0.4m, wheel_track=0.4m
  - 速度限制：max_linear_velocity=3.0m/s, max_angular_velocity=3.0rad/s
  - 启用里程计：enable_odom=true

#### sentry_sim.yaml
- 同样添加了 `chassis_controller` 的类型声明和参数配置
- 参数与 real 版本保持一致，适用于仿真环境

### 2. 更新了 Launch 文件

#### controller.py
- 添加了 `chassis_controller` 节点定义
- 将 `chassis_controller` 添加到 `sentry_controllers` 组中
- 将 `chassis_controller` 添加到 `other_controllers` 组中（infantry、hero 等机器人也可使用）

### 3. 文件修改清单

```
修改的文件：
├── src/tide_control/tide_ctrl_bringup/config/sentry/sentry_real.yaml
├── src/tide_control/tide_ctrl_bringup/config/sentry/sentry_sim.yaml
└── src/tide_control/tide_ctrl_bringup/launch/controller.py

新增的文件：
├── src/tide_controllers/tide_chassis_controller/
│   ├── include/tide_chassis_controller.hpp
│   ├── src/tide_chassis_controller.cpp
│   ├── src/tide_chassis_controller_parameter.yaml
│   ├── CMakeLists.txt
│   ├── package.xml
│   ├── tide_chassis_controller.xml
│   ├── config/chassis_controller_example.yaml
│   ├── launch/chassis_controller_example.py
│   ├── README.md
│   ├── INTEGRATION.md
│   └── CHANGELOG.md
└── src/tide_control/tide_msgs/msg/
    ├── ChassisCmd.msg
    └── ChassisState.msg
```

## 使用方法

### 编译

```bash
cd <your_workspace>
colcon build --packages-select tide_chassis_controller tide_msgs tide_ctrl_bringup
source install/setup.bash
```

### 启动 Sentry 机器人（真实机器人）

```bash
ros2 launch tide_ctrl_bringup tide_ctrl_bringup.launch.py robot_type:=sentry
```

### 启动 Sentry 机器人（仿真）

```bash
ros2 launch tide_ctrl_bringup tide_ctrl_bringup.launch.py robot_type:=sentry sim:=true
```

### 检查控制器状态

```bash
ros2 control list_controllers
```

应该看到：
```
chassis_controller[tide_chassis_controller/TideChassisController] active
left_gimbal_controller[tide_gimbal_controller/TideGimbalController] active
right_gimbal_controller[tide_gimbal_controller/TideGimbalController] active
...
```

### 发送底盘控制命令

```bash
# 前进
ros2 topic pub /chassis_controller/chassis_cmd tide_msgs/msg/ChassisCmd "{linear_x: 1.0, linear_y: 0.0, angular_z: 0.0, mode: 0}" --once

# 左平移
ros2 topic pub /chassis_controller/chassis_cmd tide_msgs/msg/ChassisCmd "{linear_x: 0.0, linear_y: 1.0, angular_z: 0.0, mode: 0}" --once

# 旋转
ros2 topic pub /chassis_controller/chassis_cmd tide_msgs/msg/ChassisCmd "{linear_x: 0.0, linear_y: 0.0, angular_z: 1.0, mode: 0}" --once

# 停止
ros2 topic pub /chassis_controller/chassis_cmd tide_msgs/msg/ChassisCmd "{linear_x: 0.0, linear_y: 0.0, angular_z: 0.0, mode: 0}" --once
```

### 查看底盘状态

```bash
ros2 topic echo /chassis_controller/chassis_state
```

## 注意事项

1. **轮子关节名称**：请根据你的 URDF/XACRO 文件中实际的关节名称修改配置文件中的 `wheel_joints` 参数

2. **机器人尺寸**：请根据实际的 Sentry 机器人尺寸调整以下参数：
   - `wheel_radius`: 轮子半径
   - `wheel_base`: 前后轮之间的距离
   - `wheel_track`: 左右轮之间的距离

3. **速度限制**：根据实际需求调整 `max_linear_velocity` 和 `max_angular_velocity`

4. **运动学实现**：当前控制器框架已完成，但麦克纳姆轮运动学算法需要在 `tide_chassis_controller.cpp` 的 `update()` 函数中实现

## 下一步工作

- [ ] 在 URDF/XACRO 中添加底盘轮子的关节定义
- [ ] 在 hardware interface 中添加轮子的接口支持
- [ ] 实现麦克纳姆轮运动学正逆解算法
- [ ] 添加速度平滑和加速度限制
- [ ] 实现里程计计算和发布
- [ ] 添加 PID 控制器以提高轮速控制精度
- [ ] 进行实际测试和参数调优
