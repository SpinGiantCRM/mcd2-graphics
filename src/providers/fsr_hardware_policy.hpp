#pragma once
#include <cstdint>
#include <string_view>
namespace mcd2::providers {
// Conservative consumer desktop families from AMD's public device_info table.
// These are hardware eligibility bits, not proof of driver/model activation.
// https://github.com/GPUOpen-Tools/device_info/blob/master/device_info.cpp
// AMD FSR SDK 2.3: official SR on RX 7000/9000; ML FG on RX 9000.
inline unsigned fsrOfficialHardware(std::uint32_t vendor,std::uint32_t device,std::string_view name){
 if(vendor!=0x1002||name.empty())return 0;
 if(name.find("Radeon RX ")==std::string_view::npos)return 0;
 if(name.back()=='M'||name.back()=='S'||name.find("M XT")!=std::string_view::npos)return 0;
 if((device==0x744c||device==0x747e||device==0x7480)&&name.find("RX 7")!=std::string_view::npos)return 1;
 if((device==0x7550||device==0x7590)&&name.find("RX 9")!=std::string_view::npos)return 3;
 return 0;
}
}
