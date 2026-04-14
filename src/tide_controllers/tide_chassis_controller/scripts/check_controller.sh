#!/bin/bash

echo "=========================================="
echo "Chassis Controller 诊断脚本"
echo "=========================================="
echo ""

# 检查编译
echo "1. 检查编译状态..."
if [ -f "install/tide_chassis_controller/lib/libtide_chassis_controller.so" ]; then
    echo "   ✅ 控制器库已编译: libtide_chassis_controller.so"
else
    echo "   ❌ 控制器库未找到，请先编译:"
    echo "      colcon build --packages-select tide_chassis_controller"
    exit 1
fi

# 检查 plugin 描述文件
echo ""
echo "2. 检查 plugin 描述文件..."
if [ -f "install/tide_chassis_controller/share/tide_chassis_controller/tide_chassis_controller.xml" ]; then
    echo "   ✅ Plugin XML 文件存在"
    echo "   内容:"
    cat install/tide_chassis_controller/share/tide_chassis_controller/tide_chassis_controller.xml | grep -A 2 "class name"
else
    echo "   ❌ Plugin XML 文件未找到"
fi

# 检查消息类型
echo ""
echo "3. 检查消息类型..."
if [ -d "install/tide_msgs/include/tide_msgs/tide_msgs/msg" ]; then
    if [ -f "install/tide_msgs/include/tide_msgs/tide_msgs/msg/chassis_cmd.hpp" ]; then
        echo "   ✅ ChassisCmd 消息已生成"
    else
        echo "   ❌ ChassisCmd 消息未找到"
    fi
    
    if [ -f "install/tide_msgs/include/tide_msgs/tide_msgs/msg/chassis_state.hpp" ]; then
        echo "   ✅ ChassisState 消息已生成"
    else
        echo "   ❌ ChassisState 消息未找到"
    fi
else
    echo "   ❌ tide_msgs 未编译，请先编译:"
    echo "      colcon build --packages-select tide_msgs"
fi

# 检查 pluginlib
echo ""
echo "4. 检查 pluginlib 注册..."
echo "   运行以下命令查看所有可用控制器:"
echo "   ros2 pkg prefix tide_chassis_controller"
echo "   ros2 run controller_manager list_controller_types | grep chassis"

echo ""
echo "=========================================="
echo "5. 启动测试步骤:"
echo "=========================================="
echo "   a) 编译所有包:"
echo "      colcon build --packages-select tide_chassis_controller tide_msgs tide_ctrl_bringup"
echo ""
echo "   b) Source 环境:"
echo "      source install/setup.bash"
echo ""
echo "   c) 查看可用控制器类型:"
echo "      ros2 run controller_manager list_controller_types | grep chassis"
echo ""
echo "   d) 启动机器人:"
echo "      ros2 launch tide_ctrl_bringup tide_ctrl_bringup.launch.py robot_type:=sentry"
echo ""
echo "   e) 检查控制器状态:"
echo "      ros2 control list_controllers"
echo ""
echo "   f) 查看日志:"
echo "      ros2 topic echo /rosout | grep -i chassis"
echo ""
echo "=========================================="
