"""
Example launch file for chassis controller
Add this to your robot's launch file
"""

from launch_ros.actions import Node

# Chassis controller spawner node
chassis_controller = Node(
    package="controller_manager",
    executable="spawner",
    arguments=[
        "chassis_controller",
        "--controller-manager",
        "/controller_manager",
    ],
)

# To use in your launch file, add chassis_controller to your GroupAction:
# Example:
# robot_controllers = GroupAction([
#     joint_state_broadcaster,
#     gimbal_controller,
#     chassis_controller,  # Add this line
# ])
