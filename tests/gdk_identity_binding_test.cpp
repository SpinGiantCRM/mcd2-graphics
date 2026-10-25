#include "../src/gdk/identity_binding.hpp"
#include <cassert>
#include <chrono>
int main(){
 using namespace mcd2::gdk;
 auto root=std::filesystem::temp_directory_path()/("gdk-binding-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root/"1234567890ABCDEF_000000001234567890ABCDEF12345678");
 std::filesystem::create_directories(root/"unrelated-game-data");
 unsigned finds=0,closes=0;BoundIdentity selected;
 auto find=[&](std::uint64_t user){++finds;return user==0x1234567890abcdefULL?std::uintptr_t(11):std::uintptr_t(22);};
 auto close=[&](std::uintptr_t){++closes;};
 assert(selectIdentity(root,find,close,selected)==IdentityStatus::Ready);
 assert(finds==1&&closes==0&&selected.handle==11&&selected.identity.user==0x1234567890abcdefULL);
 close(selected.handle);
 std::filesystem::create_directories(root/"2234567890ABCDEF_000000001234567890ABCDEF12345678");
 assert(selectIdentity(root,find,close,selected)==IdentityStatus::Ambiguous);
 assert(closes==3); // Both temporary API handles are closed on refusal.
 auto missing=[](std::uint64_t){return std::uintptr_t(0);};
 assert(selectIdentity(root,missing,close,selected)==IdentityStatus::Missing);
 std::filesystem::remove_all(root);
 assert(selectIdentity(root,find,close,selected)==IdentityStatus::Filesystem);
}
