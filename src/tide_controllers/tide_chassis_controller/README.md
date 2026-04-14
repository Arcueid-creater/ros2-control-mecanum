# Tide Chassis Controller

底盘控制器，用于控制 Tide 机器人的麦克纳姆轮底盘。

## 功能特性

- 支持麦克纳姆轮运动学
- 实时速度控制
- 底盘状态发布
- 参数动态更新

## 使用方法

### 1. 编译

```bash
cd <your_workspace>
colcon build --packages-select tide_chassis_controller tide_msgs
```

### 2. 配置

在你的机器人配置文件中添加以下内容（参考 `config/chassis_controller_example.yaml`）：

```yaml
controller_manager:
  ros__parameters:
    chassis_controller:
      type: "tide_chassis_controller/TideChassisController"

chassis_controller:
  ros__parameters:
    wheel_joints:
      - "front_left_wheel_joint"
      - "front_right_wheel_joint"
      - "rear_left_wheel_joint"
      - "rear_right_wheel_joint"
    wheel_radius: 0.076
    wheel_base: 0.4
    wheel_track: 0.4
    max_linear_velocity: 3.0
    max_angular_velocity: 3.0
    enable_odom: true
```

### 3. 启动控制器

```bash
ros2 control load_controller chassis_controller
ros2 control set_controller_state chassis_controller active
```

### 4. 发送控制命令

```bash
ros2 topic pub /chassis_controller/chassis_cmd tide_msgs/msg/ChassisCmd "{linear_x: 1.0, linear_y: 0.0, angular_z: 0.0, mode: 0}"
```

## 话题

### 订阅的话题

- `~/chassis_cmd` (tide_msgs/msg/ChassisCmd): 底盘控制命令

### 发布的话题

- `~/chassis_state` (tide_msgs/msg/ChassisState): 底盘状态信息

## 参数

- `wheel_joints`: 轮子关节名称数组
- `wheel_radius`: 轮子半径（米）
- `wheel_base`: 前后轮距离（米）
- `wheel_track`: 左右轮距离（米）
- `max_linear_velocity`: 最大线速度（m/s）
- `max_angular_velocity`: 最大角速度（rad/s）
- `enable_odom`: 是否启用里程计发布

## TODO

- [ ] 实现麦克纳姆轮运动学正逆解
- [ ] 添加里程计发布功能
- [ ] 添加速度限制和平滑处理
- [ ] 添加 PID 控制器
- [ ] 添加单元测试

## 依赖

- controller_interface
- hardware_interface
- rclcpp
- realtime_tools
- tide_msgs
- geometry_msgs
