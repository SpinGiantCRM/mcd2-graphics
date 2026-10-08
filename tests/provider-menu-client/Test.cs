global using Timer = NeoRune.Timer;
using System.Reflection;
using System.Buffers.Binary;
using UE.Engine;
static class Test {
 static void Require(bool ok){if(!ok)throw new Exception("Menu transport assertion failed");}
 static int Crc(int[] words,int first,int count){
  uint crc=uint.MaxValue;
  for(int n=0;n<count;n++){
   uint word=n==4?0:unchecked((uint)words[first+n]);
   for(int b=0;b<4;b++){
    crc^=(word>>(b*8))&255;
    for(int bit=0;bit<8;bit++)crc=(crc>>1)^((crc&1)!=0?0xedb88320u:0u);
   }
  }
  return unchecked((int)~crc);
 }
 static void Set(ProviderAuthoritySave a,int[] words){
  for(int n=0;n<words.Length;n++)typeof(ProviderAuthoritySave).GetField("W"+n)!.SetValue(a,words[n]);
 }
 static int[] Words(ProviderRequestSave r)=>Enumerable.Range(0,42).Select(r.Word).ToArray();
 static void Main(string[] args){
  var random=new Random(123);var r=new ProviderRequestSave();var a=new ProviderAuthoritySave();
  for(int trial=0;trial<1000;trial++){
   int[] rw=Enumerable.Range(0,42).Select(_=>random.Next(int.MinValue,int.MaxValue)).ToArray();
   int[] aw=Enumerable.Range(0,44).Select(_=>random.Next(int.MinValue,int.MaxValue)).ToArray();
   for(int n=0;n<42;n++)r.SetWord(n,rw[n]);Set(a,aw);
   Require(r.Checksum(0,42)==Crc(rw,0,42));Require(r.Checksum(10,32)==Crc(rw,10,32));
   Require(a.Checksum(0,44)==Crc(aw,0,44));Require(a.Checksum(12,32)==Crc(aw,12,32));
  }
  // Canonical native authority and complete client no-op transaction.
  int[] w=new int[44];w[0]=0x3244434d;w[1]=0x00315341;w[2]=1;w[3]=176;w[5]=123;w[11]=2;
  w[12]=0x3244434d;w[13]=0x00354947;w[14]=1;w[15]=128;w[18]=5;w[19]=7;
  for(int n=21;n<=27;n+=3){w[n]=1;w[n+1]=6700;w[n+2]=7700;}
  w[30]=1;w[34]=2;w[36]=1;w[38]=1000;w[39]=203;w[40]=203;
  w[16]=Crc(w,12,32);w[4]=Crc(w,0,44);Set(a,w);
  UGameplayStatics.Slots["MCD2GraphicsProviderAuthority"]=a;
  var probe=new ProviderTransportProbe();probe.PollTransport();
  Require(UGameplayStatics.SaveCount==1&&probe.SentSession==123&&probe.SentSequence==1);
  var sent=(ProviderRequestSave)UGameplayStatics.Slots["MCD2GraphicsProviderRequest"];
  var request=Words(sent);Require(request[0]==0x3244434d&&request[1]==0x00315152&&request[3]==168);
  Require(request[5]==123&&request[6]==1&&request[7]==7&&request[8]==w[16]&&request[17]==8);
  Require(request[4]==Crc(request,0,42)&&request[14]==Crc(request,10,32));
  for(int n=0;n<32;n++)if(n!=4&&n!=7)Require(request[10+n]==w[12+n]);
  probe.PollTransport();Require(UGameplayStatics.SaveCount==1); // No repeat while waiting.
  w[6]=1;w[7]=10;w[4]=Crc(w,0,44);Set(a,w);probe.PollTransport();Require(probe.LoggedSequence==1);
  w[5]=124;w[6]=0;w[7]=0;w[4]=Crc(w,0,44);Set(a,w);probe.PollTransport();Require(UGameplayStatics.SaveCount==2&&probe.SentSession==124);
  w[4]^=1;Set(a,w);probe.SentSession=0;probe.PollTransport();Require(UGameplayStatics.SaveCount==2);
  ControlsTest.Run();
  if(args.Length==1){byte[] fixture=new byte[168];for(int n=0;n<42;n++)BinaryPrimitives.WriteInt32LittleEndian(fixture.AsSpan(n*4,4),ControlsTest.Fixture![n]);File.WriteAllBytes(args[0],fixture);}
  Console.WriteLine("NeoRune menu client: 4,000 CRC comparisons, signed word preservation, no-op envelope, session/ACK/corruption checks pass");
 }
}
// Test-only substitutes. They exercise the exact client source, not UE serialization.
namespace UE.CoreUObject { public class UObject{} }
namespace UE.Engine {
 public class USaveGame:UE.CoreUObject.UObject{}
 public static class UGameplayStatics {
  public static Dictionary<string,USaveGame> Slots=new();public static int SaveCount;public static bool FailSave;
  public static USaveGame? LoadGameFromSlot(string name,int user)=>Slots.GetValueOrDefault(name);
  public static USaveGame? CreateSaveGameObject(Type type)=>Activator.CreateInstance(type) as USaveGame;
  public static bool SaveGameToSlot(USaveGame save,string name,int user){if(FailSave)return false;Slots[name]=save;SaveCount++;return true;}
 }
}
namespace NeoRune {
 public static class Unreal {public static Type ClassOf<T>()=>typeof(T);}
 public static class Timer {public static void Start(object target,string method,float seconds,bool loop){} }
 public static class Log {public static void Write(string message){} }
}
