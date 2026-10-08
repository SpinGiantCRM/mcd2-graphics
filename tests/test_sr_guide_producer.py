from pathlib import Path
import hashlib
import unittest

ROOT = Path(__file__).resolve().parents[1]

class GuideProducerTests(unittest.TestCase):
    def test_initial_extraction_preserves_the_conversion_body(self):
        text = (ROOT / "src/native/sr_guide_producer.hpp").read_text()
        start = text.index(" if(!c.current_signature){")
        end = text.index(" return nullptr;", start)
        self.assertEqual(hashlib.sha256(text[start:end].encode()).hexdigest(),
                         "6ede60ff457485619d9bb197051a48aa1cbdcb915425c6095a2fd24d053c4e2e")
        self.assertNotIn("NVSDK_NGX", text)
        self.assertNotIn("live_fixture", text)
        self.assertIn("cache.record(cmd,key,frame)", text)
        self.assertIn("entry.resources={gc,gd,gv,ge,go", text)

    def test_each_consumer_uses_its_own_generation_and_lease_cache(self):
        lean = (ROOT / "src/native/ngx_lean.hpp").read_text()
        probe = (ROOT / "src/native/sr_guide_probe.hpp").read_text()
        self.assertIn("record_sr_guides(c,lean.borrow,cmd,proxy,saved,guide_input)", lean)
        self.assertIn("struct GuideProbe : SrGuideGeneration", probe)
        self.assertIn("record_sr_guides(c,c.borrows,cmd,proxy,saved,input)", probe)
        self.assertIn("cache.recordings.empty() && cache.entries.empty() && !cache.blocked", probe)
        self.assertIn("cache.confirm_reset(cmd,epoch)", probe)
        self.assertIn("completed==UINT64_MAX", probe)
        self.assertIn("sample.submitted", probe)
        self.assertIn("if constexpr(!developer_controls)return;", probe)
        self.assertNotIn("NVSDK_NGX", probe)
        self.assertNotIn("ffxDispatch", probe)

    def test_developer_gate_cannot_be_a_full_release_build(self):
        recipe = (ROOT / "build.py").read_text()
        self.assertIn("if a.developer_controls and not a.native_only:p.error", recipe)
        self.assertIn("('1' if a.developer_controls else '0')", recipe)

    def test_fg_generator_tracks_the_extracted_conversion(self):
        lean = (ROOT / 'src/native/ngx_lean.hpp').read_text()
        recipe = (ROOT / 'experiments/fg-streamline/build_sr_candidate.py').read_text()
        self.assertEqual(lean.count(' auto resource=mcd2_ngx_set_resource;'), 1)
        self.assertIn("needle=' auto resource=mcd2_ngx_set_resource;'", recipe)
        self.assertIn("*(['--developer-controls'] if a.guide_inspection else [])", recipe)
        self.assertNotIn("build_text.replace('-DMCD2_ENABLE_DIAGNOSTICS=0'", recipe)

if __name__ == "__main__":
    unittest.main()
