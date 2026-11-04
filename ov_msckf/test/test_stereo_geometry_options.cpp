#include "core/VioManager.h"
#include "utils/opencv_yaml_parse.h"
#include <cassert>
#include <stdexcept>
int main(int argc,char **argv) {
  assert(argc==2);
  ov_msckf::VioManagerOptions options;
  auto parser=std::make_shared<ov_core::YamlParser>(argv[1]);options.print_and_load(parser);
  assert(!options.stereoGeometryCheck);
  options.state_options.do_calib_camera_pose=false;
  options.stereoGeometryCheck=true;
  for(int i=0;i<4;++i) {
    auto invalid=options;
    if(i==0)invalid.use_stereo=false;
    if(i==1)invalid.use_klt=false;
    if(i==2)invalid.state_options.num_cameras=1;
    if(i==3)invalid.state_options.do_calib_camera_pose=true;
    bool rejected=false;try{ov_msckf::VioManager instance(invalid);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);
  }
  ov_msckf::VioManager instance(options);assert(instance.get_params().stereoGeometryCheck);assert(!instance.state_initialized());
}
