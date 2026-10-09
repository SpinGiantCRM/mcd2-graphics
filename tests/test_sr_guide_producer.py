from pathlib import Path
import hashlib
import unittest

ROOT = Path(__file__).resolve().parents[1]

class GuideProducerTests(unittest.TestCase):
    def test_default_conversion_body_is_preserved_with_explicit_asset_selection(self):
        text = (ROOT / "src/native/sr_guide_producer.hpp").read_text()
        self.assertIn('const char *converter_asset="live_dense.cso"', text)
        # Only the selected converter filename differs from the extracted
        # body. The default NVIDIA/probe dispatch and resources are unchanged.
        self.assertEqual(text.count('asset_root/input.converter_asset'), 1)
        text = text.replace('asset_root/input.converter_asset', 'asset_root/"live_dense.cso"')
        start = text.index(" if(!c.current_signature){")
        end = text.index(" return nullptr;", start)
        self.assertEqual(hashlib.sha256(text[start:end].encode()).hexdigest(),
                         "6ede60ff457485619d9bb197051a48aa1cbdcb915425c6095a2fd24d053c4e2e")
        self.assertNotIn("NVSDK_NGX", text)
        self.assertNotIn("live_fixture", text)
        self.assertIn("cache.record(cmd,key,frame)", text)
        self.assertIn("entry.resources={gc,gd,gv,ge,go", text)

    def test_fsr_converter_uses_active_rect_without_changing_default_size(self):
        shader = (ROOT / 'src/shaders/live_dense.hlsl').read_text()
        self.assertIn('uint2 size=uint2(passData[9].xy)', shader)
        self.assertIn('#ifdef MCD2_ACTIVE_VIEW_RECT', shader)
        self.assertIn('size=rect.zw+1-rect.xy', shader)
        variant = (ROOT / 'src/shaders/fsr_dense.hlsl').read_text()
        self.assertIn('#define MCD2_ACTIVE_VIEW_RECT 1', variant)
        self.assertIn('#include "live_dense.hlsl"', variant)

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

    def test_fsr_fg_uses_fsr_guides_before_its_sdk_evaluation(self):
        fsr = (ROOT / 'src/native/fsr_runtime.hpp').read_text()
        recipe = (ROOT / 'experiments/fg-streamline/build_sr_candidate.py').read_text()
        self.assertEqual(fsr.count(' auto *sdk_colour=gc;'), 1)
        hook = recipe.split("fsr_extra='''", 1)[1].split("'''", 1)[0]
        self.assertIn('enabled && enabled()', hook)
        self.assertIn('state.evaluations==0||nativeReset||c.gap||c.force_reset', hook)
        self.assertIn('notify(proxy,c.current_depth,c.motion,&camera)', hook)
        self.assertNotIn('NVSDK_NGX', hook)
        self.assertNotIn('lean.', hook)
        self.assertIn('fsr_text.replace(fsr_needle,fsr_extra+fsr_needle)', recipe)
        self.assertIn("'--fsr-bridge-sha256',a.fsr_bridge_sha256", recipe)
        self.assertIn("'fsrGeneratedSHA256'", recipe)

if __name__ == "__main__":
    unittest.main()
