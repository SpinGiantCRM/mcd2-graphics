from pathlib import Path
import unittest


class SharedRestartChecks(unittest.TestCase):
    def test_completed_amd_off_handoff_only_clears_the_restart_status(self):
        source = Path(__file__).with_name('fg_controls.hpp').read_text()
        shared = source.split('if(shared){\n  auto owner=', 1)[1].split('publish(active,false);return;', 1)[0]
        self.assertIn('fgMenuNativeOffRecovered(currentRequest&&intent.session==session,available,intent.mode!=0,', shared)
        self.assertIn('intent.provider,presenterRoute,presenterCanSwitch,fault.load(),retired.load(),selectedStrategy))restartRequired=0;', shared)
        self.assertLess(shared.index('fgMenuRestartRequired('), shared.index('fgMenuNativeOffRecovered('))
        self.assertIn('wanted=admittedFrameMode&&currentRequest&&', shared)
        self.assertLess(shared.index('fgMenuNativeOffRecovered('), shared.index('wanted='))

    def test_manual_order_reaches_v3_and_exact_source_validation(self):
        controls = Path(__file__).with_name('fg_controls.hpp').read_text()
        bootstrap = Path(__file__).with_name('bootstrap_dxgi.cpp').read_text()
        for source in (controls, bootstrap):
            self.assertIn('mcd2_mfg_configure_v3', source)
            self.assertIn('int(*)(unsigned,unsigned,unsigned,unsigned,unsigned)', source)
            self.assertNotIn('mcd2_mfg_configure_v2', source)
        self.assertIn('startupFrameOrder=projection.frameOrder', bootstrap)
        self.assertIn('startupDlssGenerated,startupFsrGenerated,startupFrameOrder)', bootstrap)
        self.assertIn('||!caps||!configure?-1:', bootstrap)
        self.assertIn('selectedFrameOrder=currentRequest?selection.frameOrder:0', controls)
        self.assertIn('selectedOn?selection.fsrGenerated:0,selectedOn?selection.frameOrder:0):-1', controls)
        self.assertIn('const auto configured=!currentRequest?0:configure?configure(', controls)
        self.assertIn('if(configured){wanted=false;fault=1;}', controls)
        self.assertIn('selectedStrategy,selectedDlssGenerated,selectedFsrGenerated,selectedFrameOrder)', controls)

    def test_shared_status_keeps_permanent_retirement_restart_requirement(self):
        source = Path(__file__).with_name('fg_controls.hpp').read_text()
        shared = source.split('if(shared){\n  auto owner=', 1)[1].split('publish(active,false);return;', 1)[0]
        self.assertIn('fgMenuRestartRequired(available,intent.mode!=0,startupOwner,'
                      'intent.provider,retired.load(),selectedStrategy)', shared)
        self.assertIn('!fault&&!retired', shared)
        guide = Path(__file__).with_name('guide_recon.cpp').read_text()
        self.assertIn('configured()>0)fg_controls::retired=true;', guide)
        self.assertIn('if(!activeFGMode && permanentOff && !releasedWhileOff)', guide)


if __name__ == '__main__':
    unittest.main()
