"""Build an isolated Microsoft-ABI FSR bridge, without acquiring vendor files."""
from pathlib import Path
import argparse
import json
import subprocess
import importlib.util
_pin_spec = importlib.util.spec_from_file_location('_mcd2_fg_pins', Path(__file__).with_name('build_fsr_fg_probe.py'))
_pins = importlib.util.module_from_spec(_pin_spec)
_pin_spec.loader.exec_module(_pins)
COMMIT, HEADERS, digest = _pins.COMMIT, _pins.HEADERS, _pins.digest

def build(sdk, sysroot, output, compiler='clang-cl', linker='lld-link', presenter_switch_probe=False):
    for name, expected in HEADERS.items():
        if digest(sdk / name) != expected:
            raise ValueError('Pinned FSR header changed: ' + name)
    root = Path(__file__).resolve().parent
    output.mkdir(parents=True, exist_ok=True)
    obj, binary = output/'fsr-fg-game-bridge.obj', output/'mcd2-fsr-fg-game-bridge.dll'
    command = [compiler, '/nologo', '/std:c++20', '/O2', '/MT', '/EHsc', '/c',
               str(root/'fsr_fg_game_bridge.cpp'), '/Fo'+str(obj), '/I'+str(sdk),
               '/clang:-fms-compatibility-version=19.44',
               '/clang:-Wframe-larger-than=16384', '/clang:-Werror=frame-larger-than']
    for folder in ['crt/include', 'sdk/include/ucrt', 'sdk/include/shared', 'sdk/include/um']:
        command.append('/imsvc'+str(sysroot/folder))
    link = [linker, '/dll', '/out:'+str(binary), str(obj), 'kernel32.lib', 'user32.lib', 'bcrypt.lib', 'dxgi.lib', 'd3d12.lib', 'dxguid.lib']
    for name in ['implementation','load','support','swap','output','scene','state','retire','antilag_ready','antilag','presenter_switch_state']:
        link.append('/export:mcd2_afg_'+name+'_v1')
    for name in ['recording_contract','guides','world','present']:
        link.append('/export:mcd2_afg_'+name+'_v2')
    link += ['/export:mcd2_afg_diagnostic_bind_v1', '/export:mcd2_afg_diagnostics_v2']
    for folder in ['crt/lib/x86_64', 'sdk/lib/ucrt/x86_64', 'sdk/lib/um/x86_64']:
        link.append('/libpath:'+str(sysroot/folder))
    with (output/'build-private.log').open('w') as log:
        for step in [command, link]:
            subprocess.run(step, stdout=log, stderr=subprocess.STDOUT, check=True)
        probe_obj, probe_binary = output/'fsr-fg-game-bridge-probe.obj', output/'fsr-fg-game-bridge-probe.exe'
        probe_compile = [str(root/'fsr_fg_game_bridge_probe.cpp') if x == str(root/'fsr_fg_game_bridge.cpp')
                         else '/Fo'+str(probe_obj) if x.startswith('/Fo') else x for x in command]
        probe_link = [linker, '/out:'+str(probe_binary), str(probe_obj), 'kernel32.lib', 'user32.lib',
                      'dxgi.lib', 'd3d12.lib', 'dxguid.lib', *[x for x in link if x.startswith('/libpath:')]]
        for step in [probe_compile, probe_link]:
            subprocess.run(step, stdout=log, stderr=subprocess.STDOUT, check=True)
        if presenter_switch_probe:
            switch_obj = output/'presenter-switch-probe.obj'
            switch_binary = output/'presenter-switch-probe.exe'
            switch_compile = [str(root/'presenter_switch_probe.cpp') if x == str(root/'fsr_fg_game_bridge.cpp')
                              else '/Fo'+str(switch_obj) if x.startswith('/Fo') else x for x in command]
            # Exercise the 4K buffer geometry used by the current Linux control.
            switch_compile += ['/DMCD2_PROBE_WIDTH=3840', '/DMCD2_PROBE_HEIGHT=2160']
            switch_link = [linker, '/out:'+str(switch_binary), str(switch_obj), 'kernel32.lib',
                           'user32.lib', 'dxgi.lib', 'd3d12.lib', 'dxguid.lib',
                           *[x for x in link if x.startswith('/libpath:')]]
            for step in [switch_compile, switch_link]:
                subprocess.run(step, stdout=log, stderr=subprocess.STDOUT, check=True)
        if presenter_switch_probe:
            resize_obj=output/'presenter-resize-probe.obj';resize_binary=output/'presenter-resize-probe.exe'
            resize_compile=[str(root/'presenter_resize_probe.cpp') if x==str(root/'fsr_fg_game_bridge.cpp') else '/Fo'+str(resize_obj) if x.startswith('/Fo') else x for x in command]
            resize_link=[linker,'/out:'+str(resize_binary),str(resize_obj),'kernel32.lib','user32.lib','dxgi.lib','d3d12.lib','dxguid.lib',*[x for x in link if x.startswith('/libpath:')]]
            for step in [resize_compile,resize_link]:
                subprocess.run(step,stdout=log,stderr=subprocess.STDOUT,check=True)
    notice = root.parents[1]/'third-party/FSR-SDK-HEADERS-LICENSE.txt'
    (output/notice.name).write_bytes(notice.read_bytes())
    receipt = dict(sdkCommit=COMMIT, SDKheadersSHA256=HEADERS,
                   sourceSHA256={name:digest(root/name) for name in ['fsr_fg_game_bridge.cpp','fsr_fg_game_bridge.h','synchronized_swapchain.hpp','../../src/latency/process_exit.hpp','../../src/latency/process_lifetime.hpp', 'fsr_fg_game_bridge_probe.cpp', 'build_fsr_fg_probe.py', '../fg-streamline/fg_camera_contract.h', '../../src/native/reset_epoch_contract.hpp','../../src/latency/amd_fg_private_data.hpp','../../src/providers/fg_output_contract.hpp', '../../src/providers/graphics_store.hpp', '../../src/providers/graphics_record.hpp', '../../src/providers/graphics_intent.hpp', '../../src/providers/mfg_phase_plan.hpp', '../../src/providers/fg_menu_projection.hpp', '../../src/providers/bootstrap_module.hpp', '../../src/providers/menu_snapshot.h']},
                   recipeSHA256=digest(Path(__file__)), binarySHA256=digest(binary),
                   probeSHA256=digest(probe_binary),
                   runtimeBundled=False, gameIntegrationQualified=False,
                   headersLicenseSHA256=digest(notice))
    for name in ['../../src/providers/runtime_diagnostics.h','../../src/providers/fg_diagnostics_attribution.hpp']:
        receipt['sourceSHA256'][name]=digest(root/name)
    if presenter_switch_probe:
        receipt['presenterResizeProbe'] = dict(binarySHA256=digest(resize_binary),sourceSHA256=digest(root/'presenter_resize_probe.cpp'),gameIntegrationQualified=False,performanceQualified=False)
        receipt['presenterSwitchProbe'] = dict(
            binarySHA256=digest(switch_binary), width=3840, height=2160,
            sourceSHA256={name:digest(root/name) for name in
                          ['presenter_switch_probe.cpp', 'owned_presenter_switch_probe.hpp']},
            gameIntegrationQualified=False, performanceQualified=False)
    (output/'build-receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    return binary

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--windows-sysroot', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--clang-cl', default='clang-cl')
    parser.add_argument('--linker', default='lld-link')
    parser.add_argument('--presenter-switch-probe', action='store_true',
                        help='Also build the isolated presenter rebuild and actual ResizeBuffers probes')
    args = parser.parse_args()
    build(args.sdk.resolve(), args.windows_sysroot.resolve(), args.output.resolve(), args.clang_cl, args.linker,
          args.presenter_switch_probe)
    print('FSR game bridge compiled; no runtime downloaded or packaged.')
