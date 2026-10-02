package dev.tpcraft;

import dev.tpcraft.link.GameState;
import dev.tpcraft.link.InputEvent;
import dev.tpcraft.link.McState;
import dev.tpcraft.link.Proto;
import dev.tpcraft.link.SharedLink;
import com.mojang.blaze3d.platform.InputConstants;
import net.fabricmc.api.ClientModInitializer;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientTickEvents;
import net.minecraft.client.Minecraft;
import net.minecraft.client.input.CharacterEvent;
import net.minecraft.client.input.KeyEvent;
import net.minecraft.client.input.MouseButtonInfo;

import java.io.IOException;
import java.util.HashSet;
import java.util.Set;

/** Entrypoint do lado Minecraft. Roda escondido: -Dtpcraft.startHidden=true (a implementar). */
public final class TPCraftClient implements ClientModInitializer {
    public static final String DEFAULT_WORLD_NAME = "DuskCraft";
    public static final boolean DEFAULT_FLAT_WORLD = true;
    public static final boolean DEFAULT_PEACEFUL = true;
    public static final boolean DEFAULT_NO_MOBS = true;

    private SharedLink link;
    private final GameState game = new GameState();
    private final McState mc = new McState();
    private final InputEvent input = new InputEvent();
    private final Set<Integer> pressedScancodes = new HashSet<>();
    private final Set<Integer> pressedMouseButtons = new HashSet<>();
    private double virtualMouseX;
    private double virtualMouseY;
    private boolean virtualMouseInitialized;
    private boolean hostWasAlive;

    @Override
    public void onInitializeClient() {
        applyDefaultTestWorldSettings();
        try {
            link = SharedLink.open(SharedLink.defaultPath());
        } catch (IOException e) {
            System.err.println("[TPCraft] nao consegui abrir o link: " + e);
            return;
        }
        ClientTickEvents.END_CLIENT_TICK.register(client -> {
            if (link == null) return;
            if (!link.gameReady() || !link.gameAlive() || !link.readGame(game)) {
                if (hostWasAlive) releaseAll(client);
                hostWasAlive = false;
                link.setHeaderFlag(Proto.HDR_MC_DRIVES, false);
                return;
            }
            hostWasAlive = true;
            pumpHostInput(client);

            var player = client.player;
            if (player == null || client.level == null) {
                link.setHeaderFlag(Proto.HDR_MC_DRIVES, false);
                return;
            }

            // Nao publica posicao zero/default. Se o jogador ainda nao nasceu ou nao tem mundo valido,
            // o link nao deve teletransportar para o abismo.
            double x = player.getX();
            double y = player.getY();
            double z = player.getZ();
            if (!Double.isFinite(x) || !Double.isFinite(y) || !Double.isFinite(z)) {
                link.setHeaderFlag(Proto.HDR_MC_DRIVES, false);
                return;
            }

            // TODO(minecraft): 1) se game.has(GS_CUTSCENE|GS_RIDING|GS_LOADING) -> nao controlar o Link;
            //                  2) teleportar/ancorar o jogador do MC em game.pos / UNITS_PER_BLOCK;
            //                  3) montar colisao a partir de game.heights (grid 33x33, NaN = sem chao);
            //                  4) game.timeOfDay -> hora do mundo do MC.

            mc.flags = 0;
            if (player.onGround()) mc.flags |= Proto.MF_ON_GROUND;
            if (player.isShiftKeyDown()) mc.flags |= Proto.MF_SNEAK;
            if (player.isSprinting()) mc.flags |= Proto.MF_SPRINT;
            if (player.isSwimming()) mc.flags |= Proto.MF_SWIMMING;

            mc.pos[0] = (float) (x / Proto.UNITS_PER_BLOCK);
            mc.pos[1] = (float) (y / Proto.UNITS_PER_BLOCK);
            mc.pos[2] = (float) (z / Proto.UNITS_PER_BLOCK);
            mc.yaw = player.getYRot();
            mc.pitch = player.getXRot();
            var vel = player.getDeltaMovement();
            mc.vel[0] = (float) vel.x;
            mc.vel[1] = (float) vel.y;
            mc.vel[2] = (float) vel.z;
            mc.health = player.getHealth();
            mc.hotbar = 0;
            mc.held = 0;
            mc.attacks = 0;

            link.publishMc(mc);
            link.setHeaderFlag(Proto.HDR_MC_DRIVES, true);
        });
    }

    private void pumpHostInput(Minecraft client) {
        while (link.pollInput(input)) {
            switch (Short.toUnsignedInt(input.type)) {
                case Proto.IN_KEY -> sendKey(client, Short.toUnsignedInt(input.code),
                    input.a != 0 ? InputConstants.PRESS : InputConstants.RELEASE);
                case Proto.IN_MOUSE_BUTTON -> sendMouseButton(client, Short.toUnsignedInt(input.code),
                    input.a != 0 ? InputConstants.PRESS : InputConstants.RELEASE);
                case Proto.IN_SCROLL -> client.mouseHandler.onScroll(client.getWindow().handle(), 0.0,
                        input.a / 120.0);
                case Proto.IN_LOOK -> moveMouse(client, input.a, input.b);
                case Proto.IN_TEXT -> client.keyboardHandler.charTyped(client.getWindow().handle(),
                    new CharacterEvent(input.a));
                case Proto.IN_RELEASE_ALL -> releaseAll(client);
                case Proto.IN_OPEN_MENU -> {
                    sendKey(client, 0, InputConstants.PRESS, InputConstants.KEY_ESCAPE);
                    sendKey(client, 0, InputConstants.RELEASE, InputConstants.KEY_ESCAPE);
                }
                default -> { }
            }
        }
    }

    private void moveMouse(Minecraft client, int dx, int dy) {
        if (!virtualMouseInitialized) {
            virtualMouseX = client.getWindow().getWidth() / 2.0;
            virtualMouseY = client.getWindow().getHeight() / 2.0;
            virtualMouseInitialized = true;
        }
        virtualMouseX += dx;
        virtualMouseY += dy;
        client.mouseHandler.onMove(client.getWindow().handle(), virtualMouseX, virtualMouseY, dx, dy);
    }

    private void sendKey(Minecraft client, int scancode, int action) {
        sendKey(client, scancode, action, keyValueForHid(scancode));
    }

    private void sendKey(Minecraft client, int scancode, int action, int keycode) {
        if (keycode < 0) return;
        if (action == InputConstants.PRESS) pressedScancodes.add(scancode);
        else pressedScancodes.remove(scancode);
        client.keyboardHandler.keyPress(client.getWindow().handle(), action,
                new KeyEvent(keycode, scancode, 0));
    }

    private int keyValueForHid(int usage) {
        if (usage >= 4 && usage <= 29) return InputConstants.KEY_A + usage - 4;
        if (usage >= 30 && usage <= 38) return InputConstants.KEY_1 + usage - 30;
        if (usage >= 58 && usage <= 69) return InputConstants.KEY_F1 + usage - 58;
        return switch (usage) {
            case 39 -> InputConstants.KEY_0;
            case 40 -> InputConstants.KEY_RETURN;
            case 41 -> InputConstants.KEY_ESCAPE;
            case 42 -> InputConstants.KEY_BACKSPACE;
            case 43 -> InputConstants.KEY_TAB;
            case 44 -> InputConstants.KEY_SPACE;
            case 45 -> InputConstants.KEY_MINUS;
            case 46 -> InputConstants.KEY_EQUALS;
            case 47 -> InputConstants.KEY_LBRACKET;
            case 48 -> InputConstants.KEY_RBRACKET;
            case 49 -> InputConstants.KEY_BACKSLASH;
            case 51 -> InputConstants.KEY_SEMICOLON;
            case 52 -> InputConstants.KEY_APOSTROPHE;
            case 53 -> InputConstants.KEY_GRAVE;
            case 54 -> InputConstants.KEY_COMMA;
            case 55 -> InputConstants.KEY_PERIOD;
            case 56 -> InputConstants.KEY_SLASH;
            case 57 -> InputConstants.KEY_CAPSLOCK;
            case 73 -> InputConstants.KEY_INSERT;
            case 74 -> InputConstants.KEY_HOME;
            case 75 -> InputConstants.KEY_PAGEUP;
            case 76 -> InputConstants.KEY_DELETE;
            case 77 -> InputConstants.KEY_END;
            case 78 -> InputConstants.KEY_PAGEDOWN;
            case 79 -> InputConstants.KEY_RIGHT;
            case 80 -> InputConstants.KEY_LEFT;
            case 81 -> InputConstants.KEY_DOWN;
            case 82 -> InputConstants.KEY_UP;
            case 224 -> InputConstants.KEY_LCONTROL;
            case 225 -> InputConstants.KEY_LSHIFT;
            case 226 -> InputConstants.KEY_LALT;
            case 228 -> InputConstants.KEY_RCONTROL;
            case 229 -> InputConstants.KEY_RSHIFT;
            case 230 -> InputConstants.KEY_RALT;
            default -> -1;
        };
    }

    private void sendMouseButton(Minecraft client, int sdlButton, int action) {
        int button = switch (sdlButton) {
            case 1 -> InputConstants.MOUSE_BUTTON_LEFT;
            case 2 -> InputConstants.MOUSE_BUTTON_MIDDLE;
            case 3 -> InputConstants.MOUSE_BUTTON_RIGHT;
            case 4 -> InputConstants.MOUSE_BUTTON_4;
            case 5 -> InputConstants.MOUSE_BUTTON_5;
            default -> -1;
        };
        if (button < 0) return;
        if (action == InputConstants.PRESS) pressedMouseButtons.add(button);
        else pressedMouseButtons.remove(button);
        client.mouseHandler.onButton(client.getWindow().handle(), new MouseButtonInfo(button, 0), action);
    }

    private void releaseAll(Minecraft client) {
        for (int scancode : pressedScancodes.toArray(Integer[]::new)) {
            sendKey(client, scancode, InputConstants.RELEASE);
        }
        for (int button : pressedMouseButtons.toArray(Integer[]::new)) {
            sendMouseButton(client, switch (button) {
                case InputConstants.MOUSE_BUTTON_LEFT -> 1;
                case InputConstants.MOUSE_BUTTON_MIDDLE -> 2;
                case InputConstants.MOUSE_BUTTON_RIGHT -> 3;
                case InputConstants.MOUSE_BUTTON_4 -> 4;
                case InputConstants.MOUSE_BUTTON_5 -> 5;
                default -> 0;
            }, InputConstants.RELEASE);
        }
    }

    private void applyDefaultTestWorldSettings() {
        System.setProperty("tpcraft.world.name", DEFAULT_WORLD_NAME);
        System.setProperty("tpcraft.flat.world", Boolean.toString(DEFAULT_FLAT_WORLD));
        System.setProperty("tpcraft.peaceful", Boolean.toString(DEFAULT_PEACEFUL));
        System.setProperty("tpcraft.no.mobs", Boolean.toString(DEFAULT_NO_MOBS));
        System.out.println("[TPCraft] default test world: " + DEFAULT_WORLD_NAME
                + " (flat=" + DEFAULT_FLAT_WORLD
                + ", peaceful=" + DEFAULT_PEACEFUL
                + ", noMobs=" + DEFAULT_NO_MOBS + ")");
    }

}
