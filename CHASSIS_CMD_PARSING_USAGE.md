# 底盘速度命令解析使用说明

## 概述

本文档说明如何在下位机代码中解析从ROS2上位机发送的底盘速度命令。

## 数据格式

### 上位机发送格式（ROS2）

```cpp
PacketBuilder builder;
builder.addByte(0x20)  // 消息ID：底盘速度命令
    .addFloat(static_cast<float>(chassis_cmd_linear_x_))   // 前后速度 (m/s)
    .addFloat(static_cast<float>(chassis_cmd_linear_y_))   // 左右速度 (m/s)
    .addFloat(static_cast<float>(chassis_cmd_angular_z_)); // 旋转角速度 (rad/s)

sendCustomData(builder.getData());
```

### 数据包结构

```
[0xFF][0xAA][长度(2字节)][消息ID(1字节)][linear_x(4字节)][linear_y(4字节)][angular_z(4字节)][CRC32(4字节)]
```

- **帧头**: `0xFF 0xAA`
- **数据长度**: `13` (1字节消息ID + 3个float = 1 + 12 = 13字节)
- **消息ID**: `0x20` (TRANS_MSG_ID_CHASSIS_CMD)
- **linear_x**: 前后速度，float类型
- **linear_y**: 左右速度，float类型
- **angular_z**: 旋转角速度，float类型
- **CRC32**: 校验和

## 下位机解析

### 1. 数据结构定义

在 `trans_task.h` 中已定义：

```c
typedef struct {
    float linear_x;   // 前后速度 (m/s)
    float linear_y;   // 左右速度 (m/s)
    float angular_z;  // 旋转角速度 (rad/s)
    uint8_t updated;  // 数据更新标志
} chassis_cmd_received_t;
```

### 2. 获取底盘命令

在你的底盘控制代码中使用：

```c
#include "trans_task.h"

void chassis_control_task(void)
{
    // 获取接收到的底盘命令
    chassis_cmd_received_t* cmd = get_chassis_cmd_received();
    
    // 检查是否有新数据
    if (cmd->updated)
    {
        // 使用接收到的速度命令
        float vx = cmd->linear_x;   // 前后速度
        float vy = cmd->linear_y;   // 左右速度
        float wz = cmd->angular_z;  // 旋转角速度
        
        // 清除更新标志
        cmd->updated = 0;
        
        // TODO: 将速度命令转换为电机控制
        // 例如：麦克纳姆轮逆运动学解算
        // motor_fl = vx - vy - wz * wheel_base;
        // motor_fr = vx + vy + wz * wheel_base;
        // motor_bl = vx + vy - wz * wheel_base;
        // motor_br = vx - vy + wz * wheel_base;
    }
}
```

### 3. 完整示例：麦克纳姆轮控制

```c
#include "trans_task.h"
#include "motor_control.h"

#define WHEEL_BASE 0.3f  // 轮距的一半 (m)
#define WHEEL_RADIUS 0.05f  // 轮子半径 (m)

void chassis_control_task(void)
{
    chassis_cmd_received_t* cmd = get_chassis_cmd_received();
    
    if (cmd->updated)
    {
        float vx = cmd->linear_x;
        float vy = cmd->linear_y;
        float wz = cmd->angular_z;
        
        // 麦克纳姆轮逆运动学
        float v_fl = vx - vy - wz * WHEEL_BASE;
        float v_fr = vx + vy + wz * WHEEL_BASE;
        float v_bl = vx + vy - wz * WHEEL_BASE;
        float v_br = vx - vy + wz * WHEEL_BASE;
        
        // 转换为电机转速 (rpm)
        float rpm_fl = (v_fl / WHEEL_RADIUS) * 60.0f / (2.0f * 3.14159f);
        float rpm_fr = (v_fr / WHEEL_RADIUS) * 60.0f / (2.0f * 3.14159f);
        float rpm_bl = (v_bl / WHEEL_RADIUS) * 60.0f / (2.0f * 3.14159f);
        float rpm_br = (v_br / WHEEL_RADIUS) * 60.0f / (2.0f * 3.14159f);
        
        // 发送电机控制命令
        set_motor_speed(MOTOR_FL, rpm_fl);
        set_motor_speed(MOTOR_FR, rpm_fr);
        set_motor_speed(MOTOR_BL, rpm_bl);
        set_motor_speed(MOTOR_BR, rpm_br);
        
        cmd->updated = 0;
    }
}
```

## 调试建议

### 1. 打印接收到的数据

```c
if (cmd->updated)
{
    printf("Received chassis cmd: vx=%.3f, vy=%.3f, wz=%.3f\r\n", 
           cmd->linear_x, cmd->linear_y, cmd->angular_z);
    cmd->updated = 0;
}
```

### 2. 检查数据范围

```c
if (cmd->updated)
{
    // 限制速度范围
    if (cmd->linear_x > 2.0f) cmd->linear_x = 2.0f;
    if (cmd->linear_x < -2.0f) cmd->linear_x = -2.0f;
    
    if (cmd->linear_y > 2.0f) cmd->linear_y = 2.0f;
    if (cmd->linear_y < -2.0f) cmd->linear_y = -2.0f;
    
    if (cmd->angular_z > 3.14f) cmd->angular_z = 3.14f;
    if (cmd->angular_z < -3.14f) cmd->angular_z = -3.14f;
    
    cmd->updated = 0;
}
```

## 注意事项

1. **数据更新标志**: 使用 `updated` 标志避免重复处理相同的命令
2. **速度限制**: 根据实际机器人性能限制速度范围
3. **坐标系**: 确保上下位机使用相同的坐标系定义
4. **单位**: 确认速度单位（m/s 或 rad/s）
5. **CRC校验**: 协议已自动处理CRC32校验，无效数据会被丢弃

## 消息ID定义

在 `trans_task.c` 中定义：

```c
typedef enum
{
    TRANS_MSG_ID_RC_DBUS = 0x10,      // 遥控器数据
    TRANS_MSG_ID_CHASSIS_CMD = 0x20,  // 底盘速度命令
} trans_msg_id_e;
```

如需添加更多消息类型，继续添加枚举值即可。
