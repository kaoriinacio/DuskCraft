# Tarefas do TPCraft

## Em andamento

- [ ] Ligar captura de input suportada pelo Dusklight ao ring host->guest
- [ ] Implementar streaming de colisão TP -> Minecraft e validar o movimento
- [ ] Integrar exportação de blocos, avatar, HUD, entidades e partículas ao renderer do Dusklight

## Pendentes

- [ ] Implementar colisão por regiões e streaming de geometria TP -> Minecraft
- [ ] Implementar exportação/render de chunks, avatar, entidades, partículas e overlay
- [ ] Validar o fluxo completo no jogo real
- [ ] Identificar e preencher `GS_RIDING`, `GS_WOLF`, `GS_SWIMMING` e `GS_LOADING` em `mod.cpp`
- [ ] Aumentar o nível de renderização do mundo e integrar blocos com `GfxService` / `CameraService`

## Concluídas

- [x] Protocolo de memória compartilhada definido em `protocol/tpcraft_protocol.h`
- [x] Ponte C++ / Java / Python validada via testes de integração de protocolo
- [x] Mod nativo do Dusklight compilando contra o SDK
- [x] Simulação de teste do lado Minecraft e do Dusklight funcional
- [x] Verificar o build/entrypoint do lado Fabric no ambiente do Minecraft
- [x] Analisar ValCraft/OWCraft e definir a arquitetura de passthrough do TPCraft
- [x] Adicionar heartbeat, flags atômicas e ring SPSC no protocolo v2
- [x] Validar snapshots e eventos de input entre C++, Java e Python
- [x] Encaminhar eventos do ring aos handlers de teclado/mouse/texto do Minecraft 26.3

## Problemas conhecidos

- [ ] O mod ainda não foi carregado e validado no jogo real
- [ ] O Fabric compila, mas ainda não foi carregado no Minecraft em execução real
- [ ] O comportamento do Link em runtime real precisa de calibração fina
- [ ] Há pontos do módulo ainda não preenchidos em `mod.cpp`
- [ ] `UNITS_PER_BLOCK` segue provisório e precisa ser medido no TP
- [ ] O Fabric consome o ring, mas falta um produtor de input suportado no host
- [ ] Colisão complexa, renderização Minecraft e Steve ainda não foram implementados

## Próximos passos

1. Obter eventos de input do host por API suportada e publicá-los no ring
2. Streamar colisão TP como triângulos/voxels por regiões prioritárias
3. Exportar e desenhar conteúdo Minecraft no render do Dusklight
4. Calibrar escala, eixos, origem por stage/sala e transições
5. Integrar entidades/eventos e validar cada fluxo no jogo real

## Checklist por tarefa

### Tarefa 1: validar o Link no jogo real

- Objetivo: confirmar que o Link vive e publica dados da ponte no runtime real
- Arquivos impactados: `dusklight-mod/`, `tools/`, `protocol/`
- Testes a rodar: build do módulo e execução do jogo com save carregado
- Status: pendente

### Tarefa 2: calibrar o movimento

- Objetivo: ajustar a escala e estados do Link para que o movimento seja fiel ao mundo do jogo
- Arquivos impactados: `dusklight-mod/src/`, `protocol/`
- Testes a rodar: simulação e validação no Dusklight com Link controlável
- Status: pendente

### Tarefa 3: validar o lado Fabric

- Objetivo: verificar que a ponte Java realmente funciona no runtime do Minecraft
- Arquivos impactados: `fabric/`, `tools/`
- Testes a rodar: build Gradle e simulação de controle externo
- Status: concluído

---

Sempre que uma IA concluir ou alterar o status de uma tarefa, atualize este arquivo. O rastreio do projeto precisa seguir a realidade do código e do ambiente.
