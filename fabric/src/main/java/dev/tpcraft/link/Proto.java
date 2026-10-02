package dev.tpcraft.link;

/** Espelho de protocol/tpcraft_protocol.h (v3). */
public final class Proto {
    private Proto() {}

    public static final int MAGIC = 0x52435054;
        public static final int VERSION = 3;
        public static final int RENDER_OFF = 16384;
        public static final int RENDER_BYTES = 64 * 1024 * 1024;
        public static final int FILE_SIZE = RENDER_OFF + RENDER_BYTES;
    public static final int GRID_DIM = 33;

    // header
    public static final int OFF_MAGIC = 0, OFF_VERSION = 4, OFF_FLAGS = 8, OFF_GAME_FRAME = 16, OFF_MC_TICK = 24;
        public static final int OFF_GAME_HEARTBEAT_MS = 32, OFF_MC_HEARTBEAT_MS = 40;
    public static final int HDR_GAME_READY = 1, HDR_MC_READY = 2, HDR_MC_DRIVES = 4;

    // GameBlock (jogo -> MC)
    public static final int GAME_OFF = 64;
    public static final int G_SEQ = 0, G_STAGE_HASH = 4, G_POS = 8, G_YAW = 20, G_VEL = 24, G_STATE = 36,
            G_TIME = 40, G_GRID_CX = 44, G_GRID_CZ = 48, G_GRID_CELL = 52, G_GRID_DIM = 56, G_HEIGHTS = 64;
    public static final int GS_CUTSCENE = 1, GS_RIDING = 2, GS_WOLF = 4, GS_SWIMMING = 8, GS_LOADING = 16;

    // McBlock (MC -> jogo)
    public static final int MC_OFF = 8256;
    public static final int M_SEQ = 0, M_FLAGS = 4, M_POS = 8, M_YAW = 20, M_PITCH = 24, M_VEL = 28,
            M_HEALTH = 40, M_HOTBAR = 44, M_HELD = 48, M_ATTACKS = 52;
    public static final int MF_ON_GROUND = 1, MF_SNEAK = 2, MF_SPRINT = 4, MF_SWIMMING = 8;

    // Host -> guest single-producer/single-consumer input ring.
    public static final int INPUT_OFF = 8384, INPUT_HEAD = 0, INPUT_TAIL = 64, INPUT_DATA = 128;
    public static final int INPUT_COUNT = 256, INPUT_ENTRY_SIZE = 16;
    public static final int IN_KEY = 1, IN_MOUSE_BUTTON = 2, IN_SCROLL = 3,
            IN_TEXT = 4, IN_RELEASE_ALL = 5, IN_OPEN_MENU = 6, IN_LOOK = 7;

        // Minecraft -> Dusklight byte render ring.
        public static final int RENDER_HEAD = 0, RENDER_TAIL = 64, RENDER_DATA = 128;
        public static final int RENDER_DATA_BYTES = RENDER_BYTES - RENDER_DATA;
        public static final int REN_PAD = 0, REN_CLEAR_ALL = 1, REN_ATLAS = 2,
            REN_SECTION = 3, REN_ATLAS_REGION = 4, REN_TEXTURE = 5, REN_AVATAR = 6, REN_SCENE = 7;
        public static final int RENDER_VERTEX_SIZE = 32;

    public static final float UNITS_PER_BLOCK = 100.0f; // provisorio, igual ao header C++
}
