# Chassis Controller 集成指南

本文档说明如何将 `tide_chassis_controller` 集成到现有的 ROS2 Control 框架中。

## 步骤 1: 编译控制器

```bash
cd <your_workspace>
colcon build --packages-select tide_chassis_controller tide_msgs
source install/setup.bash
```

## 步骤 2: 更新机器人配置文件

在你的机器人配置文件中（例如 `infantry_real.yaml`），添加 chassis_controller 的配置：

```yaml
controller_manager:
  ros__parameters:
    update_rate: 1000  # Hz

    joint_state_broadcaster:
      type: "joint_state_broadcaster/JointStateBroadcaster"

    gimbal_controller:
      type: "tide_gimbal_controller/TideGimbalController"

    # 添加 chassis_controller
    chassis_controller:
      type: "tide_chassis_controller/TideChassisController"

# 添加 chassis_controller 参数配置
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

## 步骤 3: 更新 Launch 文件

在 `src/tide_control/tide_ctrl_bringup/launch/controller.py` 中添加 chassis_controller：

```python
# 在文件顶部添加 chassis_controller 节点定义
chassis_controller = Node(
    package="controller_manager",
    executable="spawner",
    arguments=[
        "chassis_controller",
        "--controller-manager",
        "/controller_manager",
    ],
)

# 在 choose_controllers 函数中，将 chassis_controller 添加到控制器组
def choose_controllers(robot_type):
    sentry_controllers = GroupAction(
        [
            joint_state_broadcaster,
            left_gimbal_controller,
            right_gimbal_controller,
            left_shooter_controller,
            right_shooter_controller,
            chassis_controller,  # 添加这一行
        ]
    )

    other_controllers = GroupAction(
        [
            joint_state_broadcaster,
            gimbal_controller,
            chassis_controller,  # 添加这一行
        ]
    )
    # ... 其余代码保持不变
```

## 步骤 4: 更新机器人描述文件（URDF/XACRO）

确保你的机器人描述文件中定义了底盘轮子的关节。例如：

```xml
<!-- 前左轮 -->
<joint name="front_left_wheel_joint" type="continuous">
  <parent link="base_link"/>
  <child link="front_left_wheel_link"/>
  <origin xyz="0.2 0.2 0" rpy="0 0 0"/>
  <axis xyz="0 1 0"/>
</joint>

<!-- 前右轮 -->
<joint name="front_right_wheel_joint" type="continuous">
  <parent link="base_link"/>
  <child link="front_right_wheel_link"/>
  <origin xyz="0.2 -0.2 0" rpy="0 0 0"/>
  <axis xyz="0 1 0"/>
</joint>

<!-- 后左轮 -->
<joint name="rear_left_wheel_joint" type="continuous">
  <parent link="base_link"/>
  <child link="rear_left_wheel_link"/>
  <origin xyz="-0.2 0.2 0" rpy="0 0 0"/>
  <axis xyz="0 1 0"/>
</joint>

<!-- 后右轮 -->
<joint name="rear_right_wheel_joint" type="continuous">
  <parent link="base_link"/>
  <child link="rear_right_wheel_link"/>
  <origin xyz="-0.2 -0.2 0" rpy="0 0 0"/>
  <axis xyz="0 1 0"/>
</joint>
```

并在 `<ros2_control>` 标签中添加这些关节的接口：

```xml
<ros2_control name="chassis_system" type="system">
  <hardware>
    <plugin>tide_hw_interface/TideHWInterface</plugin>
  </hardware>
  
  <joint name="front_left_wheel_joint">
    <command_interface name="velocity"/>
    <state_interface name="velocity"/>
  </joint>
  
  <joint name="front_right_wheel_joint">
    <command_interface name="velocity"/>
    <state_interface name="velocity"/>
  </joint>
  
  <joint name="rear_left_wheel_joint">
    <command_interface name="velocity"/>
    <state_interface name="velocity"/>
  </joint>
  
  <joint name="rear_right_wheel_joint">
    <command_interface name="velocity"/>
    <state_interface name="velocity"/>
  </joint>
</ros2_control>
```

## 步骤 5: 测试控制器

1. 启动你的机器人：
```bash
ros2 launch tide_ctrl_bringup tide_ctrl_bringup.launch.py robot_type:=infantry
```

2. 检查控制器状态：
```bash
ros2 control list_controllers
```

你应该看到 `chassis_controller` 在列表中并且状态为 `active`。

3. 发送测试命令：
```bash
ros2 topic pub /chassis_controller/chassis_cmd tide_msgs/msg/ChassisCmd "{linear_x: 0.5, linear_y: 0.0, angular_z: 0.0, mode: 0}" --once
```

4. 查看底盘状态：
```bash
ros2 topic echo /chassis_controller/chassis_state
```

## 故障排查

### 控制器无法加载

- 检查 `tide_chassis_controller` 是否正确编译
- 确认 plugin 描述文件 `tide_chassis_controller.xml` 存在
- 检查配置文件中的控制器类型名称是否正确

### 找不到关节

- 确认 URDF/XACRO 文件中定义了相应的关节
- 检查配置文件中的关节名称是否与 URDF 中的一致
- 确认 `ros2_control` 标签中包含了这些关节的接口定义

### 消息类型错误

- 确保 `tide_msgs` 包已正确编译
- 重新 source workspace: `source install/setup.bash`

## 下一步

- 实现麦克纳姆轮运动学算法
- 添加速度平滑和限制
- 集成里程计发布
- 添加 PID 控制器以提高控制精度
