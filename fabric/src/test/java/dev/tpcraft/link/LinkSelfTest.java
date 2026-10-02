package dev.tpcraft.link;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Path;
import java.nio.charset.StandardCharsets;

/** Teste de integracao (sem JUnit): java LinkSelfTest read|write <path> */
public final class LinkSelfTest {
    public static void main(String[] a) throws Exception {
        try (SharedLink l = SharedLink.open(Path.of(a[1]))) {
            if (a[0].equals("read")) {
                GameState g = new GameState();
                if (!l.readGame(g)) { System.out.println("read failed"); System.exit(1); }
                System.out.printf("stage=%08x pos=%.2f,%.2f,%.2f yaw=%.1f state=%d time=%.1f grid=%.0f,%.0f h0=%.0f h1088=%.0f nan5=%b%n",
                        g.stageHash, g.pos[0], g.pos[1], g.pos[2], g.yaw, g.state, g.timeOfDay,
                        g.gridCx, g.gridCz, g.heights[0], g.heights[1088], Float.isNaN(g.heights[5]));
            } else if (a[0].equals("input-read")) {
                InputEvent event = new InputEvent();
                if (!l.pollInput(event)) { System.out.println("input read failed"); System.exit(1); }
                System.out.printf("type=%d code=%d a=%d b=%d c=%d%n",
                        Short.toUnsignedInt(event.type), Short.toUnsignedInt(event.code),
                        event.a, event.b, event.c);
            } else if (a[0].equals("render-write")) {
                ByteBuffer payload = ByteBuffer.wrap("TPCraft-render-ring-v3".getBytes(StandardCharsets.UTF_8))
                        .order(ByteOrder.LITTLE_ENDIAN);
                if (!l.writeRender(Proto.REN_SECTION, payload)) {
                    System.out.println("render write failed");
                    System.exit(1);
                }
                System.out.println("render written");
            } else {
                McState m = new McState();
                m.flags = Proto.MF_ON_GROUND | Proto.MF_SPRINT;
                m.pos[0] = 12.5f; m.pos[1] = 64f; m.pos[2] = -3f; m.yaw = 90f; m.health = 14f;
                m.hotbar = 3; m.held = 777; m.attacks = 5;
                l.publishMc(m);
                System.out.println("written");
            }
        }
    }
}
