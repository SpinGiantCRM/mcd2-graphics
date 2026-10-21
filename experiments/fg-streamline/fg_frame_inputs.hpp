#pragma once
#include "fg_camera_contract.h"
namespace mcd2::fg {
struct FrameInputs {
 MCD2FGCamera camera{};
 bool guides=false,images=false,sdrFinalColor=false;
 void setGuides(const MCD2FGCamera& value,bool accepted){camera=value;guides=accepted;}
};
}
