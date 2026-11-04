#include "track/StereoEpipolarGate.h"
#include "cam/CamRadtan.h"
#include "cam/CamEqui.h"
#include <cassert>
#include <limits>
using namespace ov_core;
int main() {
  for (bool fisheye : {false,true}) {
    std::shared_ptr<CamBase> left = fisheye ? std::shared_ptr<CamBase>(new CamEqui(640,480)) : std::shared_ptr<CamBase>(new CamRadtan(640,480));
    std::shared_ptr<CamBase> right = fisheye ? std::shared_ptr<CamBase>(new CamEqui(640,480)) : std::shared_ptr<CamBase>(new CamRadtan(640,480));
    Eigen::VectorXd values(8);values<<320,321,300,240,.02,-.01,.0002,-.0003;left->set_value(values);
    values<<319,322,340,238,.01,-.02,-.0003,.0002;right->set_value(values);
    Eigen::Matrix3d rotation=Eigen::AngleAxisd(.012,Eigen::Vector3d::UnitY()).toRotationMatrix();
    Eigen::Vector3d translation(-.067,.001,-.0005);StereoEpipolarGate gate(left,right,rotation,translation);
    for(double depth : {.25,1.,10.,50.}) for(double x : {-.3,0.,.3}) {
      Eigen::Vector3d point(x,.1,depth),transformed=rotation*point+translation;
      auto a=left->distort_d(point.head<2>()/point.z()),b=right->distort_d(transformed.head<2>()/transformed.z());
      cv::Point2f rawLeft(a.x(),a.y()),rawRight(b.x(),b.y());
      assert(gate.accepts(rawLeft,rawRight));
      assert(!gate.accepts(rawLeft,cv::Point2f(rawRight.x,rawRight.y+12)));
    }
    std::vector<cv::KeyPoint> a{{100,120,1},{150,120,1},{200,120,1}},b;
    std::vector<size_t> aIds{1,2,3},bIds{1,2,4};
    for(size_t i=0;i<2;++i){auto n=left->undistort_cv(a[i].pt);Eigen::Vector3d point(n.x,n.y,1);point=rotation*point+translation;auto q=right->distort_d(point.head<2>()/point.z());b.emplace_back(q.x(),q.y(),1);}
    b[1].pt.y+=12;b.emplace_back(250,120,1);
    assert(gate.filter(a,b,aIds,bIds)==1);
    assert((aIds==std::vector<size_t>{1,3}) && (bIds==std::vector<size_t>{1,4}));
    assert(a[1].pt==cv::Point2f(200,120) && b[1].pt==cv::Point2f(250,120));
    assert(!gate.accepts(cv::Point2f(std::numeric_limits<float>::quiet_NaN(),120),b[0].pt));
    bool rejected=false;try {StereoEpipolarGate invalid(left,right,rotation,Eigen::Vector3d::Zero());}catch(const std::invalid_argument&){rejected=true;}assert(rejected);
    rejected=false;try {std::vector<size_t> wrong;gate.filter(a,b,wrong,bIds);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);
  }
  // Epipolar checks constrain the line, not correspondence along that line.
  auto camera=std::make_shared<CamRadtan>(640,480);
  Eigen::VectorXd values(8);values<<320,320,300,240,0,0,0,0;camera->set_value(values);
  StereoEpipolarGate rectified(camera,camera,Eigen::Matrix3d::Identity(),Eigen::Vector3d(-.067,0,0));
  assert(rectified.accepts({320,260},{300,261.5}));
  assert(!rectified.accepts({320,260},{300,262.5}));
  assert(rectified.accepts({320,260},{400,260}));
  std::vector<cv::KeyPoint> emptyLeft,emptyRight;std::vector<size_t> emptyIdsLeft,emptyIdsRight;
  assert(rectified.filter(emptyLeft,emptyRight,emptyIdsLeft,emptyIdsRight)==0);

}
