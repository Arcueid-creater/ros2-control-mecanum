/*
 * 电机数据解析示例
 * 
 * 这个文件展示了如何在 parseMotorData() 函数中添加自定义的数据解析逻辑
 */

#include <cstring>
#include <vector>
#include <cstdint>

// 示例1：解析单个电机的完整数据
void parseMotorData_Example1(const std::vector<uint8_t>& motor_data_buffer_)
{
  // 假设数据格式：
  // motor_id (1字节) + position (4字节) + velocity (4字节) + current (4字节) + temperature (4字节)
  // 总共17字节
  
  if (motor_data_buffer_.size() < 17) {
    return; // 数据不足
  }
  
  size_t offset = 0;
  
  // 读取电机ID
  uint8_t motor_id = motor_data_buffer_[offset++];
  
  // 读取位置
  float position;
  std::memcpy(&position, motor_data_buffer_.data() + offset, sizeof(float));
  offset += sizeof(float);
  
  // 读取速度
  float velocity;
  std::memcpy(&velocity, motor_data_buffer_.data() + offset, sizeof(float));
  offset += sizeof(float);
  
  // 读取电流
  float current;
  std::memcpy(&current, motor_data_buffer_.data() + offset, sizeof(float));
  offset += sizeof(float);
  
  // 读取温度
  float temperature;
  std::memcpy(&temperature, motor_data_buffer_.data() + offset, sizeof(float));
  offset += sizeof(float);
  
  // 使用数据
  printf("Motor %d: pos=%.3f, vel=%.3f, cur=%.3f, temp=%.1f\n",
         motor_id, position, velocity, current, temperature);
}

// 示例2：解析多个电机的数据
void parseMotorData_Example2(const std::vector<uint8_t>& motor_data_buffer_)
{
  // 假设数据格式：
  // motor_count (1字节) + [motor_data × N]
  // 每个motor_data: motor_id (1字节) + position (4字节) + velocity (4字节) + current (4字节)
  
  if (motor_data_buffer_.size() < 1) {
    return;
  }
  
  size_t offset = 0;
  
  // 读取电机数量
  uint8_t motor_count = motor_data_buffer_[offset++];
  
  // 每个电机数据13字节
  const size_t motor_data_size = 1 + 4 + 4 + 4;
  
  if (motor_data_buffer_.size() < 1 + motor_count * motor_data_size) {
    return; // 数据不足
  }
  
  // 遍历每个电机
  for (uint8_t i = 0; i < motor_count; i++) {
    uint8_t motor_id = motor_data_buffer_[offset++];
    
    float position, velocity, current;
    std::memcpy(&position, motor_data_buffer_.data() + offset, 4);
    offset += 4;
    std::memcpy(&velocity, motor_data_buffer_.data() + offset, 4);
    offset += 4;
    std::memcpy(&current, motor_data_buffer_.data() + offset, 4);
    offset += 4;
    
    printf("Motor %d: pos=%.3f, vel=%.3f, cur=%.3f\n",
           motor_id, position, velocity, current);
  }
}

// 示例3：解析紧凑格式（全部浮点数）
void parseMotorData_Example3(const std::vector<uint8_t>& motor_data_buffer_)
{
  // 假设数据格式：全部是浮点数，按顺序排列
  // [motor1_pos, motor1_vel, motor1_cur, motor2_pos, motor2_vel, motor2_cur, ...]
  
  const size_t float_size = sizeof(float);
  const size_t values_per_motor = 3; // position, velocity, current
  
  size_t total_floats = motor_data_buffer_.size() / float_size;
  size_t motor_count = total_floats / values_per_motor;
  
  if (motor_data_buffer_.size() % float_size != 0) {
    return; // 数据长度不是float的整数倍
  }
  
  size_t offset = 0;
  for (size_t i = 0; i < motor_count; i++) {
    float position, velocity, current;
    
    std::memcpy(&position, motor_data_buffer_.data() + offset, float_size);
    offset += float_size;
    std::memcpy(&velocity, motor_data_buffer_.data() + offset, float_size);
    offset += float_size;
    std::memcpy(&current, motor_data_buffer_.data() + offset, float_size);
    offset += float_size;
    
    printf("Motor %zu: pos=%.3f, vel=%.3f, cur=%.3f\n",
           i + 1, position, velocity, current);
  }
}

// 示例4：解析带时间戳的数据
void parseMotorData_Example4(const std::vector<uint8_t>& motor_data_buffer_)
{
  // 假设数据格式：
  // timestamp (4字节) + motor_count (1字节) + [motor_data × N]
  
  if (motor_data_buffer_.size() < 5) {
    return;
  }
  
  size_t offset = 0;
  
  // 读取时间戳
  uint32_t timestamp;
  std::memcpy(&timestamp, motor_data_buffer_.data() + offset, sizeof(uint32_t));
  offset += sizeof(uint32_t);
  
  // 读取电机数量
  uint8_t motor_count = motor_data_buffer_[offset++];
  
  printf("Timestamp: %u, Motor count: %d\n", timestamp, motor_count);
  
  // 解析每个电机数据（假设每个17字节）
  const size_t motor_data_size = 17;
  for (uint8_t i = 0; i < motor_count && offset + motor_data_size <= motor_data_buffer_.size(); i++) {
    uint8_t motor_id = motor_data_buffer_[offset++];
    
    float position, velocity, current, temperature;
    std::memcpy(&position, motor_data_buffer_.data() + offset, 4); offset += 4;
    std::memcpy(&velocity, motor_data_buffer_.data() + offset, 4); offset += 4;
    std::memcpy(&current, motor_data_buffer_.data() + offset, 4); offset += 4;
    std::memcpy(&temperature, motor_data_buffer_.data() + offset, 4); offset += 4;
    
    printf("  Motor %d: pos=%.3f, vel=%.3f, cur=%.3f, temp=%.1f\n",
           motor_id, position, velocity, current, temperature);
  }
}

// 示例5：在实际代码中集成（tide_hw_interface.cpp）
/*
void TideHardwareInterface::parseMotorData(const std::vector<uint8_t>& data)
{
  motor_rx_buffer_.insert(motor_rx_buffer_.end(), data.begin(), data.end());

  while (motor_rx_buffer_.size() >= 8)
  {
    if (motor_rx_buffer_[0] == 0xFF && motor_rx_buffer_[1] == 0xAA)
    {
      uint16_t data_length;
      std::memcpy(&data_length, motor_rx_buffer_.data() + 2, sizeof(uint16_t));
      
      size_t total_packet_size = 4 + data_length + 4;
      
      if (motor_rx_buffer_.size() < total_packet_size) break;
      
      size_t crc_offset = total_packet_size - 4;
      uint32_t received_crc;
      std::memcpy(&received_crc, motor_rx_buffer_.data() + crc_offset, sizeof(uint32_t));
      
      uint32_t calculated_crc = calculateCRC32(motor_rx_buffer_.data(), crc_offset);
      
      if (received_crc == calculated_crc)
      {
        motor_data_buffer_.clear();
        motor_data_buffer_.insert(motor_data_buffer_.end(),
                                  motor_rx_buffer_.begin() + 4,
                                  motor_rx_buffer_.begin() + crc_offset);
        
        // ========== 在这里添加你的解析代码 ==========
        
        // 方案1：单个电机
        if (motor_data_buffer_.size() == 17) {
          size_t offset = 0;
          uint8_t motor_id = motor_data_buffer_[offset++];
          
          float position, velocity, current, temperature;
          std::memcpy(&position, motor_data_buffer_.data() + offset, 4); offset += 4;
          std::memcpy(&velocity, motor_data_buffer_.data() + offset, 4); offset += 4;
          std::memcpy(&current, motor_data_buffer_.data() + offset, 4); offset += 4;
          std::memcpy(&temperature, motor_data_buffer_.data() + offset, 4); offset += 4;
          
          // 更新电机状态
          for (auto& motor : motors_) {
            if (motor->config_.tx_id == motor_id) {
              motor->angle_current = position;
              motor->measure.speed_aps = velocity;
              motor->measure.real_current = current;
              motor->measure.temperature = temperature;
              motor->status = MOTOR_OK;
              motor->update_timestamp(rclcpp::Clock().now());
              break;
            }
          }
        }
        
        // 方案2：多个电机
        else if (motor_data_buffer_.size() > 1) {
          size_t offset = 0;
          uint8_t motor_count = motor_data_buffer_[offset++];
          
          for (uint8_t i = 0; i < motor_count && offset + 17 <= motor_data_buffer_.size(); i++) {
            uint8_t motor_id = motor_data_buffer_[offset++];
            
            float position, velocity, current, temperature;
            std::memcpy(&position, motor_data_buffer_.data() + offset, 4); offset += 4;
            std::memcpy(&velocity, motor_data_buffer_.data() + offset, 4); offset += 4;
            std::memcpy(&current, motor_data_buffer_.data() + offset, 4); offset += 4;
            std::memcpy(&temperature, motor_data_buffer_.data() + offset, 4); offset += 4;
            
            for (auto& motor : motors_) {
              if (motor->config_.tx_id == motor_id) {
                motor->angle_current = position;
                motor->measure.speed_aps = velocity;
                motor->measure.real_current = current;
                motor->measure.temperature = temperature;
                motor->status = MOTOR_OK;
                motor->update_timestamp(rclcpp::Clock().now());
                break;
              }
            }
          }
        }
        
        // ========== 解析代码结束 ==========
        
        motor_rx_buffer_.erase(motor_rx_buffer_.begin(), 
                              motor_rx_buffer_.begin() + total_packet_size);
      }
      else
      {
        motor_rx_buffer_.erase(motor_rx_buffer_.begin(), motor_rx_buffer_.begin() + 2);
      }
    }
    else
    {
      motor_rx_buffer_.erase(motor_rx_buffer_.begin());
    }
  }
}
*/

// 使用建议：
// 1. 根据你的实际协议选择合适的示例
// 2. 复制相应的解析代码到 tide_hw_interface.cpp 的 parseMotorData() 函数中
// 3. 根据需要调整数据格式和字段
// 4. 确保与下位机的数据格式完全一致
