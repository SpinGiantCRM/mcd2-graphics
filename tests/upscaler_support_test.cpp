#include "upscaler_support.hpp"
#include <cassert>
int main(){
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
