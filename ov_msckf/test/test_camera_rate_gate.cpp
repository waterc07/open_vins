#include "ros/camera_rate_gate.h"
#include <cassert>
#include <limits>

int main() {
  double previous = -INFINITY;
  // Source limited to 20 Hz with timestamp jitter: no second decimator.
  const double timestamps[] = {1., 1.049, 1.101, 1.149, 1.201};
  for (double stamp : timestamps) {
    assert(ov_msckf::accept_camera_timestamp(stamp, previous, 0.));
    previous = stamp;
  }
  assert(!ov_msckf::accept_camera_timestamp(previous, previous, 0.));
  assert(!ov_msckf::accept_camera_timestamp(previous - .01, previous, 0.));
  assert(!ov_msckf::accept_camera_timestamp(NAN, previous, 0.));
  assert(!ov_msckf::accept_camera_timestamp(2., previous, -1.));
  assert(!ov_msckf::accept_camera_timestamp(1.049, 1., 20.));
  assert(ov_msckf::accept_camera_timestamp(1.051, 1., 20.));
}
