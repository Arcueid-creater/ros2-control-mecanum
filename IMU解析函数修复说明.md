# IMU解析函数修复说明

## 问题描述

IMU解析函数存在以下问题：

1. **结构体定义错误**：`ImuPacket`结构体中使用了`dummy`字段，但应该是`id`字段
2. **数据数组大小不足**：`data[3]`无法存储四元数（需要4个float）
3. **注释错误**：rid值的注释与实际协议不符

## 修复内容

### 1. 修复ImuPacket结构体定义

**修改前：**
```cpp
struct ImuPacket {
  uint8_t header[2]; // 0x55, 0xAA
  uint8_t dummy;
  uint8_t rid;       // 0x01: RPY, 0x02: Gyro, 0x03: Accel
  float data[3];     // Generic data
  uint16_t crc;
  uint8_t tail;      // 0x0A
} __attribute__((packed));
```

**修改后：**
```cpp
struct ImuPacket {
  uint8_t header[2]; // 0x55, 0xAA
  uint8_t id;        // 固定为 0xID
  uint8_t rid;       // 0x01: Accel, 0x02: Gyro, 0x03: RPY, 0x04: Quaternion
  float data[4];     // 数据（最多4个float，根据rid使用3或4个）
  uint16_t crc;
  uint8_t tail;      // 0x0A
} __attribute__((packed));
```

### 2. 修复parseImuData函数逻辑

调整了rid值的处理顺序，使其符合协议规范：

- **0x01**: 加速度数据（Accel）
- **0x02**: 角速度数据（Gyro）
- **0x03**: 姿态角数据（RPY）
- **0x04**: 四元数数据（Quaternion）

### 3. 添加imu_pub_声明

在头文件中添加了IMU发布器的声明：
```cpp
rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
```

## 协议格式对照

根据提供的协议图：

| 数据类型 | 帧格式 | rid值 |
|---------|--------|-------|
| 加速度 | 55 AA ID 01 [X Y Z] CRC16 0A | 0x01 |
| 角速度 | 55 AA ID 02 [X Y Z] CRC16 0A | 0x02 |
| 姿态角 | 55 AA ID 03 [Roll Pitch Yaw] CRC16 0A | 0x03 |
| 四元数 | 55 AA ID 04 [W X Y Z] CRC16 0A | 0x04 |

## 验证

代码已通过编译检查，无语法错误。建议进行以下测试：

1. 使用实际IMU设备测试数据接收
2. 在Foxglove中查看IMU话题数据
3. 验证四元数数据是否正确解析
