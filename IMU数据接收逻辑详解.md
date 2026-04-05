# IMU数据接收逻辑详解

## 代码位置
`tide_hw_interface.cpp` - `parseImuSerialData()` 函数

---

## IMU数据包格式

```
┌────┬────┬────┬────┬──────────────────┬────────┬────┐
│ 55 │ AA │ ID │rid │   data[16字节]   │CRC16(2)│ 0A │
└────┴────┴────┴────┴──────────────────┴────────┴────┘
  0    1    2    3         4-19          20-21   22

总长度：23字节 (sizeof(ImuPacket))
```

**字段说明：**
- `0x55 0xAA` - 帧头（2字节）
- `ID` - 设备ID（1字节）
- `rid` - 数据类型（1字节）：0x01=加速度, 0x02=角速度, 0x03=RPY, 0x04=四元数
- `data` - 数据（16字节 = 4个float）
- `CRC16` - 校验码（2字节）
- `0x0A` - 帧尾（1字节）

---

## 接收逻辑流程图

```
                    开始
                     ↓
        ┌────────────────────────┐
        │ 缓冲区是否有足够数据？  │
        │ (size >= 23字节)       │
        └────────┬───────────────┘
                 │ 是
                 ↓
        ┌────────────────────────┐
        │ 检查帧头                │
        │ [0]==0x55 && [1]==0xAA?│
        └────┬───────────┬────────┘
             │ 是        │ 否
             ↓           ↓
    ┌────────────┐  ┌──────────────┐
    │ 拷贝完整包  │  │ 丢弃第一个字节│
    │ 到packet   │  │ 继续查找帧头  │
    └────┬───────┘  └──────────────┘
         ↓
    ┌────────────┐
    │ 检查帧尾    │
    │ tail==0x0A?│
    └────┬───────┘
         │ 是
         ↓
    ┌────────────────────┐
    │ 计算CRC16（方式1）  │
    │ 包含帧头（16字节）  │
    └────┬───────────────┘
         │
         ↓
    ┌────────────┐
    │ CRC匹配？   │
    └────┬───┬───┘
         │是 │否
         ↓   ↓
    ┌────┐ ┌────────────────────┐
    │解析│ │ 计算CRC16（方式2）  │
    │数据│ │ 不含帧头（14字节）  │
    └────┘ └────┬───────────────┘
                │
                ↓
           ┌────────────┐
           │ CRC匹配？   │
           └────┬───────┘
                │ 是
                ↓
           ┌────────┐
           │ 解析数据│
           └────────┘
                ↓
           ┌────────────┐
           │ 删除已处理  │
           │ 的23字节    │
           └────────────┘
                ↓
              继续循环
```

---

## 详细代码解析

### 步骤1：检查缓冲区大小

```cpp
while (imu_rx_buffer_.size() >= sizeof(ImuPacket))
```

**作用：** 确保缓冲区至少有23字节（一个完整的IMU数据包）

**为什么这样做：**
- 避免访问越界
- 确保有完整的数据包可以处理

---

### 步骤2：查找帧头

```cpp
if (imu_rx_buffer_[0] == 0x55 && imu_rx_buffer_[1] == 0xAA)
{
    // 找到帧头，继续处理
}
else
{
    // 没找到帧头，丢弃第一个字节
    imu_rx_buffer_.erase(imu_rx_buffer_.begin());
}
```

**作用：** 帧同步 - 确保从正确的位置开始解析

**示例：**
```
错误的缓冲区：[0x12, 0x34, 0x55, 0xAA, ...]
                ↑
              丢弃这个字节

正确的缓冲区：[0x55, 0xAA, 0xID, 0xrid, ...]
                ↑
              从这里开始解析
```

---

### 步骤3：拷贝完整数据包

```cpp
ImuPacket packet;
std::memcpy(&packet, imu_rx_buffer_.data(), sizeof(ImuPacket));
```

**作用：** 将缓冲区的23字节拷贝到结构体中

**内存布局：**
```
imu_rx_buffer_:  [55 AA ID rid data... CRC16 0A ...]
                  ↓ memcpy 23字节
packet:          [55 AA ID rid data... CRC16 0A]
                  ↑                           ↑
                header[2]                   tail
```

---

### 步骤4：检查帧尾

```cpp
if (packet.tail == 0x0A)
{
    // 帧尾正确，继续验证CRC
}
```

**作用：** 二次验证 - 确保数据包完整

**为什么需要：**
- 防止误识别（虽然有帧头，但可能不是真正的数据包）
- 提高可靠性

---

### 步骤5：CRC16校验（方式1 - 包含帧头）

```cpp
uint16_t crc_calc = calculateCRC16(imu_rx_buffer_.data(), 16);
if (crc_calc == packet.crc)
{
    parseImuData(imu_rx_buffer_.data());
}
```

**计算范围：** 前16字节（包含帧头）

```
计算CRC的数据：
┌────┬────┬────┬────┬──────────────────┐
│ 55 │ AA │ ID │rid │   data[12字节]   │
└────┴────┴────┴────┴──────────────────┘
  0    1    2    3         4-15

共16字节
```

**为什么是16字节：**
- 帧头(2) + ID(1) + rid(1) + data(12) = 16字节
- 注意：data只用了前3个float（12字节），不是全部4个float

---

### 步骤6：CRC16校验（方式2 - 不含帧头）

```cpp
else
{
    // Try without header (skip 0x55 0xAA)
    uint16_t crc_calc_no_hdr = calculateCRC16(imu_rx_buffer_.data() + 2, 14);
    if (crc_calc_no_hdr == packet.crc)
    {
        parseImuData(imu_rx_buffer_.data());
    }
}
```

**计算范围：** 跳过帧头，从第3字节开始，共14字节

```
计算CRC的数据：
┌────┬────┬──────────────────┐
│ ID │rid │   data[12字节]   │
└────┴────┴──────────────────┘
  2    3         4-15

共14字节（跳过了帧头的2字节）
```

**为什么有两种方式：**
- 兼容性考虑 - 不同的IMU设备可能使用不同的CRC计算方式
- 有些设备CRC包含帧头，有些不包含
- 代码尝试两种方式，提高兼容性

---

### 步骤7：解析数据

```cpp
parseImuData(imu_rx_buffer_.data());
```

**作用：** 提取实际的IMU数据（加速度、角速度、RPY、四元数）

**详细流程：**
```cpp
void TideHardwareInterface::parseImuData(const uint8_t* frame)
{
  ImuPacket packet;
  std::memcpy(&packet, frame, sizeof(ImuPacket));

  std::lock_guard<std::mutex> lock(imu_mutex_);
  
  if (packet.rid == 0x01) // Accel (加速度)
  {
    imu_accel_[0] = packet.data[0]; // m/s^2
    imu_accel_[1] = packet.data[1];
    imu_accel_[2] = packet.data[2];
  }
  else if (packet.rid == 0x02) // Gyro (角速度)
  {
    imu_gyro_[0] = packet.data[0] * M_PI / 180.0; // 转换为 rad/s
    imu_gyro_[1] = packet.data[1] * M_PI / 180.0;
    imu_gyro_[2] = packet.data[2] * M_PI / 180.0;
  }
  else if (packet.rid == 0x03) // RPY (姿态角)
  {
    imu_rpy_[0] = packet.data[0] * M_PI / 180.0; // 转换为弧度
    imu_rpy_[1] = packet.data[1] * M_PI / 180.0;
    imu_rpy_[2] = packet.data[2] * M_PI / 180.0;
  }
  else if (packet.rid == 0x04) // Quaternion (四元数)
  {
    imu_orientation[0] = packet.data[0]; // W
    imu_orientation[1] = packet.data[1]; // X
    imu_orientation[2] = packet.data[2]; // Y
    imu_orientation[3] = packet.data[3]; // Z
  }
}
```

---

### 步骤8：删除已处理的数据

```cpp
imu_rx_buffer_.erase(imu_rx_buffer_.begin(), 
                     imu_rx_buffer_.begin() + sizeof(ImuPacket));
```

**作用：** 从缓冲区删除已处理的23字节

**示例：**
```
处理前：[55 AA ID rid data... CRC 0A | 55 AA ID rid ...]
                                      ↑
                                   删除前23字节

处理后：[55 AA ID rid ...]
         ↑
       准备处理下一个包
```

---

## 完整示例

### 示例1：正常接收

```
接收到的数据：
[55 AA 01 03 3F 80 00 00 40 00 00 00 40 40 00 00 00 00 00 00 AB CD 0A ...]
 ↑  ↑  ↑  ↑  ←------------- data[16] -----------→ ←-CRC-→ ↑
帧头 ID rid  roll=1.0  pitch=2.0  yaw=3.0  (未用)         帧尾

步骤：
1. 检查：size >= 23 ✓
2. 帧头：0x55 0xAA ✓
3. 拷贝：23字节到packet
4. 帧尾：0x0A ✓
5. CRC16：计算并验证 ✓
6. 解析：提取roll=1.0, pitch=2.0, yaw=3.0
7. 删除：前23字节
```

### 示例2：数据错位

```
接收到的数据：
[12 34 55 AA 01 03 ...]
 ↑
 垃圾数据

步骤：
1. 检查：size >= 23 ✓
2. 帧头：0x12 != 0x55 ✗
3. 删除：第一个字节 [12]
4. 继续：[34 55 AA 01 03 ...]
5. 帧头：0x34 != 0x55 ✗
6. 删除：第一个字节 [34]
7. 继续：[55 AA 01 03 ...] ✓
8. 正常处理
```

### 示例3：CRC错误

```
接收到的数据：
[55 AA 01 03 ... FF FF 0A]  // CRC错误
                 ↑
              错误的CRC

步骤：
1. 检查：size >= 23 ✓
2. 帧头：0x55 0xAA ✓
3. 拷贝：23字节到packet
4. 帧尾：0x0A ✓
5. CRC16（方式1）：不匹配 ✗
6. CRC16（方式2）：不匹配 ✗
7. 删除：前23字节（丢弃这个包）
8. 继续处理下一个包
```

---

## 关键设计特点

### 1. 滑动窗口机制

```
缓冲区：[... 55 AA ID rid data... CRC 0A 55 AA ...]
              ↑                           ↑
           处理这个包                  下一个包
```

- 每次处理一个包（23字节）
- 删除已处理的数据
- 继续处理剩余数据

### 2. 容错机制

```
┌─────────────────┐
│ 帧头检查 (0x55) │ ← 第一道防线
└────────┬────────┘
         ↓
┌─────────────────┐
│ 帧尾检查 (0x0A) │ ← 第二道防线
└────────┬────────┘
         ↓
┌─────────────────┐
│ CRC16校验       │ ← 第三道防线
└────────┬────────┘
         ↓
┌─────────────────┐
│ 数据有效        │
└─────────────────┘
```

### 3. 双CRC模式

- **方式1（包含帧头）：** 计算16字节
- **方式2（不含帧头）：** 计算14字节
- **目的：** 兼容不同的IMU设备

### 4. 自动同步

```
错误数据：[12 34 56 55 AA ...]
           ↓  ↓  ↓
        逐字节丢弃，直到找到帧头
```

---

## 性能分析

### 时间复杂度

- **最好情况：** O(1) - 第一个字节就是帧头
- **最坏情况：** O(n) - 需要逐字节查找帧头
- **平均情况：** O(1) - 正常通信时，数据对齐

### 内存使用

- **缓冲区：** 动态大小（`std::vector<uint8_t>`）
- **临时变量：** 23字节（`ImuPacket packet`）
- **总计：** 约 100-200 字节（取决于缓冲区积累）

---

## 常见问题

### Q1: 为什么CRC计算是16字节而不是23字节？

**A:** 因为CRC本身和帧尾不参与CRC计算：
```
参与CRC计算：[55 AA ID rid data(12)] = 16字节
不参与：     [CRC(2) 0A(1)] = 3字节
```

### Q2: 为什么有两种CRC计算方式？

**A:** 兼容性考虑：
- 有些IMU设备CRC包含帧头（0x55 0xAA）
- 有些IMU设备CRC不包含帧头
- 代码尝试两种方式，提高兼容性

### Q3: 如果CRC都不匹配会怎样？

**A:** 丢弃这个数据包：
```cpp
// 两种CRC都不匹配，直接删除这23字节
imu_rx_buffer_.erase(imu_rx_buffer_.begin(), 
                     imu_rx_buffer_.begin() + sizeof(ImuPacket));
```

### Q4: 为什么要检查帧尾？

**A:** 双重验证，提高可靠性：
- 帧头可能误识别（数据中恰好有0x55 0xAA）
- 帧尾提供额外的验证
- 帧头+帧尾+CRC = 三重保护

---

## 调试建议

### 1. 添加日志

```cpp
// 在parseImuSerialData()中添加
RCLCPP_DEBUG(rclcpp::get_logger("IMU"), 
            "Buffer size: %zu, Header: 0x%02X 0x%02X", 
            imu_rx_buffer_.size(), 
            imu_rx_buffer_[0], 
            imu_rx_buffer_[1]);

// CRC验证失败时
RCLCPP_WARN(rclcpp::get_logger("IMU"), 
           "CRC mismatch: calc1=0x%04X, calc2=0x%04X, received=0x%04X",
           crc_calc, crc_calc_no_hdr, packet.crc);
```

### 2. 打印原始数据

```cpp
// 打印接收到的原始数据包
std::stringstream ss;
ss << "IMU packet: ";
for (size_t i = 0; i < sizeof(ImuPacket); i++) {
    ss << std::hex << std::setfill('0') << std::setw(2) 
       << (int)imu_rx_buffer_[i] << " ";
}
RCLCPP_INFO(rclcpp::get_logger("IMU"), "%s", ss.str().c_str());
```

### 3. 统计信息

```cpp
static int total_packets = 0;
static int crc_errors = 0;
static int frame_errors = 0;

// 在相应位置增加计数
total_packets++;
crc_errors++;
frame_errors++;

// 定期打印
if (total_packets % 1000 == 0) {
    RCLCPP_INFO(rclcpp::get_logger("IMU"), 
               "IMU stats: total=%d, crc_err=%d, frame_err=%d",
               total_packets, crc_errors, frame_errors);
}
```

---

## 总结

### 接收流程

1. **等待数据** → 缓冲区积累至少23字节
2. **查找帧头** → 0x55 0xAA
3. **拷贝数据** → 23字节到结构体
4. **检查帧尾** → 0x0A
5. **验证CRC** → 两种方式（包含/不含帧头）
6. **解析数据** → 提取IMU数据
7. **删除数据** → 移除已处理的23字节
8. **循环处理** → 继续处理剩余数据

### 关键特点

- ✅ 自动帧同步
- ✅ 三重验证（帧头+帧尾+CRC）
- ✅ 双CRC模式（兼容性）
- ✅ 容错机制
- ✅ 线程安全（使用互斥锁）

### 可靠性

- **帧头误识别概率：** 1/65536 (0x55 0xAA)
- **CRC错误检测率：** 99.998%
- **总体可靠性：** 非常高

---

## 相关文档

- `tide_hw_interface.cpp` - 实现代码
- `IMU代码检查报告.md` - IMU功能说明
- `Foxglove查看IMU数据指南.md` - 数据可视化
