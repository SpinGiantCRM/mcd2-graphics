"""Compile a reversible transport-only UI trial; never install or publish it."""
from pathlib import Path
import argparse, hashlib, json, sys
REPO=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(REPO))
from build_ui import build_ui
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
    original=REPO/'src/ui/ModActor.cs';probe=REPO/'experiments/providers/neorune_transport_probe.cs'
    actor=out/'ModActor.cs';actor.write_text(actor_overlay(original.read_text()))
    build_ui(a.dotnet,sdk,a.pack_tools,out/'ui',actor=actor,extra_sources=(probe,))
    receipt={'trialOnly':True,'runtimeQualified':False,'releasedActorSHA256':sha(original),
             'overlaySHA256':sha(actor),'clientSHA256':sha(probe),'builderSHA256':sha(REPO/'build_ui.py'),
             'payloadSHA256':{path.name:sha(path) for path in sorted((out/'ui/Pak').glob('MCD2Graphics_P.*'))}}
    (out/'trial-ui-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('UI candidate compiled; no game installation or published installer changed.')
if __name__=='__main__':main()
