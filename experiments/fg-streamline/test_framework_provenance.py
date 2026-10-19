import unittest
from framework_provenance import RESHADE_COMMIT, DESCRIPTOR_HEAP_PATCH, source_matches


class FrameworkProvenanceChecks(unittest.TestCase):
    def test_clean_pinned_base(self):
        self.assertTrue(source_matches({'commit': RESHADE_COMMIT, 'sourceClean': True}, None))
        self.assertFalse(source_matches({'commit': 'another commit', 'sourceClean': True}, None))
        self.assertFalse(source_matches({'commit': RESHADE_COMMIT, 'sourceClean': 'true'}, None))

    def test_only_explicit_approved_patch(self):
        receipt = {'commit': RESHADE_COMMIT, 'sourceClean': False,
                   'baseCleanBeforePatch': True, 'approvedPatchOnly': True,
                   'patchSHA256': 'approved'}
        self.assertTrue(source_matches(receipt, 'approved'))
        self.assertFalse(source_matches(receipt, 'different'))
        self.assertFalse(source_matches(receipt, None))
        for key in ['commit', 'sourceClean', 'baseCleanBeforePatch', 'approvedPatchOnly', 'patchSHA256']:
            incomplete = receipt.copy()
            del incomplete[key]
            self.assertFalse(source_matches(incomplete, 'approved'), key)
        self.assertFalse(source_matches(dict(receipt, approvedPatchOnly='true'), 'approved'))

    def test_candidate_requires_the_complete_approved_patch_set(self):
        extra = {DESCRIPTOR_HEAP_PATCH: 'heap-fix'}
        receipt = {'commit': RESHADE_COMMIT, 'sourceClean': False,
                   'baseCleanBeforePatch': True, 'approvedPatchOnly': True,
                   'patchSHA256': 'reset', 'additionalPatchSHA256': extra}
        self.assertTrue(source_matches(receipt, 'reset', extra))
        self.assertFalse(source_matches(receipt, 'reset'))
        self.assertFalse(source_matches(receipt, 'reset', {DESCRIPTOR_HEAP_PATCH: 'wrong'}))
        self.assertFalse(source_matches(dict(receipt, additionalPatchSHA256={}), 'reset', extra))
        self.assertFalse(source_matches(dict(receipt, sourceClean=True), 'reset', extra))
        self.assertFalse(source_matches(dict(receipt, additionalPatchSHA256=dict(extra, unknown='x')), 'reset', extra))


if __name__ == '__main__':
    unittest.main()
