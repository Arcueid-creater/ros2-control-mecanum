# Chassis state message
std_msgs/Header header

# Current linear velocity in x direction (m/s)
float64 linear_x

# Current linear velocity in y direction (m/s)
float64 linear_y

# Current angular velocity around z axis (rad/s)
float64 angular_z

# Wheel velocities (rad/s)
float64[] wheel_velocities

# Control mode
uint8 mode
