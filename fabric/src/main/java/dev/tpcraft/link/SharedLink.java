package dev.tpcraft.link;

import java.io.IOException;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.VarHandle;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;

/** Lado Minecraft da ponte: mapeia o mesmo arquivo que o mod do Dusklight. */
public final class SharedLink implements AutoCloseable {
    private static final VarHandle INT_VH =
            MethodHandles.byteBufferViewVarHandle(int[].class, ByteOrder.LITTLE_ENDIAN);
        private static final VarHandle LONG_VH =
            MethodHandles.byteBufferViewVarHandle(long[].class, ByteOrder.LITTLE_ENDIAN);

    private final FileChannel channel;
    private final MappedByteBuffer buf;

    private SharedLink(FileChannel ch, MappedByteBuffer b) { this.channel = ch; this.buf = b; }

    public static Path defaultPath() {
        String local = System.getenv("LOCALAPPDATA");
        Path root = local != null ? Path.of(local)
                : Path.of(System.getProperty("user.home"), ".local", "share");
        return root.resolve("TPCraft").resolve("link.bin");
    }

    public static SharedLink open(Path path) throws IOException {
        Files.createDirectories(path.getParent());
        FileChannel ch = FileChannel.open(path, StandardOpenOption.CREATE, StandardOpenOption.READ,
                StandardOpenOption.WRITE);
        MappedByteBuffer b = ch.map(FileChannel.MapMode.READ_WRITE, 0, Proto.FILE_SIZE);
        b.order(ByteOrder.LITTLE_ENDIAN);
        if (b.getInt(Proto.OFF_MAGIC) != Proto.MAGIC || b.getInt(Proto.OFF_VERSION) != Proto.VERSION) {
            for (int i = 0; i < Proto.FILE_SIZE; i += 4) b.putInt(i, 0);
            b.putInt(Proto.OFF_MAGIC, Proto.MAGIC);
            b.putInt(Proto.OFF_VERSION, Proto.VERSION);
        }
        SharedLink l = new SharedLink(ch, b);
        l.heartbeatMc();
        l.setHeaderFlag(Proto.HDR_MC_READY, true);
        return l;
    }

    public int headerFlags() { return buf.getInt(Proto.OFF_FLAGS); }
    public boolean gameReady() { return (headerFlags() & Proto.HDR_GAME_READY) != 0; }
    public boolean gameAlive() {
        long heartbeat = buf.getLong(Proto.OFF_GAME_HEARTBEAT_MS);
        long age = System.currentTimeMillis() - heartbeat;
        return heartbeat != 0 && age >= 0 && age <= 3000;
    }

    public void setHeaderFlag(int mask, boolean on) {
        while (true) {
            int current = (int) INT_VH.getVolatile(buf, Proto.OFF_FLAGS);
            int updated = on ? (current | mask) : (current & ~mask);
            if (INT_VH.compareAndSet(buf, Proto.OFF_FLAGS, current, updated)) return;
        }
    }

    public long heartbeatGameMillis() { return buf.getLong(Proto.OFF_GAME_HEARTBEAT_MS); }

    private void heartbeatMc() {
        buf.putLong(Proto.OFF_MC_HEARTBEAT_MS, System.currentTimeMillis());
    }

    /** Leitura estavel (seqlock) do bloco jogo->MC. false = instavel/ainda sem dados. */
    public boolean readGame(GameState out) {
        int base = Proto.GAME_OFF;
        for (int attempt = 0; attempt < 4; attempt++) {
            int s1 = (int) INT_VH.getVolatile(buf, base + Proto.G_SEQ);
            if ((s1 & 1) != 0) continue;
            out.stageHash = buf.getInt(base + Proto.G_STAGE_HASH);
            for (int i = 0; i < 3; i++) out.pos[i] = buf.getFloat(base + Proto.G_POS + 4 * i);
            out.yaw = buf.getFloat(base + Proto.G_YAW);
            for (int i = 0; i < 3; i++) out.vel[i] = buf.getFloat(base + Proto.G_VEL + 4 * i);
            out.state = buf.getInt(base + Proto.G_STATE);
            out.timeOfDay = buf.getFloat(base + Proto.G_TIME);
            out.gridCx = buf.getFloat(base + Proto.G_GRID_CX);
            out.gridCz = buf.getFloat(base + Proto.G_GRID_CZ);
            out.gridCell = buf.getFloat(base + Proto.G_GRID_CELL);
            for (int i = 0; i < out.heights.length; i++)
                out.heights[i] = buf.getFloat(base + Proto.G_HEIGHTS + 4 * i);
            VarHandle.acquireFence();
            int s2 = (int) INT_VH.getVolatile(buf, base + Proto.G_SEQ);
            if (s1 == s2) {
                heartbeatMc();
                return s1 != 0;
            }
        }
        return false;
    }

    /** Publica o estado do Minecraft (chamar 1x por tick). */
    public void publishMc(McState s) {
        int base = Proto.MC_OFF;
        int seq = (int) INT_VH.getVolatile(buf, base + Proto.M_SEQ);
        INT_VH.setRelease(buf, base + Proto.M_SEQ, seq + 1);
        buf.putInt(base + Proto.M_FLAGS, s.flags);
        for (int i = 0; i < 3; i++) buf.putFloat(base + Proto.M_POS + 4 * i, s.pos[i]);
        buf.putFloat(base + Proto.M_YAW, s.yaw);
        buf.putFloat(base + Proto.M_PITCH, s.pitch);
        for (int i = 0; i < 3; i++) buf.putFloat(base + Proto.M_VEL + 4 * i, s.vel[i]);
        buf.putFloat(base + Proto.M_HEALTH, s.health);
        buf.putInt(base + Proto.M_HOTBAR, s.hotbar);
        buf.putInt(base + Proto.M_HELD, s.held);
        buf.putInt(base + Proto.M_ATTACKS, s.attacks);
        INT_VH.setRelease(buf, base + Proto.M_SEQ, seq + 2);
        buf.putLong(Proto.OFF_MC_TICK, buf.getLong(Proto.OFF_MC_TICK) + 1);
        heartbeatMc();
    }

    /** Reads one host input event. Returns false when the ring is empty. */
    public boolean pollInput(InputEvent out) {
        int base = Proto.INPUT_OFF;
        long tail = (long) LONG_VH.getVolatile(buf, base + Proto.INPUT_TAIL);
        long head = (long) LONG_VH.getAcquire(buf, base + Proto.INPUT_HEAD);
        if (tail >= head) return false;

        int entry = base + Proto.INPUT_DATA
                + (int) (tail & (Proto.INPUT_COUNT - 1)) * Proto.INPUT_ENTRY_SIZE;
        out.type = buf.getShort(entry);
        out.code = buf.getShort(entry + 2);
        out.a = buf.getInt(entry + 4);
        out.b = buf.getInt(entry + 8);
        out.c = buf.getInt(entry + 12);
        LONG_VH.setRelease(buf, base + Proto.INPUT_TAIL, tail + 1);
        return true;
    }

    /** Writes one Minecraft render update. False means Dusklight must catch up before retrying. */
    public boolean writeRender(int type, ByteBuffer payload) {
        int ring = Proto.RENDER_OFF;
        long head = (long) LONG_VH.getVolatile(buf, ring + Proto.RENDER_HEAD);
        long tail = (long) LONG_VH.getAcquire(buf, ring + Proto.RENDER_TAIL);
        int payloadBytes = payload.remaining();
        long messageBytes = (8L + payloadBytes + 7L) & ~7L;
        long position = head % Proto.RENDER_DATA_BYTES;
        long padding = position + messageBytes > Proto.RENDER_DATA_BYTES
                ? Proto.RENDER_DATA_BYTES - position : 0;
        if (messageBytes > Proto.RENDER_DATA_BYTES || head - tail + messageBytes + padding
                > Proto.RENDER_DATA_BYTES) return false;

        if (padding != 0) {
            int padAt = ring + Proto.RENDER_DATA + (int) position;
            buf.putInt(padAt, Proto.REN_PAD);
            buf.putInt(padAt + 4, 0);
            head += padding;
            position = 0;
        }

        int entry = ring + Proto.RENDER_DATA + (int) position;
        buf.putInt(entry, type);
        buf.putInt(entry + 4, payloadBytes);
        ByteBuffer destination = buf.duplicate().order(ByteOrder.LITTLE_ENDIAN);
        destination.position(entry + 8);
        destination.put(payload.duplicate());
        LONG_VH.setRelease(buf, ring + Proto.RENDER_HEAD, head + messageBytes);
        heartbeatMc();
        return true;
    }

    @Override public void close() throws IOException {
        buf.putLong(Proto.OFF_MC_HEARTBEAT_MS, 0);
        setHeaderFlag(Proto.HDR_MC_READY, false);
        buf.force();
        channel.close();
    }
}
