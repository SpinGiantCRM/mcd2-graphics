"""Compile a reversible transport-only UI trial; never install or publish it."""
from pathlib import Path
import argparse, hashlib, json, re, subprocess
REPO=Path(__file__).resolve().parents[2]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def actor_overlay(source):
    anchors={
        'public class ModActor : AActor {': 'public class ModActor : AActor {\n public ProviderTransportProbe? ProviderProbe;\n public void PollProviderTransport(){if(ProviderProbe!=null)ProviderProbe.PollTransport();}',
        '  Timer.Start(this,"PollRenderer",0.25f,true);': '  Timer.Start(this,"PollRenderer",0.25f,true);\n  ProviderProbe=UGameplayStatics.CreateSaveGameObject(Unreal.ClassOf<ProviderTransportProbe>()) as ProviderTransportProbe;\n  if(ProviderProbe!=null){ProviderProbe.StartTransport();Timer.Start(this,"PollProviderTransport",0.5f,true);}',
    }
    for before,after in anchors.items():
        if source.count(before)!=1:raise ValueError('Trial overlay anchor changed; inspect the current actor before building')
        source=source.replace(before,after)
    return source

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('dotnet','neorune-sdk','pack-tools','output'):p.add_argument('--'+name,required=True,type=Path)
    a=p.parse_args();sdk=a.neorune_sdk.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    if '<version>0.1.2</version>' not in (sdk/'NeoRune.Sdk.nuspec').read_text(encoding='utf-8-sig'):raise ValueError('Expected pinned NeoRune 0.1.2')
    refs=a.dotnet.resolve().parent/'packs/Microsoft.NETCore.App.Ref/10.0.12/ref/net10.0'
    if not refs.is_dir():raise ValueError('Expected .NET 10.0.12 reference pack')
    original=REPO/'src/ui/ModActor.cs';probe=REPO/'experiments/providers/neorune_transport_probe.cs'
    actor=out/'ModActor.cs';actor.write_text(actor_overlay(original.read_text()))
    args=['build','--mod=MCD2Graphics','--out='+str(out/'ui'),'--tools='+str(a.pack_tools.resolve())]
    args+=['--source='+str(sdk/'src'/name) for name in ('Log.cs','Timer.cs','World.cs')]
    args+=['--ref='+str(path) for path in sorted(refs.glob('*.dll'))]
    args+=['--ref='+str(sdk/'ref'/name) for name in ('NeoRune.Abstractions.dll','NeoRune.Game.dll')]
    args+=['--source='+str(actor),'--source='+str(REPO/'src/ui/ProviderSaveWords.cs'),'--source='+str(REPO/'src/ui/ProviderMenuClient.cs'),'--source='+str(probe)]
    rsp=out/'ui-build.rsp';rsp.write_text('\n'.join(args)+'\n')
    result=subprocess.run([str(a.dotnet),str(sdk/'tools/neorune.dll'),'@'+str(rsp)],capture_output=True,text=True)
    (out/'ui-build-private.log').write_text(result.stdout+result.stderr)
    if result.returncode or re.search(r'\berror\s+\w+\d+:',result.stdout+result.stderr,re.I):raise RuntimeError('NeoRune trial compilation failed; inspect private build diagnostics')
    builder=REPO/'tools/ModInfoBuilder/bin/Release/net10.0/ModInfoBuilder.dll'
    subprocess.run([str(a.dotnet),'build',str(REPO/'tools/ModInfoBuilder/ModInfoBuilder.csproj'),'-p:NeoRuneSdk='+str(sdk),'-c','Release','--nologo'],check=True)
    subprocess.run([str(a.dotnet),str(builder),str(out/'ui/Assets'),str(a.pack_tools.resolve()),str(out/'ui/Pak'),str(REPO/'manifest.json')],check=True)
    receipt={'trialOnly':True,'runtimeQualified':False,'releasedActorSHA256':sha(original),
             'overlaySHA256':sha(actor),'clientSHA256':sha(probe),'builderSHA256':sha(REPO/'tools/ModInfoBuilder/Program.cs'),
             'payloadSHA256':{path.name:sha(path) for path in sorted((out/'ui/Pak').glob('MCD2Graphics_P.*'))}}
    (out/'trial-ui-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('Transport-only UI trial compiled; released actor and installer payload unchanged.')
if __name__=='__main__':main()
