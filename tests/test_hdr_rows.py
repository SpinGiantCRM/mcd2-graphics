"""Execute the production HDR fallback with the .NET collection semantics."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class HdrRows(unittest.TestCase):
 def test_missing_native_anchor_keeps_hdr_reachable(self):
  source = (ROOT / "src/ui/ModActor.cs").read_text(encoding="utf-8")
  start = source.index("  if(!hdrInserted){int anchor=result.IndexOf(Model);")
  end = source.index("  bool changed=list.Count!=result.Count;", start)
  fallback = source[start:end]
  fixture = r'''
using System;
using System.Collections.Generic;
using System.Linq;
class Row { public int ControlId; public Row(int id){ControlId=id;} }
class Program {
 static List<object> Apply(List<object> result,object Model,bool hdrInserted,List<Row> DisplayModels){
FALLBACK
  return result;
 }
 static void Check(bool ok){if(!ok)throw new Exception("HDR row placement failed");}
 static void Main(){
  var model=new object();var before=new object();var after=new object();
  var hdr=Enumerable.Range(0,8).Select(id=>new Row(id)).ToList();
  var missing=Apply(new(){before,model,after},model,false,hdr);
  Check(missing.SequenceEqual(new object[]{before,model,hdr[0],hdr[1],hdr[2],hdr[3],after}));
  var native=new List<object>{before,hdr[0],hdr[1],hdr[2],hdr[3],model,after};
  Check(Apply(new(native),model,true,hdr).SequenceEqual(native));
  Check(Apply(new(){before,after},model,false,hdr).SequenceEqual(new[]{before,after}));
  Check(Apply(new(){model},model,false,new()).SequenceEqual(new[]{model}));
  Console.WriteLine("HDR fallback: missing native anchor, native placement, missing owned anchor and empty models passed");
 }
}
'''.replace("FALLBACK", fallback)
  with tempfile.TemporaryDirectory(prefix="mcd2-hdr-rows-") as directory:
   path=Path(directory)
   (path/"Program.cs").write_text(fixture)
   (path/"Fixture.csproj").write_text('<Project Sdk="Microsoft.NET.Sdk"><PropertyGroup><OutputType>Exe</OutputType><TargetFramework>net10.0</TargetFramework></PropertyGroup></Project>')
   result=subprocess.run(["dotnet","run","--project",str(path/"Fixture.csproj"),"-c","Release","--verbosity","quiet"],capture_output=True,text=True,timeout=120)
   self.assertEqual(result.returncode,0,result.stdout+result.stderr)

if __name__=="__main__":unittest.main()