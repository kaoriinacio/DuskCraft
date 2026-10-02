// TPCraft -- mod nativo do Dusklight. Fase 1: "Link vivo".
//   mod_update (todo frame):
//     1) publica o estado do Link/jogo no link.bin (GameState)
//     2) se o Minecraft estiver controlando (HDR_MC_DRIVES), aplica a posicao dele no Link
//
// ATENCAO: os nomes da API do jogo (dComIfGp_getPlayer, dBgS_GndChk, ...) foram conferidos nos
// headers do Dusklight, mas este arquivo ainda NAO foi testado rodando no jogo.
#include "mods/service.hpp"
#include "mods/svc/log.hpp"

#include "d/d_bg_s.h"
#include "d/d_bg_s_gnd_chk.h"
#include "d/d_com_inf_game.h"
#include "d/d_kankyo.h"
#include "d/actor/d_a_alink.h"
#include "f_op/f_op_actor_mng.h"
#include "mods/svc/hook.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>

#include "link.hpp"

DEFINE_MOD();
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(HookService, svc_hook);

namespace {

tpcraft::Link g_link;
tpcraft::GameState g_gs;
uint32_t g_lastAttacks = 0;
bool g_havePrevMc = false;
bool g_haveDriveOrigin = false;
float g_mcOrigin[3] = {};
float g_gameOrigin[3] = {};
uint32_t g_driveStageHash = 0;

DEFINE_HOOK(&daAlink_c::execute, LinkExecute);

// FNV-1a do nome do stage (ate 8 chars) -> hash estavel entre execucoes.
uint32_t hash_stage(const char* s) {
    uint32_t h = 2166136261u;
    for (; s && *s; ++s) { h ^= static_cast<uint8_t>(*s); h *= 16777619u; }
    return h;
}

// Raio vertical no chao do jogo; NaN se nao houver.
float ground_y(float x, float y, float z) {
    cXyz p(x, y, z);
    dBgS_GndChk gnd;
    gnd.SetPos(&p);
    float gy = dComIfG_Bgsp().GroundCross(&gnd);
    return (gy == -G_CM3D_F_INF) ? NAN : gy;
}

float ground_sample_for_world(float wx, float wz) {
    using namespace tpcraft;

    if (!std::isfinite(g_gs.gridCx) || !std::isfinite(g_gs.gridCz) || g_gs.gridCell <= 0.0f) {
        return NAN;
    }

    const float half = (GRID_DIM / 2) * g_gs.gridCell;
    const float dx = wx - g_gs.gridCx;
    const float dz = wz - g_gs.gridCz;
    if (dx < -half || dx > half || dz < -half || dz > half) {
        return NAN;
    }

    const uint32_t ix = static_cast<uint32_t>((dx + half) / g_gs.gridCell);
    const uint32_t iz = static_cast<uint32_t>((dz + half) / g_gs.gridCell);
    if (ix >= GRID_DIM || iz >= GRID_DIM) {
        return NAN;
    }

    const float ground = g_gs.heights[iz * GRID_DIM + ix];
    return std::isfinite(ground) ? ground : NAN;
}

void sample_heightfield(const cXyz& center) {
    using namespace tpcraft;
    const float cell = UNITS_PER_BLOCK;                 // 1 celula = 1 bloco
    const float half = (GRID_DIM / 2) * cell;
    g_gs.gridCx = center.x;
    g_gs.gridCz = center.z;
    g_gs.gridCell = cell;
    for (uint32_t iz = 0; iz < GRID_DIM; ++iz) {
        for (uint32_t ix = 0; ix < GRID_DIM; ++ix) {
            float wx = center.x - half + ix * cell;
            float wz = center.z - half + iz * cell;
            // dispara de cima (Link + 1 bloco) para pegar o chao sob/ao redor dele
            g_gs.heights[iz * GRID_DIM + ix] = ground_y(wx, center.y + cell, wz);
        }
    }
}

void publish_game() {
    using namespace tpcraft;
    fopAc_ac_c* link = dComIfGp_getPlayer(0);
    if (!link) return;

    g_gs.stageHash = hash_stage(dComIfGp_getStartStageName());
    g_gs.pos[0] = link->current.pos.x;
    g_gs.pos[1] = link->current.pos.y;
    g_gs.pos[2] = link->current.pos.z;
    g_gs.yaw = link->shape_angle.y * (3.14159265f / 32768.0f);  // s16 -> radianos
    g_gs.vel[0] = link->speed.x;
    g_gs.vel[1] = link->speed.y;
    g_gs.vel[2] = link->speed.z;

    uint32_t st = 0;
    if (dComIfGp_event_runCheck()) st |= GS_CUTSCENE;
    // TODO: GS_RIDING (Epona), GS_WOLF, GS_SWIMMING, GS_LOADING -- achar os checks em daAlink_c.
    g_gs.state = st;

    g_gs.timeOfDay = dKy_getdaytime_hour() + dKy_getdaytime_minute() / 60.0f;
    sample_heightfield(link->current.pos);   // custo: 33*33 raios por frame; reduzir/espalhar se pesar
    g_link.publish_game(g_gs);
}

void apply_minecraft(daAlink_c* link) {
    using namespace tpcraft;
    if (!link) return;
    if (!g_link.mc_drives()) {
        g_haveDriveOrigin = false;
        return;
    }
    if (g_gs.state & (GS_CUTSCENE | GS_RIDING | GS_LOADING)) return;  // jogo no comando

    McState mc;
    if (!g_link.read_mc(mc)) return;

    const bool has_valid_pos = std::isfinite(mc.pos[0]) && std::isfinite(mc.pos[1]) && std::isfinite(mc.pos[2]);
    const bool is_default_zero = (mc.pos[0] == 0.0f && mc.pos[1] == 0.0f && mc.pos[2] == 0.0f);
    if (!has_valid_pos || (is_default_zero && !(mc.flags & MF_ON_GROUND))) return;
    if (mc.pos[1] < -1000.0f || mc.pos[1] > 10000.0f) return;

    if (!g_haveDriveOrigin || g_driveStageHash != g_gs.stageHash) {
        for (int i = 0; i < 3; ++i) g_mcOrigin[i] = mc.pos[i];
        g_gameOrigin[0] = link->current.pos.x;
        g_gameOrigin[1] = link->current.pos.y;
        g_gameOrigin[2] = link->current.pos.z;
        g_driveStageHash = g_gs.stageHash;
        g_haveDriveOrigin = true;
    }

    const float targetX = g_gameOrigin[0] + (mc.pos[0] - g_mcOrigin[0]) * UNITS_PER_BLOCK;
    const float targetY = g_gameOrigin[1] + (mc.pos[1] - g_mcOrigin[1]) * UNITS_PER_BLOCK;
    const float targetZ = g_gameOrigin[2] + (mc.pos[2] - g_mcOrigin[2]) * UNITS_PER_BLOCK;
    const float ground = ground_sample_for_world(targetX, targetZ);

    if (!std::isfinite(ground)) return;

    const float finalY = std::max(targetY, ground + 20.0f);

    link->current.pos.x = targetX;
    link->current.pos.y = finalY;
    link->current.pos.z = targetZ;
    link->shape_angle.y = static_cast<s16>((mc.yaw / 180.0f) * 32768.0f);

    if (g_havePrevMc && mc.attacks != g_lastAttacks) {
        // TODO(fase 4): disparar ataque do Link / dano em atores.
    }
    g_lastAttacks = mc.attacks;
    g_havePrevMc = true;
}

void on_link_pos_move_post(ModContext*, void* args, void*, void*) {
    apply_minecraft(mods::arg<daAlink_c*>(args, 0));
}

}  // namespace

extern "C" {

MOD_EXPORT ModResult mod_initialize(ModError*) {
    if (!g_link.open(tpcraft::Link::default_path())) {
        svc_log->error(mod_ctx, "TPCraft: nao consegui abrir o link.bin");
        return MOD_ERROR;
    }
    const ModResult hookResult = mods::hook::add_post<LinkExecute>(on_link_pos_move_post);
    if (hookResult != MOD_OK) {
        g_link.close();
        svc_log->error(mod_ctx, "TPCraft: nao consegui hookar execute do Link");
        return MOD_ERROR;
    }
    svc_log->info(mod_ctx, "TPCraft: hook execute do Link instalado");
    svc_log->info(mod_ctx, "TPCraft: link aberto");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    publish_game();
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    g_link.close();
    svc_log->info(mod_ctx, "TPCraft: encerrado");
    return MOD_OK;
}

}  // extern "C"
