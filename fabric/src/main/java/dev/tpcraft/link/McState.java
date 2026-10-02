package dev.tpcraft.link;

/** O que o Minecraft publica a cada tick (o jogo aplica no Link). */
public final class McState {
    public int flags;
    public final float[] pos = new float[3];
    public float yaw, pitch;
    public final float[] vel = new float[3];
    public float health = 20f;
    public int hotbar, held, attacks;
}
