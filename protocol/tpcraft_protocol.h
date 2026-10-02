// TPCraft shared-memory protocol v3.
// Arquivo mapeado em memoria (file-backed mmap) compartilhado entre:
//   - mod do Dusklight (C++)            -> escreve GameBlock, le McBlock
//   - mod Fabric do Minecraft (Java)    -> le GameBlock, escreve McBlock
//   - tools/fake_dusklight.py           -> stand-in do jogo para testes
// Espelhado em: fabric/.../link/Proto.java e tools/fake_dusklight.py
// Little-endian. Changes to this layout must be mirrored in Java and Python.
#pragma once
#include <cstdint>

namespace tpcraft {

constexpr uint32_t PROTO_MAGIC   = 0x52435054u;  // 'T','P','C','R'
constexpr uint32_t PROTO_VERSION = 3;
constexpr uint32_t RENDER_OFF = 16384;
constexpr uint32_t RENDER_BYTES = 64u * 1024u * 1024u;
constexpr uint32_t FILE_SIZE = RENDER_OFF + RENDER_BYTES;
constexpr uint32_t GRID_DIM  = 33;          // heightfield 33x33 ao redor do Link

// ---- Header (offset 0, 64 bytes) -------------------------------------------
constexpr uint32_t OFF_MAGIC      = 0;   // u32
constexpr uint32_t OFF_VERSION    = 4;   // u32
constexpr uint32_t OFF_FLAGS      = 8;   // u32 (HDR_*)
constexpr uint32_t OFF_GAME_FRAME = 16;  // u64, incrementa a cada frame do jogo
constexpr uint32_t OFF_MC_TICK    = 24;  // u64, incrementa a cada tick do Minecraft
constexpr uint32_t OFF_GAME_HEARTBEAT_MS = 32; // u64, Unix epoch ms
constexpr uint32_t OFF_MC_HEARTBEAT_MS   = 40; // u64, Unix epoch ms

constexpr uint32_t HDR_GAME_READY = 1u << 0;
constexpr uint32_t HDR_MC_READY   = 1u << 1;
constexpr uint32_t HDR_MC_DRIVES  = 1u << 2;  // Minecraft controla o Link (F7 desliga)

// ---- GameBlock: jogo -> Minecraft (offset 64) ------------------------------
// Protegido por seqlock: writer faz seq++ (impar), escreve, seq++ (par).
constexpr uint32_t GAME_OFF = 64;
constexpr uint32_t G_SEQ        = 0;   // u32
constexpr uint32_t G_STAGE_HASH = 4;   // u32 (hash do nome do stage/sala)
constexpr uint32_t G_POS        = 8;   // f32 x3 (unidades do TP)
constexpr uint32_t G_YAW        = 20;  // f32 (radianos)
constexpr uint32_t G_VEL        = 24;  // f32 x3
constexpr uint32_t G_STATE      = 36;  // u32 (GS_*)
constexpr uint32_t G_TIME       = 40;  // f32 hora do dia 0..24
constexpr uint32_t G_GRID_CX    = 44;  // f32 centro do grid (x)
constexpr uint32_t G_GRID_CZ    = 48;  // f32 centro do grid (z)
constexpr uint32_t G_GRID_CELL  = 52;  // f32 tamanho da celula (unidades TP)
constexpr uint32_t G_GRID_DIM   = 56;  // u32
constexpr uint32_t G_HEIGHTS    = 64;  // f32[GRID_DIM*GRID_DIM], chao em Y; NaN = sem chao

constexpr uint32_t GS_CUTSCENE = 1u << 0;
constexpr uint32_t GS_RIDING   = 1u << 1;  // Epona
constexpr uint32_t GS_WOLF     = 1u << 2;
constexpr uint32_t GS_SWIMMING = 1u << 3;
constexpr uint32_t GS_LOADING  = 1u << 4;

// ---- McBlock: Minecraft -> jogo (offset 8256) ------------------------------
constexpr uint32_t MC_OFF = 8256;
constexpr uint32_t M_SEQ      = 0;   // u32
constexpr uint32_t M_FLAGS    = 4;   // u32 (MF_*)
constexpr uint32_t M_POS      = 8;   // f32 x3 (em blocos de Minecraft)
constexpr uint32_t M_YAW      = 20;  // f32 (graus MC)
constexpr uint32_t M_PITCH    = 24;  // f32 (graus MC)
constexpr uint32_t M_VEL      = 28;  // f32 x3 (blocos/tick)
constexpr uint32_t M_HEALTH   = 40;  // f32 (0..20)
constexpr uint32_t M_HOTBAR   = 44;  // u32 slot 0..8
constexpr uint32_t M_HELD     = 48;  // u32 hash do item na mao
constexpr uint32_t M_ATTACKS  = 52;  // u32 contador de swings

constexpr uint32_t MF_ON_GROUND = 1u << 0;
constexpr uint32_t MF_SNEAK     = 1u << 1;
constexpr uint32_t MF_SPRINT    = 1u << 2;
constexpr uint32_t MF_SWIMMING  = 1u << 3;

// ---- Input ring: Dusklight -> Minecraft ------------------------------------
// Single producer/consumer ring. Head and tail are monotonic counters.
constexpr uint32_t INPUT_OFF = 8384;
constexpr uint32_t INPUT_HEAD = 0;   // u64, host-owned
constexpr uint32_t INPUT_TAIL = 64;  // u64, guest-owned
constexpr uint32_t INPUT_DATA = 128;
constexpr uint32_t INPUT_COUNT = 256;
constexpr uint32_t INPUT_ENTRY_SIZE = 16;

constexpr uint16_t IN_KEY = 1;          // code = USB HID usage, a = down (1/0)
constexpr uint16_t IN_MOUSE_BUTTON = 2; // code = SDL button (1 L, 2 M, 3 R), a = down
constexpr uint16_t IN_SCROLL = 3;       // a = wheel delta
constexpr uint16_t IN_TEXT = 4;         // a = Unicode code point
constexpr uint16_t IN_RELEASE_ALL = 5;  // release every held key and button
constexpr uint16_t IN_OPEN_MENU = 6;    // open the Minecraft menu
constexpr uint16_t IN_LOOK = 7;         // a/b = mouse delta x/y

// ---- Render ring: Minecraft -> Dusklight -----------------------------------
// Byte ring with 8-byte-aligned {type, payloadBytes, payload} messages.
constexpr uint32_t RENDER_HEAD = 0;   // u64, guest-owned
constexpr uint32_t RENDER_TAIL = 64;  // u64, host-owned
constexpr uint32_t RENDER_DATA = 128;
constexpr uint32_t RENDER_DATA_BYTES = RENDER_BYTES - RENDER_DATA;
constexpr uint32_t REN_PAD = 0;          // advance to the start of the ring
constexpr uint32_t REN_CLEAR_ALL = 1;    // clear all host-side Minecraft resources
constexpr uint32_t REN_ATLAS = 2;        // width, height, RGBA8 pixels
constexpr uint32_t REN_SECTION = 3;      // section x/y/z, then RenderVertex[]
constexpr uint32_t REN_ATLAS_REGION = 4; // x/y/width/height, RGBA8 pixels
constexpr uint32_t REN_TEXTURE = 5;      // id/width/height, RGBA8 pixels
constexpr uint32_t REN_AVATAR = 6;       // batch header, then RenderVertex[]
constexpr uint32_t REN_SCENE = 7;        // origin, batches, then RenderVertex[]
constexpr uint32_t RENDER_VERTEX_SIZE = 32;

// Escala de mundo: quantas unidades do TP equivalem a 1 bloco do Minecraft.
// VALOR PROVISORIO -- medir no jogo (altura do Link vs 1.8 blocos do Steve).
constexpr float UNITS_PER_BLOCK = 100.0f;

static_assert(INPUT_DATA + INPUT_COUNT * INPUT_ENTRY_SIZE <= FILE_SIZE - INPUT_OFF);
static_assert(RENDER_DATA < RENDER_BYTES && RENDER_DATA_BYTES % 8 == 0);

}  // namespace tpcraft
