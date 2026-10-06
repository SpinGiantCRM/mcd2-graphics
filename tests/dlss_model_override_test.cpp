#include "dlss_model_override.hpp"
#include <cassert>
int main(){
 using mcd2::dlss::model_hint;
 assert(model_hint("0")==0);assert(model_hint("11")==11);
 assert(model_hint("16")==16);assert(model_hint("255")==255);
 for(auto s:{"","-1","256","4294967296","11x","1.5","+11"," 11"})assert(model_hint(s)==0);
}
