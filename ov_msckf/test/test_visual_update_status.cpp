#include "core/visual_update_status.h"
#include <cassert>
#include <limits>

int main() {
  ov_msckf::VisualUpdateStatus status;
  assert(!status.fresh(10.0, 0.5)); // Initialization/propagation alone cannot open navigation output.
  status.record(10.0, 0);
  assert(status.count() == 0);
  status.record(std::numeric_limits<double>::quiet_NaN(), 8);
  assert(status.count() == 0);
  status.record(10.0, 8);
  assert(status.count() == 1 && status.features() == 8);
  assert(status.fresh(10.0, 0.5));
  assert(status.fresh(10.5, 0.5));
  assert(!status.fresh(10.5001, 0.5));
  assert(!status.fresh(9.9, 0.5));
  assert(!status.fresh(10.1, 0));
  assert(!status.fresh(10.1, std::numeric_limits<double>::infinity()));
  assert(!status.fresh(std::numeric_limits<double>::quiet_NaN(), 0.5));
  status.record(10.6, 0); // A frame with all visual measurements rejected must not refresh the gate.
  assert(status.timestamp() == 10.0 && !status.fresh(10.6, 0.5));
  status.record(10.7, 3);
  assert(status.count() == 2 && status.features() == 3 && status.fresh(10.8, 0.5));
}
