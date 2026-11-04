#ifndef OV_CORE_STEREO_EPIPOLAR_GATE_H
#define OV_CORE_STEREO_EPIPOLAR_GATE_H

#include <cmath>
#include <memory>
#include <stdexcept>
#include <vector>
#include <Eigen/Dense>
#include <opencv2/core.hpp>
#include "cam/CamBase.h"

namespace ov_core {

class StereoEpipolarGate {
public:
  StereoEpipolarGate(std::shared_ptr<CamBase> left, std::shared_ptr<CamBase> right,
                     const Eigen::Matrix3d &rotation, const Eigen::Vector3d &translation)
      : leftCamera(std::move(left)), rightCamera(std::move(right)), rightFromLeft(rotation), rightTranslation(translation) {
    if (!leftCamera || !rightCamera || !rotation.allFinite() || !translation.allFinite() || translation.norm() < 1e-6 ||
        (rotation * rotation.transpose() - Eigen::Matrix3d::Identity()).norm() > 1e-4 || std::abs(rotation.determinant()-1.) > 1e-4)
      throw std::invalid_argument("stereo_geometry_check requires finite, nonzero-baseline rigid extrinsics");
    focal = std::max(rightCamera->get_K()(0,0), rightCamera->get_K()(1,1));
    if (!std::isfinite(focal) || focal <= 0)
      throw std::invalid_argument("stereo_geometry_check requires positive focal lengths");
  }

  bool accepts(const cv::Point2f &left, const cv::Point2f &right) const {
    if (!std::isfinite(left.x) || !std::isfinite(left.y) || !std::isfinite(right.x) || !std::isfinite(right.y)) return false;
    auto a = leftCamera->undistort_cv(left), b = rightCamera->undistort_cv(right);
    Eigen::Vector3d line = rightTranslation.cross(rightFromLeft * Eigen::Vector3d(a.x,a.y,1));
    double denominator = line.head<2>().norm();
    if (!line.allFinite() || denominator < 1e-15) return false;
    double error = std::abs(Eigen::Vector3d(b.x,b.y,1).dot(line)) / denominator * focal;
    return std::isfinite(error) && error <= 2.0;
  }

  size_t filter(std::vector<cv::KeyPoint> &left, std::vector<cv::KeyPoint> &right,
                std::vector<size_t> &leftIds, std::vector<size_t> &rightIds) const {
    if (left.size()!=leftIds.size() || right.size()!=rightIds.size())
      throw std::invalid_argument("stereo geometry observation/ID sizes differ");
    std::vector<bool> removeLeft(left.size(),false), removeRight(right.size(),false);
    size_t rejected = 0;
    for (size_t i=0;i<left.size();++i) for (size_t j=0;j<right.size();++j) {
      if (leftIds[i] == rightIds[j] && !accepts(left[i].pt,right[j].pt)) {
        removeLeft[i]=true;removeRight[j]=true;++rejected;
      }
    }
    for (size_t i=left.size();i-->0;) if(removeLeft[i]) {left.erase(left.begin()+i);leftIds.erase(leftIds.begin()+i);}
    for (size_t j=right.size();j-->0;) if(removeRight[j]) {right.erase(right.begin()+j);rightIds.erase(rightIds.begin()+j);}
    return rejected;
  }

private:
  std::shared_ptr<CamBase> leftCamera, rightCamera;
  Eigen::Matrix3d rightFromLeft;
  Eigen::Vector3d rightTranslation;
  double focal;
};
} // namespace ov_core
#endif
