#include "upscaler_support.hpp"
#include <cassert>
#include "../src/providers/fsr_hardware_policy.hpp"
int main(){
 using mcd2::providers::fsrOfficialHardware;
 assert(fsrOfficialHardware(0x1002,0x744c,"AMD Radeon RX 7900 XTX")==1);
 assert(fsrOfficialHardware(0x1002,0x747e,"AMD Radeon RX 7800 XT")==1);
 assert(fsrOfficialHardware(0x1002,0x7480,"AMD Radeon RX 7600")==1);
 assert(fsrOfficialHardware(0x1002,0x7550,"AMD Radeon RX 9070 XT")==3);
 assert(fsrOfficialHardware(0x1002,0x7590,"AMD Radeon RX 9060 XT")==3);
 assert(!fsrOfficialHardware(0x10de,0x2702,"NVIDIA GeForce RTX 4080 SUPER"));
 assert(!fsrOfficialHardware(0x1002,0x744c,"AMD Radeon RX 7900M"));
 assert(!fsrOfficialHardware(0x1002,0x7480,"AMD Radeon RX 7600S"));
 assert(!fsrOfficialHardware(0x1002,0x7551,"AMD Radeon AI PRO R9700"));
 assert(!fsrOfficialHardware(0x1002,0x744c,""));
 assert(!fsrOfficialHardware(0x1002,0xffff,"AMD Radeon RX 9999"));

 // Unsupported and unidentifiable adapters must keep native source scale.
 assert(!mcd2ui::dlss_adapter_candidate(0x1002)); // AMD
 assert(!mcd2ui::dlss_adapter_candidate(0x8086)); // Intel
 assert(!mcd2ui::dlss_adapter_candidate(0x1414)); // Software adapter
 assert(!mcd2ui::dlss_adapter_candidate(0));
 assert(mcd2ui::dlss_adapter_candidate(0x10de)); // Continue to NGX capability checks.
 // Pending source acknowledgements/observations cannot last indefinitely.
 assert(!mcd2ui::source_wait_expired(0));
 assert(!mcd2ui::source_wait_expired(15000));
 assert(mcd2ui::source_wait_expired(15001));
}
