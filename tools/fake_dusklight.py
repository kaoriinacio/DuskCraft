#!/usr/bin/env python3
"""Stand-in do Dusklight para testar o lado Minecraft sem abrir o jogo.

  python tools/fake_dusklight.py run              # simula um Link andando em circulos e imprime o que o MC manda
  python tools/fake_dusklight.py write            # escreve 1 GameState conhecido (teste)
  python tools/fake_dusklight.py read             # le o McBlock atual (teste)
  --path CAMINHO                                  # arquivo de link (padrao igual ao do jogo/MC)

Offsets espelham protocol/tpcraft_protocol.h (v3).
"""
import argparse, math, mmap, os, struct, sys, time
from pathlib import Path

MAGIC, VERSION, GRID_DIM = 0x52435054, 3, 33
OFF_MAGIC, OFF_VERSION, OFF_FLAGS, OFF_GAME_FRAME, OFF_MC_TICK = 0, 4, 8, 16, 24
OFF_GAME_HEARTBEAT_MS, OFF_MC_HEARTBEAT_MS = 32, 40
HDR_GAME_READY, HDR_MC_READY, HDR_MC_DRIVES = 1, 2, 4
GAME_OFF, MC_OFF = 64, 8256
INPUT_OFF, INPUT_HEAD, INPUT_TAIL, INPUT_DATA = 8384, 0, 64, 128
INPUT_COUNT, INPUT_ENTRY_SIZE = 256, 16
IN_KEY, IN_MOUSE_BUTTON, IN_SCROLL, IN_TEXT, IN_RELEASE_ALL, IN_OPEN_MENU, IN_LOOK = 1, 2, 3, 4, 5, 6, 7
RENDER_OFF, RENDER_BYTES = 16384, 64 * 1024 * 1024
RENDER_HEAD, RENDER_TAIL, RENDER_DATA = 0, 64, 128
RENDER_DATA_BYTES = RENDER_BYTES - RENDER_DATA
REN_PAD, REN_CLEAR_ALL, REN_ATLAS, REN_SECTION = 0, 1, 2, 3
REN_ATLAS_REGION, REN_TEXTURE, REN_AVATAR, REN_SCENE = 4, 5, 6, 7
FILE_SIZE = RENDER_OFF + RENDER_BYTES
G = dict(SEQ=0, STAGE=4, POS=8, YAW=20, VEL=24, STATE=36, TIME=40, CX=44, CZ=48, CELL=52, DIM=56, H=64)
M = dict(SEQ=0, FLAGS=4, POS=8, YAW=20, PITCH=24, VEL=28, HEALTH=40, HOTBAR=44, HELD=48, ATTACKS=52)
GS_SWIMMING = 8


def default_path() -> Path:
    local = os.environ.get("LOCALAPPDATA")
    root = Path(local) if local else Path.home() / ".local" / "share"
    return root / "TPCraft" / "link.bin"


class Link:
    def __init__(self, path: Path):
        path.parent.mkdir(parents=True, exist_ok=True)
        self.f = open(path, "r+b") if path.exists() else open(path, "w+b")
        self.f.truncate(FILE_SIZE)
        self.m = mmap.mmap(self.f.fileno(), FILE_SIZE)
        if struct.unpack_from("<II", self.m, 0) != (MAGIC, VERSION):
            self.m[:] = bytes(FILE_SIZE)
            struct.pack_into("<II", self.m, 0, MAGIC, VERSION)
        struct.pack_into("<Q", self.m, OFF_GAME_HEARTBEAT_MS, int(time.time() * 1000))
        self.set_flag(HDR_GAME_READY, True)

    def flags(self): return struct.unpack_from("<I", self.m, OFF_FLAGS)[0]

    def set_flag(self, mask, on):
        f = self.flags()
        struct.pack_into("<I", self.m, OFF_FLAGS, (f | mask) if on else (f & ~mask))

    def publish_game(self, stage, pos, yaw, vel, state, tod, cx, cz, cell, heights):
        b = GAME_OFF
        seq = struct.unpack_from("<I", self.m, b + G["SEQ"])[0]
        struct.pack_into("<I", self.m, b + G["SEQ"], (seq + 1) & 0xFFFFFFFF)
        struct.pack_into("<I", self.m, b + G["STAGE"], stage)
        struct.pack_into("<3f", self.m, b + G["POS"], *pos)
        struct.pack_into("<f", self.m, b + G["YAW"], yaw)
        struct.pack_into("<3f", self.m, b + G["VEL"], *vel)
        struct.pack_into("<I", self.m, b + G["STATE"], state)
        struct.pack_into("<f", self.m, b + G["TIME"], tod)
        struct.pack_into("<3f", self.m, b + G["CX"], cx, cz, cell)
        struct.pack_into("<I", self.m, b + G["DIM"], GRID_DIM)
        struct.pack_into(f"<{GRID_DIM * GRID_DIM}f", self.m, b + G["H"], *heights)
        struct.pack_into("<I", self.m, b + G["SEQ"], (seq + 2) & 0xFFFFFFFF)
        fr = struct.unpack_from("<Q", self.m, OFF_GAME_FRAME)[0]
        struct.pack_into("<Q", self.m, OFF_GAME_FRAME, fr + 1)
        struct.pack_into("<Q", self.m, OFF_GAME_HEARTBEAT_MS, int(time.time() * 1000))

    def push_input(self, event_type, code, a=0, b=0, c=0):
        ring = INPUT_OFF
        head = struct.unpack_from("<Q", self.m, ring + INPUT_HEAD)[0]
        tail = struct.unpack_from("<Q", self.m, ring + INPUT_TAIL)[0]
        if head - tail >= INPUT_COUNT:
            return False
        offset = ring + INPUT_DATA + (head & (INPUT_COUNT - 1)) * INPUT_ENTRY_SIZE
        struct.pack_into("<HHiii", self.m, offset, event_type, code, a, b, c)
        struct.pack_into("<Q", self.m, ring + INPUT_HEAD, head + 1)
        return True

    def pop_input(self):
        ring = INPUT_OFF
        tail = struct.unpack_from("<Q", self.m, ring + INPUT_TAIL)[0]
        head = struct.unpack_from("<Q", self.m, ring + INPUT_HEAD)[0]
        if tail >= head:
            return None
        offset = ring + INPUT_DATA + (tail & (INPUT_COUNT - 1)) * INPUT_ENTRY_SIZE
        event = struct.unpack_from("<HHiii", self.m, offset)
        struct.pack_into("<Q", self.m, ring + INPUT_TAIL, tail + 1)
        return event

    def write_render(self, message_type, payload):
        ring = RENDER_OFF
        head = struct.unpack_from("<Q", self.m, ring + RENDER_HEAD)[0]
        tail = struct.unpack_from("<Q", self.m, ring + RENDER_TAIL)[0]
        message_bytes = (8 + len(payload) + 7) & ~7
        position = head % RENDER_DATA_BYTES
        padding = RENDER_DATA_BYTES - position if position + message_bytes > RENDER_DATA_BYTES else 0
        if message_bytes > RENDER_DATA_BYTES or head - tail + message_bytes + padding > RENDER_DATA_BYTES:
            return False
        if padding:
            offset = ring + RENDER_DATA + position
            struct.pack_into("<II", self.m, offset, REN_PAD, 0)
            head += padding
            position = 0
        offset = ring + RENDER_DATA + position
        struct.pack_into("<II", self.m, offset, message_type, len(payload))
        self.m[offset + 8:offset + 8 + len(payload)] = payload
        struct.pack_into("<Q", self.m, ring + RENDER_HEAD, head + message_bytes)
        return True

    def pop_render(self):
        ring = RENDER_OFF
        while True:
            tail = struct.unpack_from("<Q", self.m, ring + RENDER_TAIL)[0]
            head = struct.unpack_from("<Q", self.m, ring + RENDER_HEAD)[0]
            if tail >= head:
                return None
            position = tail % RENDER_DATA_BYTES
            offset = ring + RENDER_DATA + position
            message_type, payload_size = struct.unpack_from("<II", self.m, offset)
            if message_type == REN_PAD:
                struct.pack_into("<Q", self.m, ring + RENDER_TAIL,
                                 tail + RENDER_DATA_BYTES - position)
                continue
            message_bytes = (8 + payload_size + 7) & ~7
            if message_bytes > head - tail:
                return None
            payload = self.m[offset + 8:offset + 8 + payload_size]
            struct.pack_into("<Q", self.m, ring + RENDER_TAIL, tail + message_bytes)
            return message_type, payload

    def read_mc(self):
        b = MC_OFF
        for _ in range(4):
            s1 = struct.unpack_from("<I", self.m, b + M["SEQ"])[0]
            if s1 & 1:
                continue
            d = dict(
                flags=struct.unpack_from("<I", self.m, b + M["FLAGS"])[0],
                pos=struct.unpack_from("<3f", self.m, b + M["POS"]),
                yaw=struct.unpack_from("<f", self.m, b + M["YAW"])[0],
                pitch=struct.unpack_from("<f", self.m, b + M["PITCH"])[0],
                vel=struct.unpack_from("<3f", self.m, b + M["VEL"]),
                health=struct.unpack_from("<f", self.m, b + M["HEALTH"])[0],
                hotbar=struct.unpack_from("<I", self.m, b + M["HOTBAR"])[0],
                held=struct.unpack_from("<I", self.m, b + M["HELD"])[0],
                attacks=struct.unpack_from("<I", self.m, b + M["ATTACKS"])[0],
            )
            if struct.unpack_from("<I", self.m, b + M["SEQ"])[0] == s1:
                return d if s1 else None
        return None

    def close(self):
        struct.pack_into("<Q", self.m, OFF_GAME_HEARTBEAT_MS, 0)
        self.set_flag(HDR_GAME_READY, False)
        self.m.flush(); self.m.close(); self.f.close()


def flat_grid(y=0.0): return [y] * (GRID_DIM * GRID_DIM)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("mode", choices=["run", "write", "read", "input-write", "input-read",
                                     "render-prime", "render-write", "render-read"])
    ap.add_argument("--path", type=Path, default=default_path())
    a = ap.parse_args()
    link = Link(a.path)
    try:
        if a.mode == "write":
            h = [float(i) for i in range(GRID_DIM * GRID_DIM)]
            h[5] = float("nan")
            link.publish_game(0xABCD1234, (1.5, -2.25, 300.0), 0.5, (0, 0, 0), GS_SWIMMING, 18.5, 10.0, 20.0, 100.0, h)
            print("written")
        elif a.mode == "read":
            d = link.read_mc()
            if not d: print("read failed"); sys.exit(1)
            print(d)
        elif a.mode == "input-write":
            if not link.push_input(IN_KEY, 30, 1): raise SystemExit("input ring full")
            print("input written")
        elif a.mode == "input-read":
            event = link.pop_input()
            if event is None: raise SystemExit("input ring empty")
            print(event)
        elif a.mode == "render-prime":
            start = RENDER_DATA_BYTES - 16
            struct.pack_into("<QQ", link.m, RENDER_OFF + RENDER_HEAD, start, start)
            print("render ring primed near wrap")
        elif a.mode == "render-write":
            payload = b"TPCraft-render-ring-v3"
            if not link.write_render(REN_SECTION, payload): raise SystemExit("render ring full")
            print("render message written")
        elif a.mode == "render-read":
            message = link.pop_render()
            if message is None: raise SystemExit("render ring empty")
            print(message)
        else:
            link.set_flag(HDR_MC_DRIVES, True)
            print(f"fake Dusklight em {a.path} -- Ctrl+C para sair")
            t0, last = time.time(), None
            while True:
                t = time.time() - t0
                pos = (math.cos(t) * 500, 0.0, math.sin(t) * 500)
                link.publish_game(0x1234, pos, t % 6.283, (0, 0, 0), 0, 12.0 + (t / 10) % 12, pos[0], pos[2], 100.0, flat_grid())
                mc = link.read_mc()
                if mc and mc != last:
                    print("MC ->", mc); last = mc
                time.sleep(1 / 30)
    except KeyboardInterrupt:
        pass
    finally:
        link.close()


if __name__ == "__main__":
    main()
