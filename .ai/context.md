# Contexto do projeto TPCraft

## Objetivo

TPCraft é um mod/projeto que faz o Minecraft rodar em paralelo com o Zelda: Twilight Princess via Dusklight, usando memória compartilhada como ponte de comunicação. O objetivo é sincronizar o Link do jogo com o jogador do Minecraft, permitindo que o mundo do jogo e o mundo do Minecraft se comuniquem.

## Tecnologias

- C++ para a ponte do lado do jogo (Dusklight)
- Java para a ponte do lado Minecraft/Fabric
- Python para simulação e testes de integração
- CMake para build do módulo nativo
- Gradle para o projeto Fabric
- Protocol buffers / layout de memória compartilhada em C (arquivo de protocolo)
- Dusklight SDK + mod nativo

## Estrutura do projeto

- protocol/tpcraft_protocol.h = layout da memória compartilhada; fonte da verdade do protocolo
- dusklight-mod/ = mod nativo do Dusklight
- dusklight-mod/src/link.* = ponte C++ do lado do jogo
- fabric/ = lado Java para integração com Minecraft/Fabric
- tools/ = scripts de teste e simulação
- docs/ = documentação técnica e arquitetura
- .ai/ = memória compartilhada para agentes/IA
- build/ = artefatos de compilação locais

## Regras de trabalho

- Não alterar o protocolo sem revisar impactos nos dois lados (C++/Java/Python).
- Não apagar ou reescrever código sem verificar dependências.
- Não instalar dependências extras sem necessidade.
- Sempre rodar testes relevantes após mudanças de protocolo, ponte ou mod.
- Não sair do escopo da tarefa atual.
- Atualizar architecture.md e tasks.md quando houver mudanças relevantes.
- Sempre manter o protocolo e o comportamento coerentes entre as linguagens.

## Estado atual

Status geral: fundação de IPC v2 implementada; passthrough completo ainda não implementado.

 - Protocolo v2: snapshots, heartbeats e ring SPSC host->guest de input.
 - Snapshots e eventos do ring: interoperabilidade C++/Java/Python validada.
 - Mod nativo do Dusklight: build Windows passa; o movimento precisa de validação de runtime.
 - Lado Minecraft (Fabric): build MC 26.3 passa; o guest encaminha eventos do ring aos handlers Minecraft.
- Tools de simulação: funcionam para testes de ponte.
 Próximas fases: produtor de input no host, colisão TP->Minecraft, render de blocos/avatar/HUD/entidades e validação real.

 - O passthrough completo ainda não foi validado no jogo.
- O arquivo de protocolo é a fonte da verdade do fluxo de memória compartilhada.
- A simulação de Minecraft e do Dusklight são usadas para testar a ponte sem depender do jogo completo.
- A calibração de UNITS_PER_BLOCK e dos estados do Link precisa ser validada no jogo.

## Contexto operacional

Ambiente de desenvolvimento:
- Windows + VS Code
- CMake + Visual Studio Build Tools
- Java 21+ recomendado
- Python 3

Comandos relevantes:

- Build do projeto nativo: cmake -S dusklight-mod -B build ...
- Build do módulo: cmake --build build --config RelWithDebInfo --target tpcraft
- Testes gerais: bash tools/run_tests.sh
- Simulação do lado Minecraft: python tools/fake_dusklight.py run

## Problemas conhecidos

- O mod do Dusklight compila, mas ainda não foi carregado no jogo real.
- O Fabric compila, mas ainda não foi carregado no Minecraft em execução real.
- UNITS_PER_BLOCK e estados do Link ainda precisam de medição no jogo real.
- O status final do comportamento vivo do Link depende de validação no Dusklight.

---

Este arquivo é a memória principal do projeto. Antes de qualquer mudança, todas as IAs devem lê-lo e respeitar suas regras e o estado atual.
