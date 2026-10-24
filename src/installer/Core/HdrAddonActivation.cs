using System.Text;
using System.Text.RegularExpressions;

namespace MCD2.Installer;
public sealed partial class InstallerEngine {
    const string HdrActivationToken="RenoDX@renodx-ue-extended.addon64";
    sealed record ActivationKey(bool Present,List<string> Entries);
    sealed record ActivationReceipt(uint Phase,ActivationKey Original,ActivationKey Last);
    static bool SameActivationKey(ActivationKey a,ActivationKey b)=>a.Present==b.Present&&a.Entries.SequenceEqual(b.Entries);
    static bool ActivationDisabled(ActivationKey key)=>key.Entries.Any(s=>s.Contains('@')?s[(s.IndexOf('@')+1)..]=="renodx-ue-extended.addon64":s=="RenoDX");
    static bool ValidActivationKey(ActivationKey key)=>!(!key.Present&&key.Entries.Count!=0)&&key.Entries.Count<=128&&key.Entries.Sum(s=>Encoding.UTF8.GetByteCount(s))<=12000&&key.Entries.All(s=>Encoding.UTF8.GetByteCount(s)<=1024&&!s.Any(c=>c<32||c==127));
    static InvalidDataException ActivationConflict()=>new("RenoDX activation ownership could not be verified. No configuration or mod files were removed. Retain HdrAddonActivation.bin and resolve the changed ReShade.ini DisabledAddons entry before retrying; other addons must remain unchanged.");
    static ActivationReceipt DecodeActivationReceipt(byte[] data) {
        if(data.Length<32||data.Length>32768||Encoding.ASCII.GetString(data,0,8)!="MCD2HDR1")throw ActivationConflict();
        uint hash=2166136261;foreach(byte value in data.AsSpan(0,data.Length-4))hash=unchecked((hash^value)*16777619);
        if(System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(data.AsSpan(data.Length-4))!=hash)throw ActivationConflict();
        try {
            using var stream=new MemoryStream(data);using var reader=new BinaryReader(stream,new UTF8Encoding(false,true));stream.Position=8;uint phase=reader.ReadUInt32();
            ActivationKey ReadKey(){uint present=reader.ReadUInt32(),count=reader.ReadUInt32();if(present>1||count>128)throw ActivationConflict();var entries=new List<string>();for(uint i=0;i<count;i++){uint length=reader.ReadUInt32();if(length>1024||stream.Position+length>data.Length-4)throw ActivationConflict();entries.Add(new UTF8Encoding(false,true).GetString(reader.ReadBytes((int)length)));}return new(present!=0,entries);}
            var original=ReadKey();var last=ReadKey();var expected=new ActivationKey(true,[..original.Entries,HdrActivationToken]);
            if(stream.Position!=data.Length-4||phase<1||phase>4||!ValidActivationKey(original)||!ValidActivationKey(last)||ActivationDisabled(original)||!SameActivationKey(last,expected))throw ActivationConflict();
            return new(phase,original,last);
        } catch(Exception e) when(e is EndOfStreamException or DecoderFallbackException or ArgumentException){throw ActivationConflict();}
    }
    static List<string> ActivationValues(string value){
        if(value.Length==0)return [];var entries=new List<string>();var entry=new StringBuilder();
        for(int i=0;i<value.Length;i++){if(value[i]==','){if(i+1<value.Length&&value[i+1]==','){entry.Append(',');i++;}else{entries.Add(entry.ToString());entry.Clear();}}else entry.Append(value[i]);}entries.Add(entry.ToString());return entries;
    }
    (string? Config,byte[]? Data,string? Lease) PrepareHdrActivationRemoval(){
        var root=GameRoot??throw new InvalidDataException("Choose the game first.");
        var lease=GameTarget(root,"Dungeons/Binaries/Win64/MCD2Graphics/HdrAddonActivation.bin");if(!File.Exists(lease))return(null,null,null);
        if(new FileInfo(lease).Length>32768)throw ActivationConflict();var receipt=DecodeActivationReceipt(File.ReadAllBytes(lease));if(receipt.Phase==4)throw ActivationConflict();
        var config=GameTarget(root,"Dungeons/Binaries/Win64/ReShade.ini");if(File.Exists(config)&&new FileInfo(config).Length>1024*1024)throw ActivationConflict();
        var bytes=File.Exists(config)?File.ReadAllBytes(config):[];bool bom=bytes.AsSpan().StartsWith(new byte[]{0xef,0xbb,0xbf});string text;
        try{text=new UTF8Encoding(false,true).GetString(bytes.AsSpan(bom?3:0));}catch(DecoderFallbackException){throw ActivationConflict();}
        var lines=Regex.Matches(text,@"[^\r\n]*(?:\r\n|\n|\r|$)").Select(m=>m.Value).Where(s=>s.Length!=0).ToList();
        string section="";int keyIndex=-1;var current=new ActivationKey(false,[]);
        for(int i=0;i<lines.Count;i++){var line=lines[i].Trim();if(line.Length==0||line[0] is ';' or '#' or '/')continue;
            if(line[0]=='['){int end=line.IndexOf(']');if(end<0)throw ActivationConflict();section=line[1..end].Trim();continue;}
            int eq=line.IndexOf('=');string key=(eq<0?line:line[..eq]).Trim(),value=eq<0?"":line[(eq+1)..].Trim();
            if(section=="INSTALL"&&key=="BasePath"&&value.Length!=0)throw ActivationConflict();
            if(section!="ADDON"||key!="DisabledAddons")continue;if(keyIndex>=0)throw ActivationConflict();keyIndex=i;current=new(true,ActivationValues(value));
        }
        if(SameActivationKey(current,receipt.Original))return(null,null,lease); // WAL never reached disk, or restoration already saved.
        if(!SameActivationKey(current,receipt.Last)||keyIndex<0)throw ActivationConflict();
        if(receipt.Original.Present){string ending=lines[keyIndex].EndsWith("\r\n")?"\r\n":lines[keyIndex].EndsWith('\n')?"\n":lines[keyIndex].EndsWith('\r')?"\r":"";lines[keyIndex]="DisabledAddons="+string.Join(',',receipt.Original.Entries.Select(s=>s.Replace(",",",,")))+ending;}
        else lines.RemoveAt(keyIndex);
        var output=new UTF8Encoding(bom).GetPreamble().Concat(Encoding.UTF8.GetBytes(string.Concat(lines))).ToArray();return(config,output,lease);
    }
}
