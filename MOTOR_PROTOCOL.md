# 电机串口通信协议说明（简化版）

## 协议概述

本协议用于电机控制和反馈数据传输，采用浮点数格式和CRC32校验。

## 协议特点

- 帧头：`0xFF 0xAA`
- 校验：CRC32
- 数据格式：灵活，由用户自定义
- 支持大数据包：一次可传输几百个字节

---

## 1. 电机控制协议（上位机 → 下位机）

### 数据包结构

| 字段 | 类型 | 字节数 | 说明 |
|------|------|--------|------|
| header | uint8_t[2] | 2 | 帧头：0xFF 0xAA |
| data_length | uint16_t | 2 | 数据段长度（字节数） |
| motor_id | uint8_t | 1 | 电机ID |
| position | float | 4 | 位置指令（弧度或角度） |
| velocity | float | 4 | 速度指令（rad/s 或 rpm） |
| torque | float | 4 | 力矩指令（N·m） |
| crc32 | uint32_t | 4 | CRC32校验值 |

**总长度：21字节**

### 示例代码（发送）

```cpp
MotorControlPacket packet;
packet.header[0] = 0xFF;
packet.header[1] = 0xAA;
packet.data_length = sizeof(uint8_t) + 3 * sizeof(float); // 13字节
packet.motor_id = 1;
packet.position = 1.57f;  // 90度
packet.velocity = 0.5f;   // 0.5 rad/s
packet.torque = 0.1f;     // 0.1 N·m

// 计算CRC32（不包括CRC32字段本身）
size_t crc_data_size = sizeof(MotorControlPacket) - sizeof(uint32_t);
packet.crc32 = calculateCRC32((uint8_t*)&packet, crc_data_size);

// 发送
serial_device->write((uint8_t*)&packet, sizeof(packet));
```

---

## 2. 电机反馈协议（下位机 → 上位机）

### 数据包结构（灵活格式）

| 字段 | 类型 | 字节数 | 说明 |
|------|------|--------|------|
| header | uint8_t[2] | 2 | 帧头：0xFF 0xAA |
| data_length | uint16_t | 2 | 数据段长度（字节数） |
| data | uint8_t[] | data_length | 用户自定义数据（可以是任意格式） |
| crc32 | uint32_t | 4 | CRC32校验值 |

**总长度：8 + data_length 字节**

### 数据处理流程

上位机接收到数据后，会自动：
1. 查找帧头 `0xFF 0xAA`
2. 读取 `data_length` 字段
3. 验证 CRC32 校验
4. 将有效数据（不含帧头、长度字段和CRC32）存储到 `motor_data_buffer_`

**你只需要在 `motor_data_buffer_` 中读取数据并按照自己的协议解析即可！**

### 示例：多电机反馈（自定义格式）

假设你想传输3个电机的数据，每个电机包含：
- motor_id (1字节)
- position (4字节 float)
- velocity (4字节 float)
- current (4字节 float)
- temperature (4字节 float)

每个电机数据：17字节  
3个电机总数据：51字节

```cpp
// 下位机构建数据包
uint8_t buffer[100];
size_t offset = 0;

// 帧头
buffer[offset++] = 0xFF;
buffer[offset++] = 0xAA;

// 数据长度
uint16_t data_length = 51; // 3个电机 × 17字节
memcpy(buffer + offset, &data_length, 2);
offset += 2;

// 电机1数据
buffer[offset++] = 1; // motor_id
float pos1 = 1.57f, vel1 = 0.5f, cur1 = 2.3f, temp1 = 45.5f;
memcpy(buffer + offset, &pos1, 4); offset += 4;
memcpy(buffer + offset, &vel1, 4); offset += 4;
memcpy(buffer + offset, &cur1, 4); offset += 4;
memcpy(buffer + offset, &temp1, 4); offset += 4;

// 电机2数据
buffer[offset++] = 2;
float pos2 = 0.78f, vel2 = 0.3f, cur2 = 1.8f, temp2 = 42.0f;
memcpy(buffer + offset, &pos2, 4); offset += 4;
memcpy(buffer + offset, &vel2, 4); offset += 4;
memcpy(buffer + offset, &cur2, 4); offset += 4;
memcpy(buffer + offset, &temp2, 4); offset += 4;

// 电机3数据
buffer[offset++] = 3;
float pos3 = 2.35f, vel3 = 0.8f, cur3 = 3.1f, temp3 = 48.2f;
memcpy(buffer + offset, &pos3, 4); offset += 4;
memcpy(buffer + offset, &vel3, 4); offset += 4;
memcpy(buffer + offset, &cur3, 4); offset += 4;
memcpy(buffer + offset, &temp3, 4); offset += 4;

// 计算CRC32
uint32_t crc = calculateCRC32(buffer, offset);
memcpy(buffer + offset, &crc, 4);
offset += 4;

// 发送
serial_write(buffer, offset); // 总共59字节
```

### 上位机解析（你需要实现）

```cpp
// 在 parseMotorData() 函数中，motor_data_buffer_ 已经包含了纯净数据
// 你只需要按照自己的格式解析即可

void TideHardwareInterface::parseMotorData(const std::vector<uint8_t>& data)
{
  // ... 自动处理帧头、长度、CRC32 ...
  
  if (received_crc == calculated_crc)
  {
    // motor_data_buffer_ 现在包含纯净数据
    // 例如：51字节的3个电机数据
    
    // TODO: 在这里添加你的解析逻辑
    size_t offset = 0;
    while (offset + 17 <= motor_data_buffer_.size())
    {
      uint8_t motor_id = motor_data_buffer_[offset++];
      
      float position, velocity, current, temperature;
      memcpy(&position, motor_data_buffer_.data() + offset, 4); offset += 4;
      memcpy(&velocity, motor_data_buffer_.data() + offset, 4); offset += 4;
      memcpy(&current, motor_data_buffer_.data() + offset, 4); offset += 4;
      memcpy(&temperature, motor_data_buffer_.data() + offset, 4); offset += 4;
      
      // 更新对应电机的状态
      for (auto& motor : motors_)
      {
        if (motor->config_.tx_id == motor_id)
        {
          motor->angle_current = position;
          motor->measure.speed_aps = velocity;
          motor->measure.real_current = current;
          motor->measure.temperature = temperature;
          break;
        }
      }
    }
  }
}
```

---

## 3. CRC32 计算

### 算法

采用标准CRC32算法（多项式：0x04C11DB7）

```cpp
uint32_t calculateCRC32(const uint8_t* data, size_t len)
{
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < len; i++)
  {
    uint8_t index = (crc ^ data[i]) & 0xFF;
    crc = (crc >> 8) ^ CRC32_TABLE[index];
  }
  return ~crc;
}
```

### 注意事项

- CRC32计算范围：从帧头开始到数据结束，不包括CRC32字段本身
- 计算长度 = 4（帧头+长度字段）+ data_length

---

## 4. 代码集成说明

### 当前实现

上位机代码已经实现了：
- ✅ 自动查找帧头 `0xFF 0xAA`
- ✅ 自动读取数据长度
- ✅ 自动验证 CRC32
- ✅ 自动提取有效数据到 `motor_data_buffer_`

### 你需要做的

在 `tide_hw_interface.cpp` 的 `parseMotorData()` 函数中：
1. 找到 `// TODO: 在这里添加你自己的数据解析逻辑` 注释
2. 从 `motor_data_buffer_` 中读取数据
3. 按照你的协议格式解析
4. 更新电机状态

---

## 5. 协议优势

1. **灵活性**：数据格式完全自定义，不受限制
2. **大数据包支持**：data_length 最大65535字节，足够传输几百个电机数据
3. **强校验**：CRC32提供可靠的数据完整性检查
4. **简单集成**：框架已处理底层细节，你只需关注业务逻辑

---

## 6. 测试建议

### 单元测试

```cpp
// 测试CRC32计算
uint8_t test_data[] = {0xFF, 0xAA, 0x0D, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
uint32_t crc = calculateCRC32(test_data, sizeof(test_data));
printf("CRC32: 0x%08X\n", crc);
```

### 数据包测试

1. 发送一个简单的数据包（例如10字节数据）
2. 验证 `motor_data_buffer_` 是否正确接收到10字节
3. 逐步增加数据量，测试大数据包（200-300字节）

---

## 7. 故障排查

| 问题 | 可能原因 | 解决方案 |
|------|----------|----------|
| CRC校验失败 | 数据传输错误、字节序不一致 | 检查串口配置、确认大小端 |
| 数据丢失 | 缓冲区溢出 | 增大接收缓冲区 |
| 解析错误 | data_length不正确 | 检查发送端的长度计算 |
| 延迟高 | 发送频率过高 | 降低发送频率 |

---

## 8. 协议版本

- 当前版本：v2.0（简化版）
- 更新日期：2025-02-08
- 特点：灵活的数据格式，用户自定义解析
