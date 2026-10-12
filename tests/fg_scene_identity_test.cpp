#include "../experiments/fg-streamline/fg_scene_identity.h"
#include "../experiments/fg-streamline/fg_camera_math.h"
#include <array>
#include <cassert>
#include <limits>
int main(){
 std::array<float,662>view{};view[128]=2;view[133]=3;view[139]=1;view[142]=10;view[544]=view[549]=view[554]=view[559]=1;view[576]=.001f;view[577]=-.002f;view[288]=20;view[292]=1;view[297]=1;view[302]=1;
 MCD2FGCamera scene{},guide{};assert(mcd2_fg::camera(view.data(),123,1,1,false,scene));assert(mcd2_fg::camera(view.data(),123,2560,1440,true,guide));assert(mcd2_fg::same_scene_view(scene,guide));
 auto bad=scene;++bad.frame;assert(!mcd2_fg::same_scene_view(bad,guide));bad=scene;bad.width=2;assert(!mcd2_fg::same_scene_view(bad,guide));bad=scene;bad.height=2;assert(!mcd2_fg::same_scene_view(bad,guide));bad=scene;bad.size=0;assert(!mcd2_fg::same_scene_view(bad,guide));bad=guide;bad.width=0;assert(!mcd2_fg::same_scene_view(scene,bad));
 for(auto array:{&MCD2FGCamera::position,&MCD2FGCamera::forward,&MCD2FGCamera::up,&MCD2FGCamera::right})for(unsigned i=0;i<3;++i){bad=scene;(bad.*array)[i]+=.001f;assert(!mcd2_fg::same_scene_view(bad,guide));(bad.*array)[i]=std::numeric_limits<float>::quiet_NaN();assert(!mcd2_fg::same_scene_view(bad,guide));}
 for(auto array:{&MCD2FGCamera::viewToClip,&MCD2FGCamera::clipToPrevious})for(unsigned i=0;i<16;++i){bad=scene;(bad.*array)[i]+=.001f;assert(!mcd2_fg::same_scene_view(bad,guide));(bad.*array)[i]=std::numeric_limits<float>::infinity();assert(!mcd2_fg::same_scene_view(bad,guide));}
 for(unsigned i=0;i<2;++i){bad=scene;bad.jitter[i]+=.001f;assert(!mcd2_fg::same_scene_view(bad,guide));bad.jitter[i]=std::numeric_limits<float>::quiet_NaN();assert(!mcd2_fg::same_scene_view(bad,guide));}
}
