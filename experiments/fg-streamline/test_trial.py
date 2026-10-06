import tempfile
import unittest
from pathlib import Path
from bounded_trial import read_rows, set_policy, summarize


class TrialChecks(unittest.TestCase):
    def test_transient_off_does_not_qualify_short_trial(self):
        rows = [{'kind': 'fg_present_outcome', 'SDKResult': 0, 'status': 0,
                 'actualPresents': 2}] * 319
        rows += [{'kind': 'game_fg_mode', 'requestedMode': 0, 'SDKResult': 0,
                  'matchingInputs': False}]
        self.assertFalse(summarize(rows)['boundedGatePassed'])

    def test_complete_gate_and_input_mismatch(self):
        rows = [{'kind': 'fg_present_outcome', 'SDKResult': 0, 'status': 0,
                 'actualPresents': 2}] * 600
        rows += [{'kind': 'game_fg_mode', 'requestedMode': 0, 'SDKResult': 0,
                  'matchingInputs': True}]
        self.assertTrue(summarize(rows)['boundedGatePassed'])
        rows += [{'kind': 'game_guide_tags', 'SDKResult': 0, 'tokenResult': 0,
                  'frameStamp': 10, 'SDKIndex': 11}]
        self.assertFalse(summarize(rows)['boundedGatePassed'])

    def test_partial_final_row_only(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'private.jsonl'
            path.write_text('{"kind":"one"}\n{"kind":', encoding='utf-8')
            self.assertEqual(read_rows(path), [{'kind': 'one'}])
            path.write_text('corrupt\n', encoding='utf-8')
            with self.assertRaises(ValueError):
                read_rows(path)

    def test_off_policy_and_atomic_temporary_removal(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'FGGuideCapture.ini'
            set_policy(path, 5, False)
            self.assertIn('EnableFG=0\nTrialRevision=5', path.read_text())
            self.assertFalse(path.with_name(path.name + '.pending').exists())


if __name__ == '__main__':
    unittest.main()
