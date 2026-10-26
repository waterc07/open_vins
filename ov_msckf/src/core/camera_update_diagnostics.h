#ifndef OV_MSCKF_CAMERA_UPDATE_DIAGNOSTICS_H
#define OV_MSCKF_CAMERA_UPDATE_DIAGNOSTICS_H

#include <cstddef>
#include <map>
#include <mutex>
#include <sstream>
#include <iomanip>

namespace ov_msckf {

struct MsckfUpdateDiagnostics {
  size_t candidates = 0;
  size_t rejected_short_track = 0;
  size_t rejected_triangulation = 0;
  size_t rejected_refinement = 0;
  size_t rejected_chi2 = 0;
  size_t accepted_constraints = 0;
  bool state_update_applied = false;
};

struct CameraUpdateDiagnostics {
  double camera_stamp_s = -1;
  const char *stage = "no_camera_frame";
  std::map<size_t, size_t> tracked_per_camera;
  size_t tracked_unique_ids = 0;
  MsckfUpdateDiagnostics msckf;
};

// Camera worker writes complete frames; the IMU/status worker reads a copy.
class CameraDiagnosticsStore {
public:
  void record(const CameraUpdateDiagnostics &frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_ = frame;
    ++frames_;
    totals_.candidates += frame.msckf.candidates;
    totals_.rejected_short_track += frame.msckf.rejected_short_track;
    totals_.rejected_triangulation += frame.msckf.rejected_triangulation;
    totals_.rejected_refinement += frame.msckf.rejected_refinement;
    totals_.rejected_chi2 += frame.msckf.rejected_chi2;
    totals_.accepted_constraints += frame.msckf.accepted_constraints;
    applied_ += frame.msckf.state_update_applied;
  }

  std::string json() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream out;
    out << std::setprecision(17) << "{\"camera_frames\":" << frames_
        << ",\"camera_stamp_s\":" << last_.camera_stamp_s
        << ",\"stage\":\"" << last_.stage << "\",\"tracked_per_camera\":{";
    bool first = true;
    for (const auto &camera : last_.tracked_per_camera) {
      if (!first) out << ",";
      first = false;
      out << "\"" << camera.first << "\":" << camera.second;
    }
    out << "},\"tracked_unique_ids\":" << last_.tracked_unique_ids;
    auto counts = [&out](const MsckfUpdateDiagnostics &counts) {
      out << "{\"candidates\":" << counts.candidates
          << ",\"rejected_short_track\":" << counts.rejected_short_track
          << ",\"rejected_triangulation\":" << counts.rejected_triangulation
          << ",\"rejected_refinement\":" << counts.rejected_refinement
          << ",\"rejected_chi2\":" << counts.rejected_chi2
          << ",\"accepted_constraints\":" << counts.accepted_constraints << "}";
    };
    out << ",\"msckf_last\":";
    counts(last_.msckf);
    out << ",\"msckf_state_update_applied\":" << (last_.msckf.state_update_applied ? "true" : "false")
        << ",\"msckf_totals\":";
    counts(totals_);
    out << ",\"msckf_applied_frames\":" << applied_ << "}";
    return out.str();
  }

private:
  mutable std::mutex mutex_;
  CameraUpdateDiagnostics last_;
  MsckfUpdateDiagnostics totals_;
  size_t frames_ = 0;
  size_t applied_ = 0;
};

class CameraDiagnosticsScope {
public:
  CameraDiagnosticsScope(CameraDiagnosticsStore *store, double timestamp) : store_(store) {
    frame.camera_stamp_s = timestamp;
    frame.stage = "tracking";
  }
  ~CameraDiagnosticsScope() { if (store_) store_->record(frame); }
  CameraUpdateDiagnostics frame;
private:
  CameraDiagnosticsStore *store_;
};

} // namespace ov_msckf
#endif
