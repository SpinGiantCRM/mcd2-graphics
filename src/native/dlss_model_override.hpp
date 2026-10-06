#pragma once
#include <charconv>
#include <string_view>
namespace mcd2::dlss {
// Zero leaves the model choice to the installed NVIDIA runtime. Unknown numeric
// hints are experimental; the runtime may ignore them or reject the feature.
inline unsigned model_hint(std::string_view text) {
 if(text.empty())return 0;
 unsigned value=0;
 const auto result=std::from_chars(text.data(),text.data()+text.size(),value);
 return result.ec==std::errc{}&&result.ptr==text.data()+text.size()&&value<=255?value:0;
}
}
