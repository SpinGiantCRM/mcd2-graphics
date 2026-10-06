"""Validate the exact framework source allowed in an isolated FG trial."""
RESHADE_COMMIT = '4eb9056c76016aad6f98495d3bbda2d721106104'


def source_matches(receipt, approved_patch_sha256):
    if receipt.get('commit') != RESHADE_COMMIT:
        return False
    if receipt.get('sourceClean') is True:
        return True
    return (receipt.get('sourceClean') is False
            and receipt.get('baseCleanBeforePatch') is True
            and receipt.get('approvedPatchOnly') is True
            and approved_patch_sha256 is not None
            and receipt.get('patchSHA256') == approved_patch_sha256)
