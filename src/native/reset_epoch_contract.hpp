#pragma once
// Private opt-in protocol for the qualified PR435 framework patch.
// ID3D12GraphicsCommandList private data: uint64_t, advanced ONLY after Reset
// succeeds. The addon opts in by storing zero before a relevant Reset call.
// Missing data is not reset proof; zero means armed without a successful stamp.
static constexpr GUID mcd2_reset_epoch_guid={0x15a8089f,0x7fcb,0x4f6a,{0xa4,0x03,0x90,0x7d,0x5c,0x66,0x29,0x81}};
