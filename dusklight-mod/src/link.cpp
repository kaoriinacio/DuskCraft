#include "link.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <sys/mman.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

namespace tpcraft {

namespace {
template <class T> T get(const uint8_t* b, uint32_t off) { T v; std::memcpy(&v, b + off, sizeof(T)); return v; }
template <class T> void put(uint8_t* b, uint32_t off, T v) { std::memcpy(b + off, &v, sizeof(T)); }
std::atomic_ref<uint32_t> seq_ref(uint8_t* b, uint32_t off) { return std::atomic_ref<uint32_t>(*reinterpret_cast<uint32_t*>(b + off)); }
std::atomic_ref<uint64_t> counter_ref(uint8_t* b, uint32_t off) { return std::atomic_ref<uint64_t>(*reinterpret_cast<uint64_t*>(b + off)); }
uint64_t now_ms() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}
}  // namespace

std::string Link::default_path() {
#ifdef _WIN32
    const char* root = std::getenv("LOCALAPPDATA");
    std::filesystem::path p = root ? root : ".";
#else
    const char* home = std::getenv("HOME");
    std::filesystem::path p = std::filesystem::path(home ? home : ".") / ".local/share";
#endif
    return (p / "TPCraft" / "link.bin").string();
}

bool Link::open(const std::string& path) {
    close();
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
#ifdef _WIN32
    HANDLE f = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    HANDLE m = CreateFileMappingA(f, nullptr, PAGE_READWRITE, 0, FILE_SIZE, nullptr);
    if (!m) { CloseHandle(f); return false; }
    void* v = MapViewOfFile(m, FILE_MAP_ALL_ACCESS, 0, 0, FILE_SIZE);
    if (!v) { CloseHandle(m); CloseHandle(f); return false; }
    file_ = f; map_ = m; base_ = static_cast<uint8_t*>(v);
#else
    int fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0) return false;
    if (ftruncate(fd, FILE_SIZE) != 0) { ::close(fd); return false; }
    void* v = mmap(nullptr, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (v == MAP_FAILED) { ::close(fd); return false; }
    fd_ = fd; base_ = static_cast<uint8_t*>(v);
#endif
    if (get<uint32_t>(base_, OFF_MAGIC) != PROTO_MAGIC || get<uint32_t>(base_, OFF_VERSION) != PROTO_VERSION) {
        std::memset(base_, 0, FILE_SIZE);
        put<uint32_t>(base_, OFF_MAGIC, PROTO_MAGIC);
        put<uint32_t>(base_, OFF_VERSION, PROTO_VERSION);
    }
    put<uint64_t>(base_, OFF_GAME_HEARTBEAT_MS, now_ms());
    set_header_flag(HDR_GAME_READY, true);
    return true;
}

void Link::close() {
    if (!base_) return;
    put<uint64_t>(base_, OFF_GAME_HEARTBEAT_MS, 0);
    set_header_flag(HDR_GAME_READY, false);
#ifdef _WIN32
    UnmapViewOfFile(base_); CloseHandle(static_cast<HANDLE>(map_)); CloseHandle(static_cast<HANDLE>(file_));
    map_ = file_ = nullptr;
#else
    munmap(base_, FILE_SIZE); ::close(fd_); fd_ = -1;
#endif
    base_ = nullptr;
}

uint32_t Link::header_flags() const {
    if (!base_) return 0;
    return seq_ref(base_, OFF_FLAGS).load(std::memory_order_acquire);
}

bool Link::mc_alive(uint64_t timeout_ms) const {
    if (!base_) return false;
    const uint64_t heartbeat = get<uint64_t>(base_, OFF_MC_HEARTBEAT_MS);
    const uint64_t now = now_ms();
    return heartbeat != 0 && now >= heartbeat && now - heartbeat <= timeout_ms;
}

void Link::set_header_flag(uint32_t mask, bool on) {
    if (!base_) return;
    auto flags = seq_ref(base_, OFF_FLAGS);
    uint32_t current = flags.load(std::memory_order_relaxed);
    uint32_t updated;
    do {
        updated = on ? (current | mask) : (current & ~mask);
    } while (!flags.compare_exchange_weak(current, updated, std::memory_order_release,
                                           std::memory_order_relaxed));
}

void Link::publish_game(const GameState& gs) {
    if (!base_) return;
    uint8_t* g = base_ + GAME_OFF;
    auto seq = seq_ref(g, G_SEQ);
    uint32_t s = seq.load(std::memory_order_relaxed);
    seq.store(s + 1, std::memory_order_release);             // impar: escrevendo
    put(g, G_STAGE_HASH, gs.stageHash);
    std::memcpy(g + G_POS, gs.pos, 12);
    put(g, G_YAW, gs.yaw);
    std::memcpy(g + G_VEL, gs.vel, 12);
    put(g, G_STATE, gs.state);
    put(g, G_TIME, gs.timeOfDay);
    put(g, G_GRID_CX, gs.gridCx);
    put(g, G_GRID_CZ, gs.gridCz);
    put(g, G_GRID_CELL, gs.gridCell);
    put<uint32_t>(g, G_GRID_DIM, GRID_DIM);
    std::memcpy(g + G_HEIGHTS, gs.heights, sizeof(gs.heights));
    seq.store(s + 2, std::memory_order_release);             // par: pronto
    put<uint64_t>(base_, OFF_GAME_FRAME, get<uint64_t>(base_, OFF_GAME_FRAME) + 1);
    put<uint64_t>(base_, OFF_GAME_HEARTBEAT_MS, now_ms());
}

bool Link::read_mc(McState& out) const {
    if (!base_) return false;
    uint8_t* m = base_ + MC_OFF;
    auto seq = seq_ref(m, M_SEQ);
    for (int attempt = 0; attempt < 4; ++attempt) {
        uint32_t s1 = seq.load(std::memory_order_acquire);
        if (s1 & 1u) continue;
        McState t;
        t.flags = get<uint32_t>(m, M_FLAGS);
        std::memcpy(t.pos, m + M_POS, 12);
        t.yaw = get<float>(m, M_YAW);
        t.pitch = get<float>(m, M_PITCH);
        std::memcpy(t.vel, m + M_VEL, 12);
        t.health = get<float>(m, M_HEALTH);
        t.hotbar = get<uint32_t>(m, M_HOTBAR);
        t.held = get<uint32_t>(m, M_HELD);
        t.attacks = get<uint32_t>(m, M_ATTACKS);
        std::atomic_thread_fence(std::memory_order_acquire);
        if (seq.load(std::memory_order_relaxed) == s1) { out = t; return s1 != 0; }
    }
    return false;
}

bool Link::push_input(const InputEvent& event) {
    if (!base_) return false;
    uint8_t* ring = base_ + INPUT_OFF;
    auto headRef = counter_ref(ring, INPUT_HEAD);
    auto tailRef = counter_ref(ring, INPUT_TAIL);
    const uint64_t head = headRef.load(std::memory_order_relaxed);
    const uint64_t tail = tailRef.load(std::memory_order_acquire);
    if (head - tail >= INPUT_COUNT) return false;

    uint8_t* entry = ring + INPUT_DATA + (head & (INPUT_COUNT - 1)) * INPUT_ENTRY_SIZE;
    std::memcpy(entry, &event, sizeof(event));
    headRef.store(head + 1, std::memory_order_release);
    return true;
}

bool Link::pop_render(RenderMessage& out) {
    if (!base_) return false;
    uint8_t* ring = base_ + RENDER_OFF;
    auto headRef = counter_ref(ring, RENDER_HEAD);
    auto tailRef = counter_ref(ring, RENDER_TAIL);
    const uint64_t dataBytes = RENDER_DATA_BYTES;

    for (;;) {
        uint64_t tail = tailRef.load(std::memory_order_relaxed);
        const uint64_t head = headRef.load(std::memory_order_acquire);
        if (tail >= head) return false;

        const uint64_t position = tail % dataBytes;
        const uint8_t* entry = ring + RENDER_DATA + position;
        const uint32_t type = get<uint32_t>(entry, 0);
        const uint32_t payloadBytes = get<uint32_t>(entry, 4);
        if (type == REN_PAD) {
            tailRef.store(tail + dataBytes - position, std::memory_order_release);
            continue;
        }

        const uint64_t messageBytes = (8ull + payloadBytes + 7ull) & ~7ull;
        if (messageBytes > head - tail || messageBytes > dataBytes) return false;
        out.type = type;
        out.payload.resize(payloadBytes);
        if (payloadBytes != 0) {
            std::memcpy(out.payload.data(), entry + 8, payloadBytes);
        }
        tailRef.store(tail + messageBytes, std::memory_order_release);
        return true;
    }
}

}  // namespace tpcraft
