"""Build/run the actual-source Windows startup rollback regression without vendor DLLs."""
from pathlib import Path
import argparse,importlib.util,os,subprocess

def main():
 p=argparse.ArgumentParser();p.add_argument('--sdk',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 root=Path(__file__).resolve().parents[1]
 spec=importlib.util.spec_from_file_location('fg_pins',root/'experiments/providers/build_fsr_fg_probe.py');pins=importlib.util.module_from_spec(spec);spec.loader.exec_module(pins)
 for name,digest in pins.HEADERS.items():
  if pins.digest(a.sdk/name)!=digest:raise ValueError('Pinned FSR header changed: '+name)
 a.output.mkdir(parents=True,exist_ok=True);out=a.output.resolve()
 vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
 vs=Path(subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip())
 source=root/'tests/fsr_startup_failure_windows_test.cpp'
 command='cl /nologo /std:c++20 /EHsc /O1 /MT /W3 '+f'/I"{a.sdk.resolve()}" /Fo"{out / "test.obj"}" /Fe"{out / "test.exe"}" "{source}" /link d3d12.lib dxgi.lib dxguid.lib user32.lib bcrypt.lib'
 build=out/'build.cmd';build.write_text('@echo off\ncall "'+str(vs/'VC/Auxiliary/Build/vcvars64.bat')+'"\nif errorlevel 1 exit /b 1\n'+command+'\n',encoding='utf-8')
 subprocess.run(['cmd','/c',str(build)],cwd=out,check=True,timeout=90)
 subprocess.run([str(out/'test.exe')],cwd=out,check=True,timeout=30)
if __name__=='__main__':main()
