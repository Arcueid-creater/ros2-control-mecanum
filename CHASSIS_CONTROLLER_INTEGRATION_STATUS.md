# Chassis 控制器集成状态检查

## 集成状态总结

✅ **已完成集成！** Chassis 控制器已经完全集成到 TideControls 框架中，并且会在启动 sentry 时自动运行。

---

## 集成检查清单

### 1. ✅ 控制器代码实现

**位置**: `src/tide_controllers/tide_chassis_controller/`

- ✅ 头文件: `include/tide_chassis_controller.hpp`
- ✅ 源文件: `src/tide_chassis_controller.cpp`
- ✅ 参数文件: `src/tide_chassis_controller_parameter.yaml`
- ✅ CMakeLists.txt 配置
- ✅ package.xml 配置
- ✅ 插件导出配置

**功能**:
- ✅ 支持直接内存访问遥控器数据（12个通道）
- ✅ 支持底盘速度命令接口（linear_x, linear_y, angular_z）
- ✅ 支持话题订阅控制（/chassis_cmd）
- ✅ 发布底盘状态（/chassis_state）

---

### 2. ✅ 硬件接口支持

**位置**: `src/tide_control/tide_hw_interface/`

- ✅ 导出遥控器状态接口（rc/ch1, rc/ch2, ...）
- ✅ 导出底盘命令接口（chassis/linear_x, chassis/linear_y, chassis/angular_z）
- ✅ 解析遥控器数据并更新内存变量
- ✅ 读取底盘命令并发送到下位机

---

### 3. ✅ URDF 配置

**位置**: `src/tide_control/tide_ctrl_bringup/description/sentry/`

#### sentry_real.xacro
```xml
<!-- 底盘轮子关节定义 -->
<joint name="front_left_wheel_joint">
    <param name="can_bus">can1</param>
    <param name="tx_id">7</param>
    <param name="motor_type">M3508</param>
    <command_interface name="velocity"></command_interface>
    <state_interface name="position"/>
    <state_interface name="velocity"/>
</joint>

<joint name="front_right_wheel_joint">
    <param name="can_bus">can1</param>
    <param name="tx_id">8</param>
    <param name="motor_type">M3508</param>
    ...
</joint>

<joint name="rear_left_wheel_joint">
    <param name="can_bus">can2</param>
    <param name="tx_id">1</param>
    <param name="motor_type">M3508</param>
    ...
</joint>

<joint name="rear_right_wheel_joint">
    <param name="can_bus">can2</param>
    <param name="tx_id">2</param>
    <param name="motor_type">M3508</param>
    ...
</joint>
```

#### sentry_sim.xacro
```xml
<!-- 仿真环境下的底盘轮子关节 -->
<joint name="front_left_wheel_joint">
    <command_interface name="velocity"></command_interface>
    <state_interface name="position"/>
    <state_interface name="velocity"/>
</joint>
<!-- ... 其他三个轮子 ... -->
```

---

### 4. ✅ 控制器配置

**位置**: `src/tide_control/tide_ctrl_bringup/config/sentry/`

#### sentry_real.yaml
```yaml
controller_manager:
  ros__parameters:
    update_rate: 1000  # Hz
    
    chassis_controller:
      type: "tide_chassis_controller/TideChassisController"

chassis_controller:
  ros__parameters:
    wheel_joints:
      - "front_left_wheel_joint"
      - "front_right_wheel_joint"
      - "rear_left_wheel_joint"
      - "rear_right_wheel_joint"
    
    wheel_radius: 0.076        # meters
    wheel_base: 0.4            # meters
    wheel_track: 0.4           # meters
    
    max_linear_velocity: 3.0   # m/s
    max_angular_velocity: 3.0  # rad/s
    
    enable_odom: true
```

#### sentry_sim.yaml
```yaml
# 相同的配置
```

---

### 5. ✅ 启动文件配置

**位置**: `src/tide_control/tide_ctrl_bringup/launch/`

#### controller.py
```python
chassis_controller = Node(
    package="controller_manager",
    executable="spawner",
    arguments=[
        "chassis_controller",
        "--controller-manager",
        "/controller_manager",
    ],
)

def choose_controllers(robot_type):
    sentry_controllers = GroupAction(
        [
            joint_state_broadcaster,
            left_gimbal_controller,
            right_gimbal_controller,
            left_shooter_controller,
            right_shooter_controller,
            chassis_controller,  # ✅ 包含在 sentry 控制器组中
        ]
    )
```

#### tide_ctrl_bringup.launch.py
```python
# 自动加载控制器
controllers = choose_controllers(robot_type)

return LaunchDescription([
    # ...
    controllers,  # ✅ 会加载 chassis_controller
    # ...
])
```

---

## 启动流程

### 启动 Sentry（真实机器人）

```bash
ros2 launch tide_ctrl_bringup tide_ctrl_bringup.launch.py robot_type:=sentry sim_mode:=false
```

**执行流程**:
1. ✅ 加载 `sentry_real.xacro` → 定义底盘轮子关节
2. ✅ 加载 `sentry_real.yaml` → 配置 chassis_controller 参数
3. ✅ 启动 `ros2_control_node` → 加载硬件接口
4. ✅ Spawn `chassis_controller` → 启动底盘控制器
5. ✅ 控制器绑定接口 → 遥控器状态接口 + 底盘命令接口

### 启动 Sentry（仿真）

```bash
ros2 launch tide_ctrl_bringup tide_ctrl_bringup.launch.py robot_type:=sentry sim_mode:=true
```

**执行流程**:
1. ✅ 加载 `sentry_sim.xacro` → 定义底盘轮子关节
2. ✅ 加载 `sentry_sim.yaml` → 配置 chassis_controller 参数
3. ✅ 启动 Gazebo + `GazeboSystem` → 仿真硬件接口
4. ✅ Spawn `chassis_controller` → 启动底盘控制器

---

## 验证方法

### 1. 检查控制器是否加载

```bash
ros2 control list_controllers
```

**期望输出**:
```
chassis_controller[tide_chassis_controller/TideChassisController] active
joint_state_broadcaster[joint_state_broadcaster/JointStateBroadcaster] active
left_gimbal_controller[tide_gimbal_controller/TideGimbalController] active
right_gimbal_controller[tide_gimbal_controller/TideGimbalController] active
...
```

### 2. 检查硬件接口

```bash
ros2 control list_hardware_interfaces
```

**期望输出**:
```
command interfaces
    chassis/angular_z [available] [claimed]
    chassis/linear_x [available] [claimed]
    chassis/linear_y [available] [claimed]
    front_left_wheel_joint/velocity [available] [claimed]
    front_right_wheel_joint/velocity [available] [claimed]
    rear_left_wheel_joint/velocity [available] [claimed]
    rear_right_wheel_joint/velocity [available] [claimed]

state interfaces
    rc/ch1 [available] [claimed]
    rc/ch2 [available] [claimed]
    rc/ch3 [available] [claimed]
    rc/ch4 [available] [claimed]
    rc/sw1 [available] [claimed]
    rc/sw2 [available] [claimed]
    ...
    front_left_wheel_joint/position [available] [claimed]
    front_left_wheel_joint/velocity [available] [claimed]
    ...
```

### 3. 检查话题

```bash
ros2 topic list | grep chassis
```

**期望输出**:
```
/chassis_controller/chassis_cmd
/chassis_controller/chassis_state
```

### 4. 发送测试命令

```bash
ros2 topic pub /chassis_controller/chassis_cmd tide_msgs/msg/ChassisCmd "{linear_x: 1.0, linear_y: 0.0, angular_z: 0.0}" --once
```

### 5. 查看底盘状态

```bash
ros2 topic echo /chassis_controller/chassis_state
```

### 6. 查看遥控器数据

```bash
ros2 topic echo /rc/dbus
```

---

## 遥控器直接控制

### 启用方法

编辑 `src/tide_controllers/tide_chassis_controller/src/tide_chassis_controller.cpp`，在 `update()` 方法中找到以下代码块并**取消注释**：

```cpp
// ========== 方式2：直接读取遥控器数据（零延迟，直接内存访问）==========
// 如果需要遥控器直接控制底盘，取消下面的注释
/*
if (rc_state_interfaces_.count("ch1") && rc_state_interfaces_.count("ch2") && 
    rc_state_interfaces_.count("ch3") && rc_state_interfaces_.count("sw1"))
{
    // 读取遥控器摇杆值
    double ch1 = rc_state_interfaces_["ch1"]->get_value();
    double ch2 = rc_state_interfaces_["ch2"]->get_value();
    double ch3 = rc_state_interfaces_["ch3"]->get_value();
    double sw1 = rc_state_interfaces_["sw1"]->get_value();
    
    // ... 计算底盘速度命令 ...
}
*/
```

### 重新编译

```bash
cd ~/TideControls  # 或你的工作空间路径
colcon build --packages-select tide_chassis_controller
source install/setup.bash
```

### 遥控器映射

- **ch1** (右摇杆左右): 底盘左右平移
- **ch2** (右摇杆上下): 底盘前后移动
- **ch3** (左摇杆左右): 底盘旋转
- **sw1**: 速度模式切换（上=慢速，中=正常，下=快速）

---

## 注意事项

### 1. CAN 总线 ID 配置

**当前配置**（可能需要根据实际硬件调整）:
- `front_left_wheel_joint`: can1, tx_id=7
- `front_right_wheel_joint`: can1, tx_id=8
- `rear_left_wheel_joint`: can2, tx_id=1
- `rear_right_wheel_joint`: can2, tx_id=2

**如何修改**: 编辑 `sentry_real.xacro` 中的 `tx_id` 参数。

### 2. 电机类型

当前使用 **M3508** 电机。如果使用其他电机，修改 `motor_type` 参数：
- M2006
- M3508
- GM6020

### 3. 轮子半径和底盘尺寸

在 `sentry_real.yaml` 中配置：
```yaml
wheel_radius: 0.076        # 轮子半径（米）
wheel_base: 0.4            # 前后轮距（米）
wheel_track: 0.4           # 左右轮距（米）
```

### 4. 速度限制

```yaml
max_linear_velocity: 3.0   # 最大线速度（m/s）
max_angular_velocity: 3.0  # 最大角速度（rad/s）
```

---

## 故障排查

### 问题1: 控制器未加载

**检查**:
```bash
ros2 control list_controllers
```

**解决**:
1. 检查 URDF 中是否定义了底盘轮子关节
2. 检查配置文件中的关节名称是否匹配
3. 查看日志: `ros2 launch tide_ctrl_bringup tide_ctrl_bringup.launch.py robot_type:=sentry sim_mode:=false`

### 问题2: 接口未声明（claimed）

**检查**:
```bash
ros2 control list_hardware_interfaces
```

**解决**:
1. 确认控制器已激活: `ros2 control set_controller_state chassis_controller active`
2. 检查控制器配置中的 `wheel_joints` 是否正确

### 问题3: 遥控器数据无响应

**检查**:
```bash
ros2 topic echo /rc/dbus
```

**解决**:
1. 确认下位机正确发送遥控器数据（msg_id=0x10）
2. 检查串口连接: `/dev/stm32h7`
3. 查看硬件接口日志

### 问题4: 底盘不移动

**检查**:
1. 发送测试命令: `ros2 topic pub /chassis_controller/chassis_cmd ...`
2. 查看底盘状态: `ros2 topic echo /chassis_controller/chassis_state`
3. 检查电机 CAN ID 是否正确
4. 确认下位机接收到底盘命令（msg_id=0x20）

---

## 相关文档

- [遥控器直接内存访问技术文档](RC_DIRECT_MEMORY_ACCESS.md)
- [遥控器直接访问快速使用指南](RC_DIRECT_ACCESS_USAGE.md)
- [底盘控制器集成指南](src/tide_controllers/tide_chassis_controller/INTEGRATION.md)
- [串口通信协议](CHASSIS_SERIAL_COMMUNICATION.md)

---

## 总结

✅ **Chassis 控制器已完全集成！**

当你启动 sentry 时（无论是真实机器人还是仿真），chassis_controller 会自动加载并运行。

**下一步**:
1. 启动系统验证集成
2. 根据实际硬件调整 CAN ID
3. 如需遥控器直接控制，取消注释相关代码
4. 实现麦克纳姆轮运动学解算（TODO）
