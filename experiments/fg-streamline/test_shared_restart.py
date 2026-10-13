from pathlib import Path
import unittest


class SharedRestartChecks(unittest.TestCase):
    def test_shared_status_keeps_permanent_retirement_restart_requirement(self):
        source = Path(__file__).with_name('fg_controls.hpp').read_text()
        shared = source.split('if(shared){', 1)[1].split('publish(active,false);return;', 1)[0]
        self.assertIn('fgMenuRestartRequired(available,intent.mode!=0,startupOwner,'
                      'intent.provider,retired.load())', shared)
        self.assertIn('!fault&&!retired', shared)
        guide = Path(__file__).with_name('guide_recon.cpp').read_text()
        self.assertIn('configured()>0)fg_controls::retired=true;', guide)
        self.assertIn('if(!activeFGMode && permanentOff && !releasedWhileOff)', guide)


if __name__ == '__main__':
    unittest.main()
