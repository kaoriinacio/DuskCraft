# Arquitetura do TPCraft

## Visão geral

O projeto cria uma ponte de comunicação entre o jogo de Zelda: Twilight Princess em execução via Dusklight e um mundo Minecraft em paralelo. A ideia central é usar memória compartilhada e um protocolo fixo para representar o estado do Link e do mundo do jogo, permitindo sincronização de estado e ações.

## Componentes principais

### 1. Protocolo de memória compartilhada

- Nome: protocol/tpcraft_protocol.h
- Responsabilidade: definir o layout e o contrato de memória compartilhada entre os lados do jogo e do Minecraft
- Entradas: estado do Link, posição, estágio, flags, hora, estados do jogador
- Saídas: estrutura de leitura/escrita compartilhada
- Dependências: todas as implementações de ponte e simulação

### 2. Ponte C++ do lado do Dusklight

- Nome: dusklight-mod/src/link.*
- Responsabilidade: publicar o Link usando os dados do jogo e aplicar comandos vindos do Minecraft
- Entradas: estado do Link e do mundo do jogo
- Saídas: arquivo link.bin / estado compartilhado
- Dependências: SDK do Dusklight, mod nativo e protocolo

### 3. Mod nativo do Dusklight

- Nome: dusklight-mod/
- Responsabilidade: integrar a lógica do Link com o motor do jogo, publicar o estado e receber instruções do Minecraft
- Entradas: eventos do jogo, posição do Link, idade do estado do mundo
- Saídas: atualização de estado e comandos para o jogador
- Dependências: API do Dusklight, CMake, SDK e protocol

### 4. Ponte Java do lado Minecraft

- Nome: fabric/src/main/java/dev/tpcraft/link/
- Responsabilidade: ler o estado compartilhado e mover o jogador/Link no mundo do Minecraft
- Entradas: link.bin e estado do jogo externo
- Saídas: movimentação e sincronização do mundo virtual
- Dependências: protocolo e runtime do Fabric

### 5. Ferramentas de simulação e testes

- Nome: tools/
- Responsabilidade: simular o jogo, testar a ponte entre linguagens e verificar o fluxo de dados sem depender do jogo real
- Entradas: comandos de teste ou scripts Python
- Saídas: leitura de estado, movimentação, logs de diagnóstico
- Dependências: Python, C++, Java e protocolo

## Fluxo principal

1. O Dusklight publica o estado atual do Link em memória compartilhada.
2. A ponte C++ e/ou Java lê esse estado em um formato definido pelo protocolo.
3. O lado do Minecraft interpreta o estado e sincroniza o movimento / ações do jogador.
4. O projeto também permite o fluxo inverso: comandos do Minecraft são aplicados ao estado do Link no lado do Dusklight.
5. As ferramentas de simulação verificam se o protocolo funciona entre C++, Java e Python.

## Decisões de arquitetura

### 2026-10-02 - Separar protocolo da implementação

Motivo:
O projeto depende de múltiplas linguagens e implementações concorrentes. O protocolo central é a referência de verdade para evitar divergência entre as pontes.

Resultado esperado:
Tanto o lado C++ quanto o lado Java e o script em Python compartilham a mesma interpretação do estado do Link.

### 2026-10-02 - Usar memória compartilhada como canal principal

Motivo:
O projeto trabalha com sincronização em tempo real entre dois mundos diferentes e precisa de fluxo rápido, sem dependência de rede ou I/O pesado.

Resultado esperado:
O estado do Link e do mundo pode ser lido continuamente em tempo real.

### 2026-10-02 - Migrar toolchain do Fabric para MC 26.3

Motivo:
O alvo definido passou a ser Minecraft 26.3, que não é ofuscado e exige Loom 1.17+ com o plugin `net.fabricmc.fabric-loom` (sem `-remap`) e sem mappings Mojang/Yarn.

Resultado esperado:
O lado Fabric builda contra MC 26.3 sem remap de access widener e sem mappings obsoletos.

## Riscos e observações

- A validação real no jogo é ainda o maior risco: a ponte compila, mas ainda não foi testada no Dusklight em execução real.
- O estado do Link depende de campos específicos como position, stage, flags, state e hora do jogo.
- O ajuste de escala do movimento e as transições de estado precisam ser calibrados no jogo real.
- A troca de dados entre dois motores diferentes exige cuidado com coordenação de frames e estados.

## Critérios de aceitação

- O protocolo continua consistente entre C++, Java e Python.
- O link.bin é atualizado em tempo real quando o Link está ativo no jogo.
- A posição do Link é lida corretamente e muda conforme o personagem se move.
- O fluxo inverso do Minecraft para o jogo funciona sem corrupção de estado.
- O mod pode ser carregado e validado em ambiente do Dusklight real.

---

Este arquivo registra a arquitetura e as decisões do sistema. Antes de implementar qualquer mudança, confirme se ela bate com a arquitetura atual e com o estado da ponte.
