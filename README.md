# DuskCraft

Minecraft inside **The Legend of Zelda: Twilight Princess** (Dusklight port), in the style of
[SkyCraft](https://github.com/chasmlol/SkyCraft) / [ValCraft](https://github.com/LoAlCo/ValCraft):
both games run at the same time and talk through **shared memory**.
Minecraft runs hidden and simulates the player; Dusklight renders everything.

> Status: **IPC v2 foundation in progress; passthrough still incomplete.**
> - Snapshots and SPSC input ring are tested across C++, Java and Python.
> - Native and Fabric builds pass on Windows.
> - Fabric consumes input events from the protocol; capturing them on the Dusklight side and validating in the runtime is still pending.
> - Streamed collision and export/render of blocks, avatar, entities and HUD are not integrated yet.

## What is here

| Folder | What it is | State |
|---|---|---|
| `protocol/tpcraft_protocol.h` | Shared memory layout v2 | snapshots, heartbeat and input ring |
| `dusklight-mod/src/link.*` | Game-independent C++ bridge | compiles; interop tested |
| `dusklight-mod/` | Native mod and Dusklight hooks | compiles; movement/render need real validation |
| `fabric/src/main/java/dev/tpcraft/link/` | MC/Fabric-independent Java bridge | snapshots and ring consumer tested |
| `fabric/` (rest) | Entrypoint and Gradle build | build passes; forwards ring events to MC handlers |
| `tools/fake_dusklight.py` | Fake game to test the Minecraft side | ready |
| `tools/run_tests.sh` | C++ <-> Java <-> Python test | ready |
| `docs/DESIGN.md` | Architecture, phases and risks | |

## Testing the bridge

cd "C:\Users\henri\OneDrive\Documentos\DuskCraft"

@'
# DuskCraft

Minecraft inside **The Legend of Zelda: Twilight Princess** (Dusklight port), in the style of
[SkyCraft](https://github.com/chasmlol/SkyCraft) / [ValCraft](https://github.com/LoAlCo/ValCraft):
both games run at the same time and talk through **shared memory**.
Minecraft runs hidden and simulates the player; Dusklight renders everything.

> Status: **IPC v2 foundation in progress; passthrough still incomplete.**
> - Snapshots and SPSC input ring are tested across C++, Java and Python.
> - Native and Fabric builds pass on Windows.
> - Fabric consumes input events from the protocol; capturing them on the Dusklight side and validating in the runtime is still pending.
> - Streamed collision and export/render of blocks, avatar, entities and HUD are not integrated yet.

## What is here

| Folder | What it is | State |
|---|---|---|
| `protocol/tpcraft_protocol.h` | Shared memory layout v2 | snapshots, heartbeat and input ring |
| `dusklight-mod/src/link.*` | Game-independent C++ bridge | compiles; interop tested |
| `dusklight-mod/` | Native mod and Dusklight hooks | compiles; movement/render need real validation |
| `fabric/src/main/java/dev/tpcraft/link/` | MC/Fabric-independent Java bridge | snapshots and ring consumer tested |
| `fabric/` (rest) | Entrypoint and Gradle build | build passes; forwards ring events to MC handlers |
| `tools/fake_dusklight.py` | Fake game to test the Minecraft side | ready |
| `tools/run_tests.sh` | C++ <-> Java <-> Python test | ready |
| `docs/DESIGN.md` | Architecture, phases and risks | |

## Testing the bridge
bash tools/run_tests.sh # requires g++, JDK 21+ and python3
python tools/fake_dusklight.py run # simulates Link walking in circles and prints what MC sends

text

## Building the mod (Windows)

Requires git, CMake 3.26+, Visual Studio Build Tools (C++) and internet (CMake downloads the Dusklight SDK
and a link-stub).
cmake -S dusklight-mod -B build -DDUSKLIGHT_VERSION=<tag of YOUR Dusklight version>
cmake --build build --config RelWithDebInfo --target tpcraft
copy build\mods\tpcraft.dusk %APPDATA%\TwilitRealm\Dusklight\mods

text

Note: mods with `game` depend on the Dusklight version. The CMake default is `v2.0.3`;
change it to the version you have installed. In the sandbox I built against current `main`, on Linux.
If any API name does not exist in your version, the compile error will point exactly to it.

## Next phases

1. Wire a Dusklight-supported input producer into the ring already consumed by Fabric.
2. Implement TP->Minecraft collision by regions; the current heightfield does not represent walls or objects.
3. Export block sections, avatar, entities, particles and overlay from Fabric to the Dusklight renderer.
4. Calibrate coordinates per stage/room, scale and orientation in the real runtime.
5. Integrate interactions, combat, loot and persistence; test each subsystem in-game.

## Notes

- The TPCraft v2 protocol is its own; it is not binary-compatible with SkyCraft/ValCraft.
- No Nintendo or Mojang assets are included. You need your own copy of the games.
