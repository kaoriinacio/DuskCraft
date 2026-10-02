// Teste de integracao: le o que o outro lado escreveu e responde.
//   ./link_selftest write <path>   -> escreve um GameState conhecido
//   ./link_selftest read  <path>   -> imprime o McState lido (para o teste Java/Python)
#include "link.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
using namespace tpcraft;
int main(int argc, char** argv) {
    if (argc < 3) return 2;
    Link l;
    if (!l.open(argv[2])) { std::puts("open failed"); return 1; }
    if (!std::strcmp(argv[1], "write")) {
        GameState gs; gs.stageHash = 0xABCD1234u; gs.pos[0] = 1.5f; gs.pos[1] = -2.25f; gs.pos[2] = 300.f;
        gs.yaw = 0.5f; gs.state = GS_SWIMMING; gs.timeOfDay = 18.5f; gs.gridCx = 10; gs.gridCz = 20;
        for (uint32_t i = 0; i < GRID_DIM * GRID_DIM; ++i) gs.heights[i] = float(i);
        gs.heights[5] = NAN;
        l.publish_game(gs);
        std::puts("written");
    } else if (!std::strcmp(argv[1], "push-input")) {
        if (!l.push_input(InputEvent{IN_KEY, 30, 1, 2, 3})) return 1;
        std::puts("input written");
    } else if (!std::strcmp(argv[1], "read-render")) {
        RenderMessage message;
        if (!l.pop_render(message)) return 1;
        std::printf("type=%u size=%zu payload=%.*s\n", message.type, message.payload.size(),
                    static_cast<int>(message.payload.size()), message.payload.data());
    } else {
        McState m;
        if (!l.read_mc(m)) { std::puts("read failed"); return 1; }
        std::printf("flags=%u pos=%.2f,%.2f,%.2f yaw=%.1f health=%.1f hotbar=%u held=%u attacks=%u\n",
                    m.flags, m.pos[0], m.pos[1], m.pos[2], m.yaw, m.health, m.hotbar, m.held, m.attacks);
    }
    return 0;
}
