#pragma once
#include <algorithm>
#include <deque>
#include <cstddef>

namespace ov_msckf {
// Caller holds the camera queue mutex. T supplies a measurement timestamp.
template <typename T>
size_t trim_camera_queue(std::deque<T> &queue, size_t max_count, double max_span_s) {
  std::sort(queue.begin(), queue.end());
  const size_t before = queue.size();
  while (!queue.empty() && (queue.size() > max_count ||
         queue.back().timestamp - queue.front().timestamp > max_span_s)) {
    queue.pop_front();
  }
  return before - queue.size();
}
}
