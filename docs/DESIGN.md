# Design

## Visao geral

```
 Dusklight (C++ mod)  <--- link.bin (mmap, 16 KB) --->  Minecraft + Fabric (Java, escondido)
   escreve GameBlock                                       le GameBlock
   le McBlock                                              escreve McBlock
```

- **Transporte:** arquivo `%LOCALAPPDATA%\TPCraft\link.bin` mapeado em memoria pelos dois processos
  (file-backed mmap: funciona igual em C++/Java/Python, sem JNI).
- **Consistencia:** seqlock por bloco. Writer: `seq++` (impar), escreve, `seq++` (par).
  Reader: le `seq`, copia, le `seq` de novo; se mudou ou era impar, tenta de novo.
- **Direcao desejada:** o jogo e dono do mundo (colisao, tempo, stage); o Minecraft e dono do jogador
  (posicao, fisica, inventario). `HDR_MC_DRIVES` reserva a flag de ownership; o fluxo ainda nao e passthrough completo.

O protocolo v2 adiciona heartbeats dos dois processos, atualizacao atomica das flags compartilhadas
e um ring SPSC de 256 eventos host->guest. O Fabric consome eventos e chama handlers nativos;
falta um produtor de input suportado no host. Colisao e renderizacao tambem ainda nao possuem rings;
o protocolo atual nao e um passthrough completo.

## GameBlock (jogo -> MC)

Posicao/yaw/velocidade do Link, hash do stage, flags de estado (cutscene, Epona, lobo, nadando,
loading), hora do dia e um **heightfield 33x33** de colisao ao redor do Link (altura do chao por celula,
NaN = sem chao). Heightfield e o primeiro passo barato; depois pode virar voxels ou malha.

## McBlock (MC -> jogo)

Posicao/yaw/pitch/velocidade, flags (chao, agachado, correndo, nadando), vida, slot da hotbar,
item na mao e contador de swings (o jogo dispara o ataque quando o contador muda).

## Fases

1. **Link vivo:** MC publica posicao, jogo move o Link. Sem colisao ainda (usar chao plano do heightfield).
2. **Colisao:** jogo exporta heightfield; MC usa como chao. Andar em colinas/escadas.
3. **Blocos:** o Dusklight expoe `GfxService` (WebGPU direto, estagios como `GFX_STAGE_SCENE_AFTER_OPAQUE`)
   e `CameraService` (matrizes de view/projecao, override de camera). Desenhar malhas de chunks ali, com profundidade.
   Precisa de `FEATURES ... webgpu` no `add_mod`.
4. **Combate/loot:** dano do MC nos atores inimigos; rupees e drops viram itens do MC.
5. **Escavar o terreno:** buracos na colisao do TP (como o SkyCraft 0.1.1 faz no Skyrim).

## Protocolo v2

- O host publica `GameState` e seu heartbeat; o guest publica `McState` e heartbeat. O guest consome input, mas o host ainda nao produz eventos reais.
- O ring de input transporta teclas, botoes, scroll, texto, release-all e abertura do menu.
- O ring foi definido como SPSC: um produtor Dusklight e um consumidor Minecraft.
- Proximas expansoes: ring de colisao TP->Minecraft e ring de renderizacao/eventos Minecraft->Dusklight.

## Cuidados

- Devolver o controle ao jogo em: cutscene, Epona, forma de lobo, loading, menus.
- Cada stage/sala precisa de espaco proprio de coordenadas no mundo do MC
  (o SkyCraft tem esse problema: interiores compartilham um mundo so).
- `UNITS_PER_BLOCK` (100.0) e provisorio.
- O Dusklight nao aceita PRs gerados principalmente por IA; vale so se for mandar algo de volta.
