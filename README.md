# TPCraft

Minecraft dentro do **Zelda: Twilight Princess** (port Dusklight), no estilo do
[SkyCraft](https://github.com/chasmlol/SkyCraft) / [ValCraft](https://github.com/LoAlCo/ValCraft):
os dois jogos rodam ao mesmo tempo e conversam por **memoria compartilhada**.
O Minecraft roda escondido e simula o jogador; o Dusklight desenha tudo.

> Status: **fundação de IPC v2 em implementação; passthrough ainda incompleto.**
> - Snapshots e ring SPSC de input são testados entre C++, Java e Python.
> - Builds nativos e Fabric passam no ambiente Windows.
> - Fabric consome eventos de input do protocolo; falta capturá-los no DuskLight e validar no runtime.
> - Colisão streamed e exportação/render de blocos, avatar, entidades e HUD ainda não estão integrados.

## O que tem aqui

| Pasta | O que e | Estado |
|---|---|---|
| `protocol/tpcraft_protocol.h` | Layout da memoria compartilhada v2 | snapshots, heartbeat e ring de input |
| `dusklight-mod/src/link.*` | Ponte C++ independente do jogo | compila; interoperabilidade testada |
| `dusklight-mod/` | Mod nativo e hooks do Dusklight | compila; movimento/render precisam de validação real |
| `fabric/src/main/java/dev/tpcraft/link/` | Ponte Java independente de MC/Fabric | snapshots e consumer do ring testados |
| `fabric/` (resto) | Entrypoint e build Gradle | build passa; encaminha eventos do ring aos handlers MC |
| `tools/fake_dusklight.py` | Jogo falso pra testar o lado Minecraft | pronto |
| `tools/run_tests.sh` | Teste C++ <-> Java <-> Python | pronto |
| `docs/DESIGN.md` | Arquitetura, fases e riscos | |

## Testar a ponte

```
bash tools/run_tests.sh        # precisa de g++, JDK 21+ e python3
python tools/fake_dusklight.py run   # simula o Link andando em circulos e imprime o que o MC manda
```

## Buildar o mod (Windows)

Precisa de git, CMake 3.26+, Visual Studio Build Tools (C++) e internet (o CMake baixa o SDK do Dusklight
e um link-stub).

```
cmake -S dusklight-mod -B build -DDUSKLIGHT_VERSION=<tag da SUA versao do Dusklight>
cmake --build build --config RelWithDebInfo --target tpcraft
copy build\mods\tpcraft.dusk %APPDATA%\TwilitRealm\Dusklight\mods
```

Importante: mods com `game` dependem da versao do Dusklight. O padrao no CMake e `v2.0.3`;
troque pela versao que voce tem instalada. No sandbox eu buildei contra o `main` atual, em Linux.
Se algum nome da API nao existir na sua versao, o erro de compilacao aponta exatamente qual.

## Proximas fases

1. Ligar um produtor de input suportado pelo Dusklight ao ring já consumido pelo Fabric.
2. Implementar colisão por regiões TP->Minecraft; o heightfield atual não representa paredes e objetos.
3. Exportar seções de blocos, avatar, entidades, partículas e overlay do Fabric ao renderer do Dusklight.
4. Calibrar coordenadas por stage/sala, escala e orientação no runtime real.
5. Integrar interações, combate, loot e persistência; testar cada subsistema no jogo.

## Notas

- O protocolo TPCraft v2 e proprio; ele nao e binariamente compativel com SkyCraft/ValCraft.
- Nenhum asset da Nintendo ou da Mojang esta incluido. Voce precisa do seu proprio jogo.
