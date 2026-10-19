"""Validate the exact framework source allowed in an isolated FG trial."""
RESHADE_COMMIT = '4eb9056c76016aad6f98495d3bbda2d721106104'
DESCRIPTOR_HEAP_FIX_COMMIT = '64017f98cc1393db51d9f5b1307c64350680f41c'
DESCRIPTOR_HEAP_PATCH = 'qualification/fg-release/reshade-descriptor-heap-lifetime.patch'


def source_matches(receipt, approved_patch_sha256, additional_patch_sha256=None):
    additional = additional_patch_sha256 or {}
    if receipt.get('additionalPatchSHA256', {}) != additional:
        return False
    if receipt.get('commit') != RESHADE_COMMIT:
        return False
    if receipt.get('sourceClean') is True:
        return not additional
    return (receipt.get('sourceClean') is False
            and receipt.get('baseCleanBeforePatch') is True
            and receipt.get('approvedPatchOnly') is True
            and approved_patch_sha256 is not None
            and receipt.get('patchSHA256') == approved_patch_sha256)
