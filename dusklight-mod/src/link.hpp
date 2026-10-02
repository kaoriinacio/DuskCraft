// Ponte de memoria compartilhada (lado do jogo). Independente do Dusklight:
// compila sozinha (C++20) e e testada com tools/fake_dusklight.py.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "../../protocol/tpcraft_protocol.h"

namespace tpcraft {

struct GameState {
    uint32_t stageHash = 0;
    float pos[3] = {0, 0, 0};
    float yaw = 0;
    float vel[3] = {0, 0, 0};
    uint32_t state = 0;       // GS_*
    float timeOfDay = 12.0f;
    float gridCx = 0, gridCz = 0, gridCell = UNITS_PER_BLOCK;
    float heights[GRID_DIM * GRID_DIM] = {};  // preencher (NaN = sem chao)
};

struct McState {
    uint32_t flags = 0;       // MF_*
    float pos[3] = {0, 0, 0};
    float yaw = 0, pitch = 0;
    float vel[3] = {0, 0, 0};
    float health = 20.0f;
    uint32_t hotbar = 0, held = 0, attacks = 0;
};

struct InputEvent {
    uint16_t type = 0, code = 0;
    int32_t a = 0, b = 0, c = 0;
};
static_assert(sizeof(InputEvent) == INPUT_ENTRY_SIZE);

struct RenderMessage {
    uint32_t type = 0;
    std::vector<uint8_t> payload;
};

class Link {
public:
    ~Link() { close(); }
    // Abre (e cria, se preciso) o arquivo de link. Caminho padrao: default_path().
    bool open(const std::string& path);
    void close();
    bool is_open() const { return base_ != nullptr; }

    void set_header_flag(uint32_t mask, bool on);
    uint32_t header_flags() const;
    bool mc_ready() const { return header_flags() & HDR_MC_READY; }
    bool mc_alive(uint64_t timeout_ms = 3000) const;
    bool mc_drives() const { return (header_flags() & HDR_MC_DRIVES) && mc_alive(); }

    void publish_game(const GameState& gs);  // chamar 1x por frame
    bool read_mc(McState& out) const;        // false se leitura instavel (tente no proximo frame)
    bool push_input(const InputEvent& event); // false se o guest nao consumiu a fila a tempo
    bool pop_render(RenderMessage& out);     // consome uma mensagem Minecraft->Dusklight

    static std::string default_path();       // %LOCALAPPDATA%\TPCraft\link.bin (ou ~/.local/share/TPCraft/link.bin)

private:
    uint8_t* base_ = nullptr;
#ifdef _WIN32
    void* file_ = nullptr;
    void* map_ = nullptr;
#else
    int fd_ = -1;
#endif
};

}  // namespace tpcraft
