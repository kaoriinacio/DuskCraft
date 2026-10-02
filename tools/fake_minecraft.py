#!/usr/bin/env python3
"""Minecraft falso: faz o Link andar em circulo ao redor de onde ele esta (teste da fase 1).

  python tools/fake_minecraft.py                 # raio 3 blocos, ate Ctrl+C
  python tools/fake_minecraft.py --radius 2 --seconds 20

Precisa do Dusklight aberto, com o mod TPCraft, e o Link controlavel (dentro de um save).
Use num lugar plano e aberto. Ctrl+C devolve o controle ao jogo (o Link fica onde estiver).
NAO mexe na flag GAME_READY do jogo.
"""
import argparse, math, mmap, os, struct, time
from pathlib import Path

FILE_SIZE, MAGIC, VERSION = 16384 + 64 * 1024 * 1024, 0x52435054, 3
OFF_FLAGS, OFF_MC_TICK, OFF_MC_HEARTBEAT_MS = 8, 24, 40
HDR_MC_READY, HDR_MC_DRIVES = 2, 4
GAME_OFF, MC_OFF = 64, 8256
UNITS_PER_BLOCK = 100.0
MF_ON_GROUND = 1


def default_path() -> Path:
    local = os.environ.get("LOCALAPPDATA")
    return (Path(local) if local else Path.home() / ".local" / "share") / "TPCraft" / "link.bin"


def set_flag(m, mask, on):
    f = struct.unpack_from("<I", m, OFF_FLAGS)[0]
    struct.pack_into("<I", m, OFF_FLAGS, (f | mask) if on else (f & ~mask))


def read_game_pos(m):
    for _ in range(8):
        s1 = struct.unpack_from("<I", m, GAME_OFF)[0]
        if s1 == 0 or s1 & 1:
            continue
        pos = struct.unpack_from("<3f", m, GAME_OFF + 8)
        if struct.unpack_from("<I", m, GAME_OFF)[0] == s1:
            return pos
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--path", type=Path, default=default_path())
    ap.add_argument("--radius", type=float, default=3.0, help="raio em blocos")
    ap.add_argument("--seconds", type=float, default=0, help="0 = ate Ctrl+C")
    a = ap.parse_args()

    with open(a.path, "r+b") as f:
        m = mmap.mmap(f.fileno(), FILE_SIZE)
        if struct.unpack_from("<II", m, 0) != (MAGIC, VERSION):
            raise SystemExit("link.bin invalido: abra o Dusklight com o mod primeiro")
        pos = read_game_pos(m)
        if not pos:
            raise SystemExit("sem dados do jogo ainda: entre num save e tente de novo")
        cx, cy, cz = (c / UNITS_PER_BLOCK for c in pos)
        print(f"centro (blocos): {cx:.2f}, {cy:.2f}, {cz:.2f} -- Ctrl+C para parar")
        set_flag(m, HDR_MC_READY, True)
        set_flag(m, HDR_MC_DRIVES, True)
        t0 = time.time()
        try:
            while a.seconds <= 0 or time.time() - t0 < a.seconds:
                t = time.time() - t0
                ang = t * 0.6  # rad/s
                x, z = cx + a.radius * math.cos(ang), cz + a.radius * math.sin(ang)
                yaw = max(-179.0, min(179.0, ((math.degrees(-ang) + 180) % 360) - 180))
                seq = struct.unpack_from("<I", m, MC_OFF)[0]
                struct.pack_into("<I", m, MC_OFF, (seq + 1) & 0xFFFFFFFF)       # impar: escrevendo
                struct.pack_into("<I", m, MC_OFF + 4, MF_ON_GROUND)
                struct.pack_into("<3f", m, MC_OFF + 8, x, cy, z)
                struct.pack_into("<2f", m, MC_OFF + 20, yaw, 0.0)
                struct.pack_into("<3f", m, MC_OFF + 28, 0.0, 0.0, 0.0)
                struct.pack_into("<f", m, MC_OFF + 40, 20.0)
                struct.pack_into("<III", m, MC_OFF + 44, 0, 0, 0)
                struct.pack_into("<I", m, MC_OFF, (seq + 2) & 0xFFFFFFFF)       # par: pronto
                tick = struct.unpack_from("<Q", m, OFF_MC_TICK)[0]
                struct.pack_into("<Q", m, OFF_MC_TICK, tick + 1)
                struct.pack_into("<Q", m, OFF_MC_HEARTBEAT_MS, int(time.time() * 1000))
                time.sleep(1 / 30)
        except KeyboardInterrupt:
            pass
        finally:
            set_flag(m, HDR_MC_DRIVES, False)
            set_flag(m, HDR_MC_READY, False)
            struct.pack_into("<Q", m, OFF_MC_HEARTBEAT_MS, 0)
            m.flush(); m.close()
            print("controle devolvido ao jogo")


if __name__ == "__main__":
    main()
