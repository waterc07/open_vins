#include "bounded_camera_queue.h"
#include <cassert>
struct Sample { double timestamp; bool operator<(const Sample &b) const { return timestamp < b.timestamp; } };
int main() {
 std::deque<Sample> q;
 for (int i=0;i<10000;++i) { q.push_back({i/30.}); ov_msckf::trim_camera_queue(q,10,.5); assert(q.size()<=10); assert(q.back().timestamp-q.front().timestamp<=.5); }
 q={{2.0},{1.0},{1.8}}; assert(ov_msckf::trim_camera_queue(q,10,.5)==1); assert(q.front().timestamp==1.8);
 q={{1.0},{1.1},{1.2}}; assert(ov_msckf::trim_camera_queue(q,10,.5)==0); assert(q.size()==3);
 q.clear(); assert(ov_msckf::trim_camera_queue(q,10,.5)==0);
}
