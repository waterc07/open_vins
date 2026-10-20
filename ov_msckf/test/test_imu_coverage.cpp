#include "state/Propagator.h"
#include "state/State.h"
#include "state/StateHelper.h"
#include "update/UpdaterZeroVelocity.h"
#include "feat/FeatureDatabase.h"
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ov_msckf;
static void require(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
static ov_core::ImuData sample(double t) {
  ov_core::ImuData x;
  x.timestamp = t;
  x.wm.setZero();
  x.am << 0., 0., 9.81;
  return x;
}
static std::shared_ptr<State> make_state() {
  StateOptions options;
  auto state = std::make_shared<State>(options);
  state->_timestamp = 100.;
  return state;
}
static void unchanged(std::shared_ptr<State> state, const Eigen::VectorXd &imu, const Eigen::MatrixXd &cov) {
  require(state->_timestamp == 100., "timestamp advanced with missing IMU");
  require(state->_imu->value().isApprox(imu, 0.), "IMU state changed with missing IMU");
  require(StateHelper::get_full_covariance(state).isApprox(cov, 0.), "covariance changed with missing IMU");
  require(state->_clones_IMU.empty(), "clone created with missing IMU");
}
int main(int argc, char **argv) {
  try {
    require(argc == 2, "test case required");
    std::string name(argv[1]);
    auto state = make_state();
    auto imu = state->_imu->value();
    auto cov = StateHelper::get_full_covariance(state);
    NoiseManager noise;
    auto prop = std::make_shared<Propagator>(noise, 9.81);
    if (name == "missing_start" || name == "missing_end") {
      for (int i = (name == "missing_start" ? 1 : 0); i <= 5; ++i) prop->feed_imu(sample(100. + .02*i));
      prop->propagate_and_clone(state, name == "missing_start" ? 100.08 : 100.12);
      unchanged(state, imu, cov);
      Eigen::Matrix<double,13,1> prediction;
      Eigen::Matrix<double,12,12> prediction_cov;
      require(!prop->fast_state_propagate(state, name == "missing_start" ? 100.08 : 100.12, prediction, prediction_cov),
              "fast propagation accepted missing bounds");
      unchanged(state, imu, cov);
    } else if (name == "selection") {
      std::vector<ov_core::ImuData> data{sample(100.),sample(100.02),sample(100.04)};
      auto selected = Propagator::select_imu_readings(data,100.005,100.035);
      require(selected.size() == 3, "in-bounds interpolation failed");
      require(selected.front().timestamp == 100.005 && selected.back().timestamp == 100.035, "wrong interval bounds");
      for (double start : {100.,100.005}) {
        auto exact = Propagator::select_imu_readings(data,start,100.04);
        require(exact.size() == 3 && exact.front().timestamp == start && exact.back().timestamp == 100.04,
                "last real sample incorrectly rejected");
      }
      require(Propagator::select_imu_readings(data,99.99,100.03,false).empty(), "missing start accepted");
      require(Propagator::select_imu_readings(data,100.,100.05,false).empty(), "missing end extrapolated");
      require(Propagator::select_imu_readings({sample(100.)},100.,100.01,false).empty(), "single sample accepted");
      require(Propagator::select_imu_readings(data,100.03,100.01,false).empty(), "backward interval accepted");
      require(Propagator::select_imu_readings(data,100.,std::numeric_limits<double>::quiet_NaN(),false).empty(), "NaN interval accepted");
    } else if (name == "recovery") {
      for (int i=0;i<=5;++i) prop->feed_imu(sample(100.+.02*i));
      prop->propagate_and_clone(state,100.12);
      unchanged(state,imu,cov);
      prop->feed_imu(sample(100.12));
      prop->feed_imu(sample(100.14));
      prop->propagate_and_clone(state,100.12);
      require(state->_timestamp == 100.12 && state->_clones_IMU.count(100.12)==1, "propagation did not recover");
      require(state->_imu->value().allFinite() && StateHelper::get_full_covariance(state).allFinite(), "nonfinite recovery");
      Eigen::Matrix<double,13,1> prediction;
      Eigen::Matrix<double,12,12> prediction_cov;
      require(prop->fast_state_propagate(state,100.14,prediction,prediction_cov), "covered fast propagation rejected");
      require(state->_timestamp == 100.12 && prediction.allFinite() && prediction_cov.allFinite(), "fast propagation mutated state");
    } else if (name == "zupt") {
      UpdaterOptions options;
      auto db=std::make_shared<ov_core::FeatureDatabase>();
      UpdaterZeroVelocity zupt(options,noise,db,prop,9.81,.1,10.,.5);
      for (int i=1;i<=5;++i) zupt.feed_imu(sample(100.+.02*i));
      require(!zupt.try_update(state,100.08), "ZUPT accepted missing start");
      unchanged(state,imu,cov);
      UpdaterZeroVelocity tail_zupt(options,noise,db,prop,9.81,.1,10.,.5);
      for (int i=0;i<=5;++i) tail_zupt.feed_imu(sample(100.+.02*i));
      require(!tail_zupt.try_update(state,100.12), "ZUPT accepted missing end");
      unchanged(state,imu,cov);
    } else throw std::runtime_error("unknown case");
    std::cout << name << " PASS\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
