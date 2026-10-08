#include "../src/latency/engine_layout.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <vector>

using namespace mcd2::engine;
using Image = std::map<std::uint32_t,std::vector<std::uint8_t>>;
static auto reader(const Image& image) {
    return [&image](std::uint32_t rva,std::span<const std::uint8_t> expected) {
        const auto it=image.find(rva);
        return it!=image.end() && std::equal(expected.begin(),expected.end(),it->second.begin(),it->second.end());
    };
}
int main() {
    Image oldImage, newImage;
    assert(guards(released,[&](auto rva,auto bytes){oldImage[rva]={bytes.begin(),bytes.end()};return true;}));
    const std::set<std::uint32_t> addresses{updated.simStart,updated.simStartDispatch,updated.simEndDispatch,
        updated.pacingDispatch,updated.registryGet,updated.count,updated.add,updated.remove,updated.nameConstructor};
    assert(select(updated.timestamp,updated.imageSize,[&](auto rva,auto bytes){
        if(!addresses.contains(rva))return false;
        newImage[rva]={bytes.begin(),bytes.end()};return true;
    })==&updated);
    assert(oldImage.size()==5 && newImage.size()==9);
    assert(select(0,0,reader(oldImage))==&released);
    assert(select(updated.timestamp,updated.imageSize,reader(oldImage))==&released);
    assert(select(updated.timestamp,updated.imageSize,reader(newImage))==&updated);
    assert(!select(updated.timestamp^1,updated.imageSize,reader(newImage)));
    assert(!select(updated.timestamp,updated.imageSize-1,reader(newImage)));
    assert(!select(updated.timestamp,updated.imageSize,reader(Image{})));
    unsigned rejects=0;
    for(const auto* reference:{&oldImage,&newImage})for(const auto& [rva,bytes]:*reference) {
        auto missing=*reference;missing.erase(rva);
        assert(!select(updated.timestamp,updated.imageSize,reader(missing)));++rejects;
        for(std::size_t n=0;n<bytes.size();++n) {
            auto corrupt=*reference;corrupt[rva][n]^=1;
            assert(!select(updated.timestamp,updated.imageSize,reader(corrupt)));++rejects;
        }
    }
    Image mixed;
    mixed[released.simStart]=oldImage.at(released.simStart);
    mixed[updated.simStartDispatch]=newImage.at(updated.simStartDispatch);
    assert(!select(updated.timestamp,updated.imageSize,reader(mixed)));
    // Explicit maps retain their independent counter and registry identities.
    assert(released.registryGet==0x12a64d0 && released.registry==0xba78270);
    assert(released.count==0x12a7c80 && released.add==0x12b4eb0 && released.remove==0x12bb990);
    assert(released.nameConstructor==0x1461040 && released.simulationCounter==0xbe45f10 && released.renderCounter==0xbe45f18);
    assert(updated.renderCounter==updated.simulationCounter+8);
    for(const auto rva:addresses)assert(rva<updated.imageSize);
    std::cout<<"Both explicit engine layouts passed; "<<rejects<<" missing/corrupt guards rejected; unknown and mixed layouts refused.\n";
}
