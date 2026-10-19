"""Execute actual Actor temporal restore and its EndPlay call site against fakes."""
import argparse
from pathlib import Path
import subprocess
import tempfile
from test_fg_foliage_policy import method


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--dotnet',default='dotnet');args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    source=(root/'src/ui/ModActor.cs').read_text(encoding='utf-8')
    restore=method(source,'public void RestoreTemporalQuality()')
    end=method(source,'protected override void ReceiveEndPlay(')
    # Execute the exact cleanup branch preceding unrelated UI/display teardown.
    branch=end[end.index('{')+1:end.index('  RestoreSharpness();')]
    fixture=(root/'tests/provider-menu-client/TemporalEndPlayFixture.cs.txt').read_text(encoding='utf-8')
    fixture=fixture.replace('// ACTUAL_RESTORE',restore).replace('// ACTUAL_END_BRANCH',branch)
    with tempfile.TemporaryDirectory(prefix='mcd2-temporal-endplay-') as temporary:
        folder=Path(temporary);(folder/'Program.cs').write_text(fixture,encoding='utf-8')
        (folder/'Test.csproj').write_text('<Project Sdk="Microsoft.NET.Sdk"><PropertyGroup><OutputType>Exe</OutputType><TargetFramework>net10.0</TargetFramework><Nullable>enable</Nullable><TreatWarningsAsErrors>true</TreatWarningsAsErrors></PropertyGroup></Project>')
        subprocess.run([args.dotnet,'run','--project',str(folder/'Test.csproj'),'-c','Release'],check=True)

if __name__=='__main__':main()
