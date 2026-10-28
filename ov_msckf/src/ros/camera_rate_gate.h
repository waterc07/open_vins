#pragma once
#include <cmath>

namespace ov_msckf {
inline bool accept_camera_timestamp(double timestamp, double previous, double frequency) {
  if (!std::isfinite(timestamp) || !std::isfinite(frequency) || frequency < 0 || timestamp <= previous)
    return false;
  return frequency == 0 || timestamp >= previous + 1.0 / frequency;
}
}
