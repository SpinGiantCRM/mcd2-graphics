# AMD Anti-Lag 2 source

The DX12 header and MIT license are unchanged copies from
[GPUOpen AntiLag2-SDK commit 390aa4a](https://github.com/GPUOpen-LibrariesAndSDKs/AntiLag2-SDK/tree/390aa4a8c8655d0ae6e90079db2c85e103a96da3).
The build verifies both hashes before compiling the Microsoft COM ABI bridge.
No AMD driver or FSR runtime is vendored.

| File | SHA-256 |
| --- | --- |
| ffx_antilag2_dx12.h | `0554c804cfb02442974b90cf1887031bfd18d41fba2054a2cd23c8ee65e6265f` |
| LICENSE.txt | `020463d3b92d6d901c9b9d93a432359529be76883bb48f76eaeeecaea2f15c71` |

The optional build-test `amdxc64.dll` is a synthetic interface fixture under
`tests/`, not an AMD driver. Never install it in a game or include it in an
artifact/release. The Windows artifact copies only the project's bridge,
provenance receipt and MIT notice from that build directory.
