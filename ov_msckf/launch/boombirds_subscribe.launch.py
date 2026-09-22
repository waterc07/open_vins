# =============================================================================
# Boom Birds 专用 OpenVINS 订阅启动文件  ---  TEST-ONLY 合成配置
# =============================================================================
# 基于 ov_msckf/launch/subscribe.launch.py，默认加载
#   share/ov_msckf/config/boombirds_synthetic/estimator_config.yaml
# 该配置为合成占位（非真机标定，详见 config/boombirds_synthetic/*.yaml 头部）。
#
# 订阅话题（由 estimator_config.yaml -> kalibr_*_chain.yaml 的 rostopic 决定）:
#   /boom_birds/imu, /boom_birds/stereo/left_raw, /boom_birds/stereo/right_raw
# 如需临时改话题，可用 ROS 参数覆盖（源码 opencv_yaml_parse.h:157-179 优先读
#   <sensor>_<key> 形式的参数）:
#   ros2 run ov_msckf run_subscribe_msckf --ros-args -p config_path:=... \
#     -p imu0_rostopic:=/other/imu -p cam0_rostopic:=/other/left -p cam1_rostopic:=/other/right
#
# 用法:
#   ros2 launch ov_msckf boombirds_subscribe.launch.py
#   ros2 launch ov_msckf boombirds_subscribe.launch.py rviz_enable:=true
# =============================================================================

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo, OpaqueFunction
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from ament_index_python.packages import get_package_share_directory
import os

launch_args = [
    DeclareLaunchArgument(name="namespace", default_value="ov_msckf", description="namespace"),
    DeclareLaunchArgument(name="ov_enable", default_value="true", description="enable OpenVINS node"),
    DeclareLaunchArgument(name="rviz_enable", default_value="false", description="enable rviz node"),
    DeclareLaunchArgument(
        name="config",
        default_value="boombirds_synthetic",
        description="config folder name under share/ov_msckf/config (Boom Birds: boombirds_synthetic)",
    ),
    DeclareLaunchArgument(
        name="config_path",
        default_value="",
        description="path to estimator_config.yaml. If not given, determined based on provided 'config' above",
    ),
    DeclareLaunchArgument(
        name="verbosity",
        default_value="INFO",
        description="ALL, DEBUG, INFO, WARNING, ERROR, SILENT",
    ),
    DeclareLaunchArgument(
        name="use_stereo",
        default_value="true",
        description="if we have more than 1 camera, if we should try to track stereo constraints between pairs",
    ),
    DeclareLaunchArgument(
        name="max_cameras",
        default_value="2",
        description="how many cameras we have 1 = mono, 2 = stereo, >2 = binocular (all mono tracking)",
    ),
    DeclareLaunchArgument(
        name="save_total_state",
        default_value="false",
        description="record the total state with calibration and features to a txt file",
    ),
    # [BB-JAZZY] 布尔项需显式传入，见 node 参数块注释
    DeclareLaunchArgument("use_fej", default_value="true"),
    # [BB-JAZZY] 布尔项需显式传入，见 node 参数块注释
    DeclareLaunchArgument("calib_cam_extrinsics", default_value="false"),
    # [BB-JAZZY] 布尔项需显式传入，见 node 参数块注释
    DeclareLaunchArgument("calib_cam_intrinsics", default_value="false"),
    # [BB-JAZZY] 布尔项需显式传入，见 node 参数块注释
    DeclareLaunchArgument("calib_cam_timeoffset", default_value="false"),
    # [BB-JAZZY] 布尔项需显式传入，见 node 参数块注释
    DeclareLaunchArgument("calib_imu_intrinsics", default_value="false"),
    # [BB-JAZZY] 布尔项需显式传入，见 node 参数块注释
    DeclareLaunchArgument("calib_imu_g_sensitivity", default_value="false"),
    # [BB-JAZZY] 布尔项需显式传入，见 node 参数块注释
    DeclareLaunchArgument("try_zupt", default_value="false"),
    # [BB-JAZZY] 布尔项需显式传入，见 node 参数块注释
    DeclareLaunchArgument("init_dyn_use", default_value="false"),
    # [BB-JAZZY] 布尔项需显式传入，见 node 参数块注释
    DeclareLaunchArgument("init_dyn_mle_opt_calib", default_value="false"),
    # [BB-JAZZY] 布尔项需显式传入，见 node 参数块注释
    DeclareLaunchArgument("use_klt", default_value="true"),
    # [BB-JAZZY] 布尔项需显式传入，见 node 参数块注释
    DeclareLaunchArgument("use_aruco", default_value="false"),
    # [BB-JAZZY] 布尔项需显式传入，见 node 参数块注释
    DeclareLaunchArgument("use_mask", default_value="false"),
    # [BB-JAZZY] 布尔项需显式传入，见 node 参数块注释
    DeclareLaunchArgument("record_timing_information", default_value="false"),
    # [BB-JAZZY] 布尔项需显式传入，见 node 参数块注释
    DeclareLaunchArgument("downsample_cameras", default_value="false")
]


def launch_setup(context):
    config_path = LaunchConfiguration("config_path").perform(context)
    if not config_path:
        configs_dir = os.path.join(get_package_share_directory("ov_msckf"), "config")
        available_configs = os.listdir(configs_dir)
        config = LaunchConfiguration("config").perform(context)
        if config in available_configs:
            config_path = os.path.join(
                get_package_share_directory("ov_msckf"), "config", config, "estimator_config.yaml"
            )
        else:
            return [
                LogInfo(
                    msg="ERROR: unknown config: '{}' - Available configs are: {} - not starting OpenVINS".format(
                        config, ", ".join(available_configs)
                    )
                )
            ]
    else:
        if not os.path.isfile(config_path):
            return [
                LogInfo(
                    msg="ERROR: config_path file: '{}' - does not exist. - not starting OpenVINS".format(config_path)
                )
            ]

    node1 = Node(
        package="ov_msckf",
        executable="run_subscribe_msckf",
        condition=IfCondition(LaunchConfiguration("ov_enable")),
        namespace=LaunchConfiguration("namespace"),
        output="screen",
        parameters=[
            {"verbosity": LaunchConfiguration("verbosity")},
            {"use_stereo": LaunchConfiguration("use_stereo")},
            {"max_cameras": LaunchConfiguration("max_cameras")},
            {"save_total_state": LaunchConfiguration("save_total_state")},
            {"config_path": config_path},
            # [BB-JAZZY FIX] estimator_config.yaml 中的布尔项必须以 ROS 参数显式传入：
            # 实测（2026-09-22）只给 config_path 时，YAML 布尔项被解析为非布尔值，
            # 报 "the node init_dyn_use has an invalid boolean type of []"，
            # 随后 "unable to parse all parameters, please fix" 并异常退出（exit -11）。
            # 显式传类型正确的布尔值后节点正常启动并完成订阅（已在同一环境实测）。
            {"use_fej": ParameterValue(LaunchConfiguration("use_fej"), value_type=bool)},
            {"calib_cam_extrinsics": ParameterValue(LaunchConfiguration("calib_cam_extrinsics"), value_type=bool)},
            {"calib_cam_intrinsics": ParameterValue(LaunchConfiguration("calib_cam_intrinsics"), value_type=bool)},
            {"calib_cam_timeoffset": ParameterValue(LaunchConfiguration("calib_cam_timeoffset"), value_type=bool)},
            {"calib_imu_intrinsics": ParameterValue(LaunchConfiguration("calib_imu_intrinsics"), value_type=bool)},
            {"calib_imu_g_sensitivity": ParameterValue(LaunchConfiguration("calib_imu_g_sensitivity"), value_type=bool)},
            {"try_zupt": ParameterValue(LaunchConfiguration("try_zupt"), value_type=bool)},
            {"init_dyn_use": ParameterValue(LaunchConfiguration("init_dyn_use"), value_type=bool)},
            {"init_dyn_mle_opt_calib": ParameterValue(LaunchConfiguration("init_dyn_mle_opt_calib"), value_type=bool)},
            {"use_klt": ParameterValue(LaunchConfiguration("use_klt"), value_type=bool)},
            {"use_aruco": ParameterValue(LaunchConfiguration("use_aruco"), value_type=bool)},
            {"use_mask": ParameterValue(LaunchConfiguration("use_mask"), value_type=bool)},
            {"record_timing_information": ParameterValue(LaunchConfiguration("record_timing_information"), value_type=bool)},
            {"downsample_cameras": ParameterValue(LaunchConfiguration("downsample_cameras"), value_type=bool)},
        ],
    )

    node2 = Node(
        package="rviz2",
        executable="rviz2",
        condition=IfCondition(LaunchConfiguration("rviz_enable")),
        arguments=[
            "-d" + os.path.join(get_package_share_directory("ov_msckf"), "launch", "display_ros2.rviz"),
            "--ros-args",
            "--log-level",
            "warn",
        ],
    )

    return [node1, node2]


def generate_launch_description():
    opfunc = OpaqueFunction(function=launch_setup)
    ld = LaunchDescription(launch_args)
    ld.add_action(opfunc)
    return ld
