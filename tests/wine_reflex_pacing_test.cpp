#include "../experiments/fg-streamline/wine_reflex_pacing.hpp"
#include <cassert>
int main() {
 using mcd2::compat::reflex_disabled_extensions;
 const auto wanted=L"VK_KHR_swapchain_maintenance1,VK_EXT_swapchain_maintenance1";
 assert(reflex_disabled_extensions(L"")==wanted);
 assert(reflex_disabled_extensions(wanted)==wanted);
 assert(reflex_disabled_extensions(L"VK_KHR_present_wait")==std::wstring(L"VK_KHR_present_wait,")+wanted);
 const auto separated=L"VK_EXT_swapchain_maintenance1;VK_KHR_swapchain_maintenance1";
 assert(reflex_disabled_extensions(separated)==separated);
 assert(reflex_disabled_extensions(L"VK_KHR_present_wait;")==std::wstring(L"VK_KHR_present_wait;")+wanted);
 assert(reflex_disabled_extensions(L"VK_KHR_swapchain_maintenance10")==std::wstring(L"VK_KHR_swapchain_maintenance10,")+wanted);
}
