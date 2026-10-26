#include "core/VioManager.h"
#include "ros/ROS2Visualizer.h"
#include "state/State.h"
#include "utils/sensor_data.h"
#include "utils/print.h"
#include <cassert>
#include <thread>

int main(int argc, char **argv) {
  assert(argc >= 2);
  const std::string config = argv[1];
  rclcpp::init(argc, argv);
  ov_core::Printer::setPrintLevel("WARNING");
  for (const bool enabled : {false, true}) {
    rclcpp::NodeOptions options;
    options.use_intra_process_comms(true);
    options.parameter_overrides({rclcpp::Parameter("camera_diagnostics", enabled)});
    auto node = std::make_shared<rclcpp::Node>(enabled ? "diagnostics_on" : "diagnostics_off", options);
    auto parser = std::make_shared<ov_core::YamlParser>(config);
    ov_msckf::VioManagerOptions params;
    params.print_and_load(parser);
    params.use_multi_threading_subs = false;
    params.use_multi_threading_pubs = false;
    auto app = std::make_shared<ov_msckf::VioManager>(params);
    auto visualizer = std::make_shared<ov_msckf::ROS2Visualizer>(node, app);
    assert(app->camera_diagnostics_active() == enabled);
    std::string received;
    auto subscription = node->create_subscription<std_msgs::msg::String>("status", 10,
        [&](std_msgs::msg::String::ConstSharedPtr message) { received = message->data; });
    ov_core::CameraData camera;
    camera.timestamp = 100.;
    for (int i = 0; i < params.state_options.num_cameras; ++i) {
      const auto calibration = app->get_state()->_cam_intrinsics_cameras.at(i);
      cv::Mat image(calibration->h(), calibration->w(), CV_8UC1);
      cv::randu(image, 0, 255);
      camera.sensor_ids.push_back(i);
      camera.images.push_back(image);
      camera.masks.push_back(cv::Mat::zeros(image.rows, image.cols, CV_8UC1));
    }
    app->feed_measurement_camera(camera);
    auto imu = std::make_shared<sensor_msgs::msg::Imu>();
    imu->header.stamp.sec = 100;
    imu->header.stamp.nanosec = 20000000;
    imu->linear_acceleration.z = 9.81;
    visualizer->callback_inertial(imu);
    for (int i = 0; i < 50 && received.empty(); ++i) {
      rclcpp::spin_some(node);
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    assert(!received.empty());
    assert((received.find("\"camera_diagnostics\":") != std::string::npos) == enabled);
    if (enabled) assert(received.find("\"camera_frames\":1") != std::string::npos);
    assert(received.find("\"visual_tracking_ready\":false") != std::string::npos);
  }
  rclcpp::shutdown();
}
