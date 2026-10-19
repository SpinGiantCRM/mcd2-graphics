"""Integration boundaries around the executable managed menu-client fixtures."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SrMenuControls(unittest.TestCase):
    def test_native_edits_do_not_select_an_upscaler(self):
        actor = (ROOT / 'src/ui/ModActor.cs').read_text()
        preset = actor.split('public void ChangePreset(', 1)[1].split('public void ChangeScale(', 1)[0]
        scale = actor.split('public void ChangeScale(', 1)[1].split('public void ResetOwned(', 1)[0]
        self.assertIn('SelectMenuQuality(family,quality)', preset)
        self.assertIn('SelectMenuCustomScale(family,ClampScale(scale))', scale)
        self.assertNotIn('SelectMenuSr', preset + scale)
        mode = actor.split('public void ChangeMode(', 1)[1].split('public void UpdateScale(', 1)[0]
        self.assertIn('SrChoiceAvailable(next)', mode)
        self.assertLess(mode.index('ShareSrPreferences(family)'), mode.index('SelectMenuSr(next)'))
        self.assertIn('a.W20==1 && SrChoiceAvailable(1)?', actor)

    def test_vendor_query_is_cached_at_device_initialization(self):
        source = (ROOT / 'src/latency/latency_addon.cpp').read_text()
        init = source.split('void init(a::device*device){', 1)[1].split('void present(', 1)[0]
        self.assertLess(init.index('process_exit::terminating()'), init.index('device->get_api()'))
        self.assertLess(init.index('renderingVendorDevice!=nativeIdentity'), init.index('CreateDXGIFactory1'))
        self.assertIn('EnumAdapterByLuid(native->GetAdapterLuid()', init)
        self.assertIn('DXGI_ADAPTER_FLAG_SOFTWARE', init)
        self.assertLess(init.index('renderingVendorId=vendor'), init.index('if(installed||registry'))
        worker = source.split('void settings_loop(){', 1)[1].split('void apply_hdr_config(){', 1)[0]
        self.assertIn('state["RenderingVendorId"]=renderingVendorId.load()', worker)
        self.assertNotIn('CreateDXGIFactory1', worker)
        self.assertNotIn('EnumAdapter', worker)


if __name__ == '__main__':
    unittest.main()
