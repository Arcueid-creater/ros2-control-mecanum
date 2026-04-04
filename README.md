# TideControls

## 注意事项

* **请使用foxglove取代rviz和rqt**
* 请根据需要自行配置foxglove面板，如果可视化的模型错乱，请将```面板->场景->网络上轴```选项设置为```z-up```
* 运行设备需要更换rt实时内核，推荐[xanmod-rt内核](https://xanmod.org/)👍
* 编译命令```./build.sh all```(全编译)、```./build.sh package```(编译单个包，package为你要编译的包名)
* 若需要debug调试，编译时请添加 **--debug** 参数，如```./build.sh all --debug```，并在vscode的左侧调试选项中选择 [launch.json](.vscode/launch.json) 配置文件
* can设备使能命令```./enable_socketcan can0```(单个使能，can0为需要使能的端口)、```./enable_socketcan all```全部使能(默认只有can0和can1,若超过两个can设备请修改脚本)
*  运行仿真：```ros2 launch tide_ctrl_bringup tide_ctrl_bringup.launch.py sim_mode:=true robot_type:=sentry world:=RMUL2025```(机器人类型和gazebo地图模型修改为你需要的)
 * 如果嫌gazebo太卡，不需要gazebo仿真，请将兵种_real.xacro中的```enable_virtual_control```参数设置为```true```，并将兵种_real.yaml中的```open_loop```参数设置为```true```（需要控制真实硬件时请改为false关闭开环模式）,然后```运行实车```,打开```foxgolve```可视化软件即可，无需担心疯车，因为这不会启用真实硬件
*  ⚠️运行实车：```ros2 launch tide_ctrl_bringup tide_ctrl_bringup.launch.py sim_mode:=false robot_type:=sentry```

⚠️：第一次运行实车前，如果需要控制真实硬件，请将兵种_real.xacro中的```enable_virtual_control```参数设置为```false```关闭虚拟控制，以及```need_calibration```参数设置为```true```开启校准模式，手动将云台摆放到你期望的初始位置，将终端输出结果填到对应电机关节的```offset```参数（较烦琐待优化），否则会有疯车风险！！

ros2 topic echo /serial_received_data

## 安装依赖
```
rosdepc install -r --from-paths src --ignore-src --rosdistro $ROS_DISTRO -y
```
