# Test vehicle

A small ROS 2 project for driving a car around a Gazebo city world. The world and launch file are in `worlds/` and `launch/`; the keyboard controller is in `src/`.

## What you need

- ROS 2 with `ament_cmake`, `rclcpp`, and `geometry_msgs`
- Gazebo Fortress (`ign gazebo`)
- `ros_gz_bridge`

Source your ROS 2 setup file before building or running the package.

## Build

From your ROS 2 workspace:

```bash
colcon build --packages-select testvehicle
source install/setup.bash
```

## Drive the car

Start the simulation and bridge:

```bash
ros2 launch testvehicle city_drive.launch.py
```

Open another terminal, source the workspace there too, then run the controller:

```bash
ros2 run testvehicle keyboard_control
```

Keep the controller in a terminal where it can read your key presses.

| Key | Action |
| --- | --- |
| `W` / `S` | Accelerate / reverse |
| `A` / `D` | Steer |
| `↑` / `↓` | Change the speed preset |
| `Space` | Brake |
| `Esc` or `Ctrl-C` | Quit the controller |

The controller sends velocity commands on `/car/cmd_vel`. The bridge configuration is in `config/bridge.yaml`.
