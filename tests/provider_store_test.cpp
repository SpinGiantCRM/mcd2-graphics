#include "../src/providers/graphics_store.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace mcd2::providers;
namespace fs = std::filesystem;

static void rawWrite(const fs::path& path, std::span<const std::uint8_t> bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
    assert(file.good());
}

static int run(const fs::path& root, int argc, char** argv) {
    assert(argc >= 2);
    // A Python coordinator supplies fixture directories and independently
    // terminates these helper processes; no real save directory is discovered.
    GraphicsStore store(root);
    if (argc >= 3) {
        const std::string command = argv[2];
        if (command == "hold" || command == "prepare") {
            store_detail::WriterLock lock(store.lockPath());
            assert(lock.status == StoreStatus::Ok);
            if (command == "prepare") {
                DecodedGraphicsRecord current;
                assert(store.load(current).status == StoreStatus::Ok);
                ++current.intent.revision;
                current.intent.fgEnabled = true;
                current.intent.fg = FgProvider::Amd;
                GraphicsRecord pending;
                assert(encodeGraphicsRecord(current.intent, pending));
                assert(store_detail::writeFlushed(store.pendingPath(), pending));
            }
            std::cout << "ready\n" << std::flush;
            std::string stop;
            std::getline(std::cin, stop);
            return 0;
        }
        if (command == "publish") {
            assert(argc == 6);
            GraphicsIntent request;
            request.revision = std::stoul(argv[3]);
            RecordStamp expected{std::uint32_t(std::stoul(argv[4])), std::uint32_t(std::stoul(argv[5]))};
            const auto result = store.publish(request, expected);
            std::cout << unsigned(result.status) << ' ' << result.stamp.revision << ' ' << result.stamp.checksum << '\n';
            return 0;
        }
        if (command == "read") {
            DecodedGraphicsRecord value;
            const auto result = store.load(value);
            std::cout << unsigned(result.status) << ' ' << result.stamp.revision << ' ' << result.stamp.checksum << '\n';
            return 0;
        }
        assert(false);
    }

    unsigned roundTrips = 0, rejected = 0;
    GraphicsRecord bytes;
    for (unsigned sr = 0; sr <= 3; ++sr)
        for (unsigned fg = 0; fg <= 2; ++fg)
            for (unsigned strategy = 0; strategy <= 2; ++strategy)
                for (unsigned latency = 0; latency <= 3; ++latency) {
                    GraphicsIntent value;
                    value.sr = SrProvider(sr); value.fgEnabled = true; value.fg = FgProvider(fg);
                    value.fgStrategy = FgStrategy(strategy); value.latency = LatencyProvider(latency);
                    value.requestedMultiplier = strategy ? 4 : 2;
                    value.srPreferences[1] = {Quality::Custom, 7100, 7900};
                    value.srPreferences[2] = {Quality::NativeAA, 10000, 8700};
                    value.hdr = {true, 410, 167, 203};
                    assert(encodeGraphicsRecord(value, bytes));
                    DecodedGraphicsRecord decoded;
                    assert(decodeGraphicsRecord(bytes, decoded) && decoded.intent == value);
                    assert(decoded.bootstrap.requested == requestedOwner(value));
                    ++roundTrips;
                }
    GraphicsIntent request;
    request.revision = 9; request.fgEnabled = true;
    request.migratedFrom = {9,3,5};
    assert(encodeGraphicsRecord(request, bytes));
    assert(std::equal(recordMagic.begin(), recordMagic.end(), bytes.begin()));
    assert(recordWord(bytes, 8) == 1 && recordWord(bytes, 12) == 128);
    auto refuses = [&](std::span<const std::uint8_t> broken) {
        DecodedGraphicsRecord out; out.intent.revision = 123;
        const auto original = out;
        assert(!decodeGraphicsRecord(broken, out) && out == original);
        const auto startup = selectStartup(broken, 42);
        assert(!startup.valid && startup.bootstrap.requested == PresentationOwner::Native);
        ++rejected;
    };
    for (std::size_t size = 0; size < bytes.size(); ++size) refuses(std::span(bytes).first(size));
    std::array<std::uint8_t, 129> extended{};
    std::copy(bytes.begin(), bytes.end(), extended.begin()); refuses(extended);
    for (std::size_t at = 0; at < bytes.size(); ++at)
        for (unsigned bit = 0; bit != 8; ++bit) {
            auto broken = bytes; broken[at] ^= std::uint8_t(1u << bit); refuses(broken);
        }
    // Valid checksums cannot authorize malformed enums, booleans, owner,
    // revisions, calibration or migration metadata.
    constexpr std::array<std::pair<unsigned,unsigned>, 13> mutations{{
        {8,2}, {12,124}, {20,2}, {24,6}, {28,0}, {32,4}, {36,6},
        {72,2}, {76,2}, {100,2}, {104,411}, {116,0}, {124,10}
    }};
    for (auto [offset, word] : mutations) {
        auto broken = bytes; setRecordWord(broken, offset, word);
        setRecordWord(broken, checksumOffset, recordChecksum(broken)); refuses(broken);
    }
    const auto startup = selectStartup(bytes, 42);
    assert(startup.valid && matchesStartup(startup, bytes, 42));
    assert(!matchesStartup(startup, bytes, 43));
    assert(!selectStartup(bytes, 0).valid);
    auto changed = request; changed.fg = FgProvider::Amd;
    GraphicsRecord different;
    assert(encodeGraphicsRecord(changed, different));
    assert(!matchesStartup(startup, different, 42)); // Same revision, different intent.
    ++changed.revision;
    assert(encodeGraphicsRecord(changed, different));
    assert(!matchesStartup(startup, different, 42));
    auto invalidIntent = request; invalidIntent.migratedFrom.sr = 0;
    const auto oldBytes = bytes;
    assert(!encodeGraphicsRecord(invalidIntent, bytes) && bytes == oldBytes);

    // Filesystem transaction gates, with no real/player data.
    assert(fs::is_directory(root));
    DecodedGraphicsRecord observed;
    assert(store.load(observed).status == StoreStatus::Missing);
    auto published = store.publish(request, {});
    assert(published.status == StoreStatus::Ok && published.stamp.revision == 9);
    assert(store.load(observed).status == StoreStatus::Ok && observed.intent == request);
    const auto initial = observed;
    assert(!fs::exists(store.pendingPath()));
    assert(store.publish(changed, {}).status == StoreStatus::Conflict);
    assert(store.publish(request, published.stamp).status == StoreStatus::Conflict);
    auto wrongStamp = published.stamp; wrongStamp.checksum ^= 1;
    assert(store.publish(changed, wrongStamp).status == StoreStatus::Conflict);
    assert(store.load(observed).status == StoreStatus::Ok && observed == initial);
    {
        store_detail::WriterLock lock(store.lockPath());
        assert(lock.status == StoreStatus::Ok);
        assert(store.publish(changed, published.stamp).status == StoreStatus::Busy);
    }
    // An unfinished record is never a startup source, even with a valid CRC.
    rawWrite(store.pendingPath(), different);
    assert(store.load(observed).status == StoreStatus::Ok && observed == initial);
    published = store.publish(changed, published.stamp);
    assert(published.status == StoreStatus::Ok && !fs::exists(store.pendingPath()));
    assert(store.load(observed).status == StoreStatus::Ok && observed.intent == changed);
    const auto accepted = observed;
    ++changed.revision;
    for (unsigned size : {0u,1u,64u,127u}) {
        rawWrite(store.committedPath(), std::span(different).first(size));
        assert(store.load(observed).status == StoreStatus::Invalid && observed == accepted);
        assert(store.publish(changed, {}).status == StoreStatus::Invalid);
        assert(fs::file_size(store.committedPath()) == size);
    }
    rawWrite(store.committedPath(), different);
    // A non-file pending path is refused rather than deleted recursively.
    fs::create_directory(store.pendingPath());
    assert(store.publish(changed, published.stamp).status == StoreStatus::IoError);
    assert(fs::is_directory(store.pendingPath()));
    fs::remove(store.pendingPath());
#ifndef _WIN32
    const auto target = root / "untouched.bin";
    rawWrite(target, different);
    fs::create_symlink(target, store.pendingPath());
    assert(store.publish(changed, published.stamp).status == StoreStatus::IoError);
    assert(fs::file_size(target) == 128);
    fs::remove(store.pendingPath());
    fs::remove(store.committedPath());
    fs::create_symlink(target, store.committedPath());
    assert(store.load(observed).status != StoreStatus::Ok);
    assert(store.publish(changed, published.stamp).status != StoreStatus::Ok);
    fs::remove(store.committedPath());
    rawWrite(store.committedPath(), different);
#endif
    GraphicsStore relative("not-an-absolute-fixture");
    assert(relative.publish(request, {}).status == StoreStatus::IoError);
    std::cout << roundTrips << " provider combinations round-tripped; " << rejected
              << " corrupt/partial records rejected; startup and filesystem gates passed.\n";
    return 0;
}

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
    assert(argc >= 2);
    std::vector<std::string> arguments(std::size_t(argc), std::string{});
    std::vector<char*> pointers(std::size_t(argc), nullptr);
    // Only command names and numeric parameters use ASCII. The fixture path
    // stays UTF-16 throughout Windows filesystem operations.
    for (int n = 2; n < argc; ++n) arguments[n] = std::string(argv[n], argv[n]+wcslen(argv[n]));
    for (int n = 0; n < argc; ++n) pointers[n] = arguments[n].data();
    return run(fs::absolute(argv[1]), argc, pointers.data());
}
#else
int main(int argc, char** argv) {
    assert(argc >= 2);
    return run(fs::absolute(argv[1]), argc, argv);
}
#endif
