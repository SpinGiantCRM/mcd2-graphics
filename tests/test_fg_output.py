from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
class OutputTests(unittest.TestCase):
 def test_sdk_context_uses_validated_actual_output(self):
  s=(ROOT/'experiments/providers/fsr_fg_game_bridge.cpp').read_text()
  output=s.split('extern "C" int mcd2_afg_output_v1(',1)[1].split('extern "C" unsigned mcd2_afg_recording_contract_v2',1)[0]
  for field in ('validFgOutput(next)','s.swap->GetDesc1(&actual)','actual.Width!=next.width','actual.Height!=next.height','unsigned(actual.Format)!=next.backbufferFormat'):self.assertIn(field,output)
  self.assertLess(output.index('completed<s.pendingOutputFence'),output.index('release(w.command)'))
  self.assertLess(output.index('release(w.command)'),output.index('s.destroy(&s.fg'))
  self.assertNotIn('s.destroy(&s.presenter',output)
  self.assertNotIn('WaitForSingleObject',output)
  create=s.split('int createGeneration(',1)[1].split('extern "C" int mcd2_afg_load_v1',1)[0]
  self.assertIn('fgOutputHdr(output)?FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE:0',create)
  self.assertIn('DXGI_FORMAT(output.sceneFormat)',create)
  self.assertIn('DXGI_FORMAT(output.backbufferFormat)',create)
  self.assertIn('p->backbufferTransferFunction!=transfer',s)
 def test_exact_original_draw_and_pass_boundary(self):
  s=(ROOT/'experiments/fg-streamline/guide_recon.cpp').read_text()
  draw=s.split('bool draw_common(',1)[1].split('void finish(',1)[0]
  self.assertIn('cmd->draw_indexed(call.elements,call.instances,call.first,call.base,call.firstInstance)',draw)
  self.assertIn('else cmd->draw(call.elements,call.instances,call.first,call.firstInstance)',draw)
  self.assertIn('return true;',draw)
  self.assertIn('state.pendingScene=UINT32_MAX;',draw)
  end=s.split('bool end_pass(',1)[1].split('int scene_view_frame(',1)[0]
  self.assertLess(end.index('list->EndRenderPass()'),end.index('snapshot_scene(cmd,stamp)'))
  self.assertIn('if(c.passStores)stamp=c.pendingScene',end)
  self.assertIn('mcd2_fg::camera(view.data()',s)
  self.assertIn('const bool sceneShader=mcd2::providers::qualifiedSdrSceneShader(state.ps);',draw)
  self.assertIn('viewResult=sceneShader?scene_view_frame(cmd,state,id):-9;',draw)
  self.assertNotIn('state.ps==0x088c6818',draw)
  identity=(ROOT/'experiments/fg-streamline/fg_scene_identity.h').read_text()
  for field in ('position','forward','up','right','viewToClip','clipToPrevious','jitter'):
   self.assertIn('memcmp(decoded.'+field+',expected.'+field,identity)
 def test_snapshot_is_owned_and_rgb_only(self):
  h=(ROOT/'experiments/fg-streamline/fg_scene_copy.h').read_text()
  shader=(ROOT/'experiments/fg-streamline/fg_scene_copy.hlsl').read_text()
  self.assertIn('float4(scene.Load(int3(p.xy,0)).rgb,1.0)',shader.replace(' ',''))
  self.assertNotIn('backbuffer->AddRef',h)
  self.assertIn('CopyResource(e.stage,backbuffer)',h)
  self.assertIn('cmd->Dispatch((width+7)/8,(height+7)/8,1)',h)
  self.assertIn('if(!queue)return false',h)
  self.assertLess(h.index('WaitForSingleObject'),h.index('entries.clear()'))
  guide=(ROOT/'experiments/fg-streamline/guide_recon.cpp').read_text()
  self.assertIn('recordings+=state.usesScene',guide)
  self.assertIn('!result&&!recordings&&fg_scene::cleanup()',guide)
  self.assertIn('GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN',guide)
 def test_build_and_install_include_scene_shader(self):
  recipe=(ROOT/'experiments/fg-streamline/build_probe.py').read_text()
  for name in ('fg_scene_copy.h','fg_scene_copy.hlsl','fg_scene_copy_probe.cpp','src/providers/fg_output_contract.hpp','FG_SCENE_COPY.cso'):self.assertIn(name,recipe)
  self.assertIn('FG_SCENE_COPY.cso',(ROOT/'experiments/fg-streamline/build_windows_candidate.py').read_text())
  self.assertIn("'probe/FG_SCENE_COPY.cso': BIN+'FG_SCENE_COPY.cso'",(ROOT/'tools/assemble_amd_candidate.py').read_text())
  self.assertIn("'output'",(ROOT/'experiments/providers/build_fsr_fg_game_bridge.py').read_text())
  self.assertIn('GetProcAddress(amdFgBridge,"mcd2_afg_output_v1")',(ROOT/'experiments/fg-streamline/bootstrap_dxgi.cpp').read_text())
 def test_reset_notification_is_not_recording_retirement(self):
  guide=(ROOT/'experiments/fg-streamline/guide_recon.cpp').read_text()
  reset=guide.split('void reset(',1)[1].split('void destroy_cmd(',1)[0]
  self.assertIn('found->second.sceneReset.request(known,epoch)',reset)
  self.assertIn('scene_epoch(cmd,epoch,true)',reset)
  self.assertIn('const bool known=scene_epoch(cmd,epoch);if(state.sceneReset.confirmed(known,epoch))state=Cmd{};',guide)
  self.assertIn('E(destroy_command_list,destroy_cmd)',guide)
  self.assertIn('!state.sceneReset.pending&&state.passKnown',guide)
  self.assertIn('src/native/reset_epoch_contract.hpp',(ROOT/'experiments/fg-streamline/build_probe.py').read_text())
 def test_amd_ui_has_no_hdr_requirement(self):
  self.assertIn('if(SelectedFgProvider()==0 && (DisplaySaved==null || DisplaySaved.HDROutput==0))',(ROOT/'src/ui/ModActor.cs').read_text())
if __name__=='__main__':unittest.main()
