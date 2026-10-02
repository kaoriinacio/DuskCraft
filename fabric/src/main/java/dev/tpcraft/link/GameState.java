package dev.tpcraft.link;

/** O que o jogo (Dusklight) publica a cada frame. */
public final class GameState {
    public int stageHash;
    public final float[] pos = new float[3];
    public float yaw;
    public final float[] vel = new float[3];
    public int state;
    public float timeOfDay;
    public float gridCx, gridCz, gridCell;
    public final float[] heights = new float[Proto.GRID_DIM * Proto.GRID_DIM];

    public boolean has(int flag) { return (state & flag) != 0; }
}
