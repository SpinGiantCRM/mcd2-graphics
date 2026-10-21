#include "../src/gdk/save_slots.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>

int main(int argc,char** argv){
    using namespace mcd2::gdk;
    assert(argc==2);
    Identity parsed;
    assert(identity("1234567890ABCDEF_000000001234567890ABCDEF12345678",parsed));
    assert(parsed.user==0x1234567890ABCDEFull);
    assert(parsed.scid=="00000000-1234-5678-90AB-CDEF12345678");
    for(const auto* invalid:{"t","0000000000000000_000000001234567890ABCDEF12345678",
        "1234567890ABCDEG_000000001234567890ABCDEF12345678",
        "1234567890ABCDEF/000000001234567890ABCDEF12345678",
        "1234567890ABCDEF_000000001234567890ABCDEF1234567Z"})assert(!identity(invalid,parsed));
    const char* fixtures[]={"settings.sav","display-settings.sav","fg-settings.sav","runtime.sav","display-runtime.sav","fg-runtime.sav"};
    for(std::size_t index=0;index<slots.size();++index){
        assert(slots[index].nativeWrites==(index>=3));
        std::ifstream file(std::filesystem::path(argv[1])/fixtures[index],std::ios::binary);
        std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(file),{});
        assert(valid(slots[index],bytes));
        std::size_t tableAt=0,tableEnd=0;unsigned count=0;
        assert(versionTable(bytes,tableAt,tableEnd,count)&&count==93);
        auto gdk=bytes;gdk.erase(gdk.begin()+tableEnd-60,gdk.begin()+tableEnd);gdk[tableAt]=90;
        assert(valid(slots[index],gdk));
        const auto cache=nativePayload(gdk);
        assert(cache==bytes);
        assert(storePayload(cache,gdk)==gdk);
        auto original=gdk;original[tableAt+4]=0x5a;
        assert(storePayload(nativePayload(original),original)==original);
        assert(storePayload(bytes,bytes)==bytes);
        auto unsupported=gdk;unsupported[tableAt]=89;assert(!valid(slots[index],unsupported));
        auto changed=cache;changed[tableAt+4]=1;assert(storePayload(changed,gdk).empty());
        for(std::size_t other=0;other<slots.size();++other)if(other!=index)assert(!valid(slots[other],bytes));
        auto damaged=bytes;damaged[0]=0;assert(!valid(slots[index],damaged));
        damaged=bytes;damaged.push_back(0);assert(!valid(slots[index],damaged));
        assert(!valid(slots[index],std::vector<std::uint8_t>(8193)));
    }
    assert(!valid({"PlayerSave","GraphicsRuntimeStateSave",true},{}));
}
