#pragma once
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <string>
#include <type_traits>
#include <variant>

namespace mcd2::mfg {
// Original bounded store for the public NGX parameter interface. It never owns
// COM resources. The caller retains them through command completion/retirement.
class Parameters {
    using Value=std::variant<unsigned long long,float,double,unsigned,int,void*>;
    std::map<std::string,Value> values_;
    template<class T,class U> static bool convert(U value,T& output) {
        if constexpr(std::is_arithmetic_v<T> && std::is_arithmetic_v<U>) {
            const long double number=static_cast<long double>(value);
            if constexpr(std::is_integral_v<T>) {
                constexpr int bits=std::numeric_limits<T>::digits;
                const long double upper=std::ldexp(1.L,bits);
                const long double lower=std::is_signed_v<T> ? -upper : 0.L;
                if(!std::isfinite(number) || number<lower || number>=upper || std::trunc(number)!=number) return false;
            } else if(!std::isfinite(number) || std::fabs(number)>std::numeric_limits<T>::max()) return false;
            const T converted=static_cast<T>(value);
            if(static_cast<long double>(converted)!=number) return false;
            output=converted; return true;
        } else return false;
    }
    static bool key_valid(const char* key) {
        if(!key || !*key) return false;
        for(unsigned i=0;i<128;++i) if(!key[i]) return true;
        return false;
    }
public:
    template<class T> bool set(const char* key,T value) {
        if(!key_valid(key)) return false;
        if constexpr(std::is_floating_point_v<T>) if(!std::isfinite(value)) return false;
        auto found=values_.find(key);
        if(found==values_.end() && values_.size()>=128) return false;
        values_.insert_or_assign(key,Value(value)); return true;
    }
    template<class T> bool get(const char* key,T& output) const {
        if(!key_valid(key)) return false;
        auto found=values_.find(key); if(found==values_.end()) return false;
        if(const auto exact=std::get_if<T>(&found->second)) { output=*exact; return true; }
        return std::visit([&](auto value) { return convert(value,output); },found->second);
    }
    void reset() { values_.clear(); }
};
}
