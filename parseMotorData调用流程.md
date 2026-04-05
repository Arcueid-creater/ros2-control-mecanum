# parseMotorData 调用流程说明

## 调用链

```
下位机发送数据
    ↓ 串口
SerialDevice::run() 线程
    ↓ 读取串口数据
::read(fd_, rx_buffer.data(), rx_buffer.size())
    ↓ 第 206 行
receive_callback_(data)
    ↓ 回调函数
parseMotorData(data)
    ↓ 第 466 行
解析数据并存储到 motor_data_buffer_
```

## 详细说明

### 1. 初始化阶段（on_init 函数）

**位置：** `tide_hw_interface.cpp` 第 335-338 行

```cpp
// 创建回调函数（Lambda 表达式）
auto motor_receive_callback = [this](const std::vector<uint8_t>& data) {
  this->parseMotorData(data);  // ← 调用 parseMotorData
};

// 创建串口设备，传入回调函数
serial_device_ = std::make_shared<SerialDevice>(
    serial_port_, 
    baud_rate_, 
    motor_receive_callback  // ← 传入回调
);
```

### 2. SerialDevice 构造函数

**位置：** `tide_hw_interface.cpp` 第 85-133 行

```cpp
SerialDevice::SerialDevice(
    const std::string& port_name, 
    uint32_t baud_rate, 
    ReceiveCallback callback)  // ← 接收回调函数
  : port_name_(port_name), 
    baud_rate_(baud_rate), 
    receive_callback_(std::move(callback))  // ← 保存回调
{
  // 打开串口...
  
  running_ = true;
  thread_ = std::thread(&SerialDevice::run, this);  // ← 启动接收线程
}
```

### 3. SerialDevice::run() 线程（持续运行）

**位置：** `tide_hw_interface.cpp` 第 174-210 行

```cpp
void SerialDevice::run()
{
  std::vector<uint8_t> rx_buffer(1024);
  struct pollfd fds[1];
  fds[0].fd = fd_;
  fds[0].events = POLLIN;

  while (running_ && rclcpp::ok())
  {
    // ... 发送部分 ...

    // ========== 接收数据 ==========
    int ret = poll(fds, 1, 10);  // 等待数据
    if (ret > 0 && (fds[0].revents & POLLIN))
    {
      // 从串口读取数据
      ssize_t n = ::read(fd_, rx_buffer.data(), rx_buffer.size());
      
      if (n > 0)
      {
        // 创建数据向量
        std::vector<uint8_t> data(rx_buffer.begin(), rx_buffer.begin() + n);
        
        // ========== 调用回调函数 ==========
        receive_callback_(data);  // ← 这里调用 parseMotorData！
      }
    }
  }
}
```

### 4. parseMotorData 函数

**位置：** `tide_hw_interface.cpp` 第 466-520 行

```cpp
void TideHardwareInterface::parseMotorData(const std::vector<uint8_t>& data)
{
  // 添加到接收缓冲区
  motor_rx_buffer_.insert(motor_rx_buffer_.end(), data.begin(), data.end());

  // 查找帧头 0xFF 0xAA
  while (motor_rx_buffer_.size() >= 8)
  {
    if (motor_rx_buffer_[0] == 0xFF && motor_rx_buffer_[1] == 0xAA)
    {
      // 解析数据长度
      uint16_t data_length;
      std::memcpy(&data_length, motor_rx_buffer_.data() + 2, sizeof(uint16_t));
      
      // 计算完整包大小
      size_t total_packet_size = 4 + data_length + 4;
      
      // 检查是否接收完整
      if (motor_rx_buffer_.size() < total_packet_size)
      {
        break;  // 等待更多数据
      }
      
      // 验证CRC32
      size_t crc_offset = total_packet_size - 4;
      uint32_t received_crc;
      std::memcpy(&received_crc, motor_rx_buffer_.data() + crc_offset, sizeof(uint32_t));
      
      uint32_t calculated_crc = calculateCRC32(motor_rx_buffer_.data(), crc_offset);
      
      if (received_crc == calculated_crc)
      {
        // CRC校验通过，提取有效数据
        motor_data_buffer_.clear();
        motor_data_buffer_.insert(motor_data_buffer_.end(),
                                  motor_rx_buffer_.begin() + 4,
                                  motor_rx_buffer_.begin() + crc_offset);
        
        // TODO: 在这里添加你的数据解析逻辑
        // motor_data_buffer_ 现在包含了纯净的数据
        
        // 移除已处理的数据包
        motor_rx_buffer_.erase(motor_rx_buffer_.begin(), 
                              motor_rx_buffer_.begin() + total_packet_size);
      }
      else
      {
        // CRC校验失败
        motor_rx_buffer_.erase(motor_rx_buffer_.begin(), motor_rx_buffer_.begin() + 2);
      }
    }
    else
    {
      // 不是有效帧头
      motor_rx_buffer_.erase(motor_rx_buffer_.begin());
    }
  }
}
```

## 完整的数据流程图

```
┌─────────────────────────────────────────────────────────────┐
│                        初始化阶段                            │
├─────────────────────────────────────────────────────────────┤
│ on_init()                                                   │
│   ↓                                                         │
│ 创建 Lambda 回调: [this](data) { parseMotorData(data); }   │
│   ↓                                                         │
│ 创建 SerialDevice(port, baud, callback)                    │
│   ↓                                                         │
│ SerialDevice 构造函数保存 callback                          │
│   ↓                                                         │
│ 启动线程: thread_ = std::thread(&SerialDevice::run, this)  │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                        运行阶段                              │
├─────────────────────────────────────────────────────────────┤
│ SerialDevice::run() 线程持续运行                            │
│   ↓                                                         │
│ poll(fds, 1, 10) - 等待串口数据                            │
│   ↓                                                         │
│ ::read(fd_, rx_buffer, size) - 读取串口数据                │
│   ↓                                                         │
│ receive_callback_(data) - 调用回调函数                     │
│   ↓                                                         │
│ parseMotorData(data) - 解析数据                            │
│   ↓                                                         │
│ 查找帧头 0xFF 0xAA                                         │
│   ↓                                                         │
│ 验证 CRC32                                                  │
│   ↓                                                         │
│ 提取数据到 motor_data_buffer_                              │
│   ↓                                                         │
│ 你的解析逻辑（TODO）                                        │
└─────────────────────────────────────────────────────────────┘
```

## 关键点

### 1. 回调函数机制

```cpp
// 定义回调类型
using ReceiveCallback = std::function<void(const std::vector<uint8_t>&)>;

// 保存回调
ReceiveCallback receive_callback_;

// 调用回调
receive_callback_(data);
```

### 2. Lambda 表达式

```cpp
// 创建 Lambda 表达式作为回调
auto motor_receive_callback = [this](const std::vector<uint8_t>& data) {
  this->parseMotorData(data);  // 捕获 this 指针，调用成员函数
};
```

### 3. 独立线程

`SerialDevice::run()` 在独立线程中运行，持续监听串口数据：

```cpp
// 构造函数中启动线程
thread_ = std::thread(&SerialDevice::run, this);

// run() 函数在独立线程中持续运行
void SerialDevice::run()
{
  while (running_ && rclcpp::ok())
  {
    // 持续监听和处理数据
  }
}
```

## 调用频率

- **parseMotorData** 的调用频率取决于：
  1. 下位机发送数据的频率
  2. 串口接收到数据的速度
  3. 每次可能接收到部分数据或完整数据包

- **不是定时调用**，而是**事件驱动**（有数据到达时调用）

## 线程安全

`parseMotorData` 在 `SerialDevice::run()` 线程中被调用，与主线程分离：

```
主线程                    SerialDevice 线程
  │                            │
  │ on_init()                  │
  │ ├─ 创建 SerialDevice       │
  │ └─ 启动线程 ──────────────→│ run() 开始
  │                            │
  │ read() 读取状态            │ poll() 等待数据
  │                            │ ↓
  │                            │ ::read() 读取串口
  │                            │ ↓
  │                            │ receive_callback_(data)
  │                            │ ↓
  │                            │ parseMotorData(data)
  │                            │ ↓
  │ read() 读取状态 ←──────────│ 更新 motor_data_buffer_
  │                            │
```

## 如何添加自己的解析逻辑

在 `parseMotorData` 函数的 TODO 位置添加：

```cpp
if (received_crc == calculated_crc)
{
  // 提取有效数据
  motor_data_buffer_.clear();
  motor_data_buffer_.insert(motor_data_buffer_.end(),
                            motor_rx_buffer_.begin() + 4,
                            motor_rx_buffer_.begin() + crc_offset);
  
  // ========== 在这里添加你的解析逻辑 ==========
  
  // 示例：解析浮点数数组
  size_t float_count = motor_data_buffer_.size() / sizeof(float);
  const float* float_data = reinterpret_cast<const float*>(motor_data_buffer_.data());
  
  for (size_t i = 0; i < float_count && i < motors_.size(); i++)
  {
    motors_[i]->angle_current = float_data[i];
  }
  
  // 或者：解析混合数据
  // size_t offset = 0;
  // uint8_t type = motor_data_buffer_[offset++];
  // uint32_t timestamp;
  // std::memcpy(&timestamp, motor_data_buffer_.data() + offset, 4);
  // offset += 4;
  // ...
  
  // 移除已处理的数据包
  motor_rx_buffer_.erase(motor_rx_buffer_.begin(), 
                        motor_rx_buffer_.begin() + total_packet_size);
}
```

## 总结

✅ **parseMotorData** 在 `SerialDevice::run()` 线程中被调用  
✅ **通过回调机制** 实现（Lambda 表达式）  
✅ **事件驱动** 而非定时调用  
✅ **独立线程** 运行，不阻塞主线程  
✅ **自动解析** 帧头、长度、CRC32  
✅ **提取数据** 到 `motor_data_buffer_` 供你使用  

现在你知道 `parseMotorData` 是如何被调用的了！🎯
