#pragma once
#include "sr_runtime_protocol.hpp"
#include "fg_diagnostics_attribution.hpp"
namespace mcd2::diagnostics {
inline MCD2DiagnosticIdentityV1 contextIdentity(unsigned session,const providers::DecodedGraphicsRecord& authority,const providers::SrContext& context,std::uint64_t observed,std::uint64_t now){
 if(!session||context.session!=session||context.intent!=authority.bootstrap.stamp||!providers::validContext(context)||!context.ready||!observed||observed>now||now-observed>3000)return {};
 return {session,context.intent.revision,context.intent.checksum,context.world};
}
struct Projection {unsigned world=0;MCD2SrDiagnosticsV1 sr{};MCD2FgDiagnosticsV2 fg{};unsigned amdMode=0;std::uint64_t amdFrames=0;};
inline Projection project(MCD2DiagnosticIdentityV1 identity,MCD2SrDiagnosticsV1 sr,MCD2FgDiagnosticsV2 fg,unsigned amdRevision,unsigned amdChecksum,unsigned amdMode,std::uint64_t amdFrames,std::uint64_t now){
 Projection out;out.sr=sr;out.fg=fg;
 if(!valid(identity)||sr.size!=sizeof(sr)||!sr.provider||sr.provider>2||sr.revision!=identity.revision||sr.world!=identity.world){out.sr.provider=0;out.sr.evaluations=0;}
 if(!valid(identity)||fg.size!=sizeof(fg)||fg.reserved||!fg.provider||fg.provider>2||fg.active>1||!same(identity,{fg.session,fg.revision,fg.checksum,fg.world})||!fg.observedAtMs||fg.observedAtMs>now||now-fg.observedAtMs>3000){out.fg.provider=out.fg.active=out.fg.fault=0;out.fg.rendered=out.fg.generated=0;}
 if(valid(identity)){out.world=identity.world;if(amdRevision==identity.revision&&amdChecksum==identity.checksum){out.amdMode=amdMode;out.amdFrames=amdFrames;}}
 return out;
}
}
