#include "core/camera_update_diagnostics.h"
#include "core/visual_update_status.h"
#include "update/UpdaterMSCKF.h"
#include "state/State.h"
#include "feat/Feature.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  using namespace ov_msckf;
  CameraDiagnosticsStore store;
  assert(store.json().find("\"camera_frames\":0") != std::string::npos);
  {
    CameraDiagnosticsScope scope(&store, 12.0);
    scope.frame.stage = "initializing";
    scope.frame.tracked_per_camera = {{0, 30}, {1, 28}};
    scope.frame.tracked_unique_ids = 35;
  }
  assert(store.json().find("\"tracked_unique_ids\":35") != std::string::npos);
  // An all-rejected frame must replace last-frame counts, without opening navigation.
  VisualUpdateStatus gate;
  gate.record(10., 4);
  {
    CameraDiagnosticsScope scope(&store, 13.0);
    scope.frame.stage = "update_complete";
    scope.frame.msckf.candidates = 8;
    scope.frame.msckf.rejected_triangulation = 8;
  }
  assert(store.json().find("\"tracked_unique_ids\":0") != std::string::npos);
  assert(store.json().find("\"rejected_triangulation\":8") != std::string::npos);
  assert(!gate.fresh(13., .5));
  UpdaterOptions options;
  ov_core::FeatureInitializerOptions initializer;
  UpdaterMSCKF updater(options, initializer);
  MsckfUpdateDiagnostics counts;
  counts.candidates = 999;
  std::vector<std::shared_ptr<ov_core::Feature>> features;
  StateOptions state_options;
  auto state = std::make_shared<State>(state_options);
  updater.update(state, features, &counts);
  assert(counts.candidates == 0 && counts.accepted_constraints == 0 && !counts.state_update_applied);
  // Real updater branch: a track with no valid clone measurements is discarded.
  features.push_back(std::make_shared<ov_core::Feature>());
  updater.update(state, features, &counts);
  assert(features.empty());
  assert(counts.candidates == 1 && counts.rejected_short_track == 1 && !counts.state_update_applied);
  // Null output keeps the ordinary updater API usable.
  updater.update(state, features);
  CameraDiagnosticsStore concurrent;
  std::thread worker([&]() {
    for (int i = 0; i < 1000; ++i) {
      CameraUpdateDiagnostics frame;
      frame.camera_stamp_s = i;
      frame.msckf.candidates = 1;
      frame.msckf.rejected_chi2 = 1;
      concurrent.record(frame);
    }
  });
  for (int i = 0; i < 1000; ++i) assert(!concurrent.json().empty());
  worker.join();
  assert(concurrent.json().find("\"camera_frames\":1000") != std::string::npos);
  std::cout << store.json() << std::endl;
}
