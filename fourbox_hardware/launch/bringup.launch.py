import os
from launch import LaunchDescription
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.substitutions import PathJoinSubstitution, Command, FindExecutable
from launch.actions import RegisterEventHandler
from launch.event_handlers import OnProcessStart

def generate_launch_description():
    # Аргументы запуска

    # Пути к пакетам
    pkg_hardware_name = 'fourbox_hardware'
    pkg_description_name = 'fourbox_description'
    
    
    
    pkg_hardware_path = FindPackageShare(pkg_hardware_name).find(pkg_hardware_name)
    pkg_description_path = PathJoinSubstitution([
        FindPackageShare(pkg_description_name), 'description', 'fourbox.urdf.xacro'
    ])
    robot_controllers_path = PathJoinSubstitution([
        FindPackageShare(pkg_hardware_name), 'config', 'controllers.yaml'
    ])

    hardware_file = PathJoinSubstitution([FindPackageShare(pkg_hardware_name), 'config', 'robot_hardware.yaml'])
    
    # ✅ ПРАВИЛЬНО: robot_description без ParameterValue!
    robot_description_content = Command([
        FindExecutable(name='xacro'),
        ' ',
        pkg_description_path
    ])

    robot_description = {'robot_description': robot_description_content}
    
    # Robot State Publisher
    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        output='screen',
        parameters=[{'robot_description': robot_description_content}]
    )
    
    # Controller Manager
    # controller_manager = Node(
    #     package='controller_manager',
    #     executable='ros2_control_node',
    #     parameters=[{'robot_description': robot_description_content},
    #                 robot_controllers_path],
    #     output='screen'
    # )

    controller_manager = Node(
        package='controller_manager',
        executable='ros2_control_node',
        parameters=[robot_description, robot_controllers_path],
        output='screen'
    )
    
    # Спавнеры с ПРАВИЛЬНЫМИ аргументами
    joint_state_broadcaster_spawner = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['joint_state_broadcaster', '--controller-manager', '/controller_manager'],
        output='screen',
    )
    
    mecanum_controller_spawner = Node(
        package='controller_manager',
        executable='spawner', 
        arguments=['fourbox_mecanum_controller', '--controller-manager', '/controller_manager'],
        output='screen',
    )
    
    # Последовательный запуск
    joint_state_broadcaster_event = RegisterEventHandler(
        OnProcessStart(
            target_action=controller_manager,
            on_start=[joint_state_broadcaster_spawner]
        )
    )
    
    mecanum_controller_event = RegisterEventHandler(
        OnProcessStart(
            target_action=joint_state_broadcaster_spawner,
            on_start=[mecanum_controller_spawner]
        )
    )
    
    return LaunchDescription([
        robot_state_publisher,
        controller_manager,
        joint_state_broadcaster_event,
        mecanum_controller_event,
    ])
