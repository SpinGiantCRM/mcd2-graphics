#pragma once
extern "C" {
void mcd2_resource_from_view(reshade::api::device*,uint64_t,reshade::api::resource*);
void mcd2_view_desc(reshade::api::device*,uint64_t,reshade::api::resource_view_desc*);
void mcd2_resource_desc(reshade::api::device*,uint64_t,reshade::api::resource_desc*);
void mcd2_back_buffer(reshade::api::swapchain*,uint32_t,reshade::api::resource*);
void mcd2_reshade_bridge_layout(uint32_t*);
}
