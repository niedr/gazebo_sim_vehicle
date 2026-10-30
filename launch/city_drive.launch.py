import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import ExecuteProcess


def generate_launch_description():
    pkg_dir = get_package_share_directory("testvehicle")
    world_path = os.path.join(pkg_dir, "worlds", "car_city.sdf")
    bridge_config = os.path.join(pkg_dir, "config", "bridge.yaml")

    return LaunchDescription([
        ExecuteProcess(
            cmd=["ign", "gazebo", "-r", world_path],
            output="screen",
        ),
        ExecuteProcess(
            cmd=[
                "ros2", "run", "ros_gz_bridge", "static_bridge",
                "--ros-args", "-p", f"config_file:={bridge_config}",
            ],
            output="screen",
        ),
    ])
