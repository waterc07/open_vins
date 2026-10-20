#ifndef OV_MSCKF_VISUAL_UPDATE_STATUS_H
#define OV_MSCKF_VISUAL_UPDATE_STATUS_H

#include <atomic>
#include <cmath>
#include <cstddef>

namespace ov_msckf {

class VisualUpdateStatus {
public:
  void record(double timestamp, size_t accepted_features) {
    if (!std::isfinite(timestamp) || timestamp < 0 || accepted_features == 0)
      return;
    last_timestamp.store(timestamp);
    last_features.store(accepted_features);
    update_count.fetch_add(1);
  }

  bool fresh(double timestamp, double timeout) const {
    const double last = last_timestamp.load();
    const double age = timestamp - last;
    return update_count.load() > 0 && last >= 0 && std::isfinite(timestamp) &&
           std::isfinite(timeout) && timeout > 0 && age >= 0 && age <= timeout;
  }

  double timestamp() const { return last_timestamp.load(); }
  size_t count() const { return update_count.load(); }
  size_t features() const { return last_features.load(); }

private:
  std::atomic<double> last_timestamp{-1.0};
  std::atomic<size_t> update_count{0};
  std::atomic<size_t> last_features{0};
};

} // namespace ov_msckf
#endif
