# Tide Hardware Interface IMU 移植说明文档

本文档详细说明了将 "ros2-humble 例程" 中的达妙 (DM) IMU 驱动逻辑移植到 `tide_hw_interface` 框架中的改动内容。

## 1. 协议实现 (Protocol Implementation)
- **CRC16 校验**: 在 `tide_hw_interface.cpp` 中移植了基于 CCITT (0x1021) 的表驱动 CRC16 算法，用于验证 IMU 串口数据包的完整性。
- **数据包结构**: 定义了 `ImuPacket` 结构体，精确匹配 19 字节的硬件通信协议：
  - 帧头: `0x55 0xAA`
  - 数据: 3个 `float32` (Roll, Pitch, Yaw)
  - 校验: 16位 CRC
  - 帧尾: `0x0A`

## 2. 核心逻辑改动 (Core Logic Changes)
### 串口数据分发 (`parseSerialData`)
- 增强了缓冲区解析能力，支持在一个串口流中同时解析**电机反馈包 (0x5A)** 和 **IMU 数据包 (0x55 0xAA)**。
- 引入了状态机式的解析逻辑，能够自动跳过无效字节并定位正确的帧头。

### IMU 数据处理 (`parseImuData`)
- **单位转换**: 将 IMU 原始输出的“度”转换为 ROS 2 标准的“弧度”。
- **姿态解算**: 实现了 Euler (ZYX) 到 Quaternion (四元数) 的转换，确保姿态信息在 RViz 等工具中能够正确显示。

## 3. ROS 2 接口集成
- **新增 Node 句柄**: 在硬件接口内部启动了一个辅助节点 `tide_hw_imu_node` 用于管理发布者。
- **发布话题**:
  - `/imu/data` (`sensor_msgs/msg/Imu`): 发布包含姿态四元数的标准 IMU 消息。
  - `/imu/rpy` (`geometry_msgs/msg/Vector3Stamped`): 发布原始 RPY 数据（单位：度），方便实时监控。
- **坐标系**: 默认使用 `imu_link` 作为 frame_id。

## 4. 工程配置更新
- **package.xml**: 添加了 `sensor_msgs` 和 `geometry_msgs` 的依赖。
- **CMakeLists.txt**: 
  - 添加了对 `sensor_msgs` 和 `geometry_msgs` 的 `find_package` 调用。
  - 更新了编译选项，确保消息头文件能正确链接。

## 5. 文件变更列表
- `@d:\git_project\tide_controls_ws\src\tide_control\tide_hw_interface\include\tide_hw_interface.hpp`: 增加结构体定义、发布者成员及解析函数声明。
- `@d:\git_project\tide_controls_ws\src\tide_control\tide_hw_interface\src\tide_hw_interface.cpp`: 实现 CRC 校验、IMU 解析及话题发布逻辑。
- `@d:\git_project\tide_controls_ws\src\tide_control\tide_hw_interface\CMakeLists.txt`: 更新依赖库配置。
- `@d:\git_project\tide_controls_ws\src\tide_control\tide_hw_interface\package.xml`: 更新包依赖声明。
