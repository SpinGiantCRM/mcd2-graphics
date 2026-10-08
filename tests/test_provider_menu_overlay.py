from pathlib import Path
import importlib.util, unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("menu_probe",ROOT/"experiments/providers/build_menu_transport_probe.py")
probe=importlib.util.module_from_spec(spec);spec.loader.exec_module(probe)
class MenuOverlay(unittest.TestCase):
 def test_overlay_preserves_original_and_uses_actor_timer(self):
  source=(ROOT/"src/ui/ModActor.cs").read_text();modified=probe.actor_overlay(source)
  self.assertIn('Timer.Start(this,"PollProviderTransport",0.5f,true)',modified)
  self.assertIn('ProviderProbe.PollTransport()',modified)
  self.assertEqual(modified.count('public ProviderTransportProbe? ProviderProbe;'),1)
  self.assertEqual((ROOT/"src/ui/ModActor.cs").read_text(),source)
 def test_changed_or_duplicated_anchor_refused(self):
  source=(ROOT/"src/ui/ModActor.cs").read_text()
  for changed in (source.replace('public class ModActor : AActor {',''),source+source):
   with self.assertRaises(ValueError):probe.actor_overlay(changed)
if __name__=="__main__":unittest.main()
