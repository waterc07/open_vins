# ROS 2 图像缓存

`run_subscribe_msckf` 参数 `camera_queue_max_count` 默认 10，`camera_queue_max_span_s` 默认 0.5 s，均要求正值。单目和双目回调入队后按采样时间排序，超过容量或时间跨度时丢最旧帧，5 s 限频告警记录本次和累计丢帧数。正常 IMU 回调仍按原逻辑消费队列，不改变跟踪频率、特征数量或估计器更新。

无 IMU 或处理积压时，缓存有界不代表 VIO 有效。大量丢帧仍须排查输入频率、时间偏移和计算负载。

离线队列回归：在母仓库加载 Python 环境后运行 `python3 -m pytest companion/ros2_ws/src/boom_birds_nav/test/test_openvins_camera_queue.py -q`；需要 g++。
