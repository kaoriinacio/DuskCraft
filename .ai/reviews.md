# Revisões das IAs

## ChatGPT

### Data:
2026-10-02
### Tarefa:
Contextualização do projeto e mapeamento do fluxo de trabalho
### Observações:

- O projeto tem arquitetura bem definida por protocolo central, com duas pontes em linguagens diferentes.
- O foco atual está em validar a ponte no runtime real do jogo, não em reescrever a base.
- O maior risco é o comportamento real do Link em Dusklight, e não a lógica de memória compartilhada em si.

### Sugestões:

- Continuar usando o protocolo como fonte da verdade para qualquer ajuste.
- Validar no jogo antes de mexer em escalas ou valores sensíveis.
- Usar a simulação apenas como apoio para regressões, não como substituto do teste real.

---

## Claude

### Data:
2026-10-02
### Tarefa:
Revisão da arquitetura e estado atual do projeto
### Observações:

- O estado mais avançado é a comunicação entre as três linguagens e a camada nativa compilando.
- A parte que ainda precisa de evidência concreta é o comportamento em runtime do Dusklight.
- O projeto está em um bom ponto para validação no jogo e refinamento de escala.

### Sugestões:

- Priorizar validação no ambiente real antes de novas alterações estruturais.
- Medir a escala do movimento do Link para ajustar `UNITS_PER_BLOCK` com base em observações reais.
- Manter a documentação de protocolo atualizada sempre que houver mudança de layout ou estado.

---

## DeepSeek

### Data:
2026-10-02
### Tarefa:
Diagnóstico do pipeline e próximos passos de validação
### Observações:

- A ponte C++/Java/Python parece funcional em simulação.
- O ponto crítico agora é o runtime real do jogo, onde as coordenadas, estados e flags exigem validação prática.
- O projeto está em fase de integração e ajuste fino, não de refatoração ampla.

### Sugestões:

- Validar `link.bin` em save real e confirmar atualização por frame.
- Testar diferença entre valores de posição e escala do mundo.
- Priorizar correções locais que afetem diretamente a sincronização do Link.

---

## Copilot

### Data:
2026-10-02
### Tarefa:
Ajuste inicial do setup de IA e organização do projeto
### Observações:

- O projeto está pronto para receber fluxo de IA orientado por memória compartilhada.
- A estrutura `.ai/` deve ser usada como contexto global para planejamento, revisão e execução.
- O projeto real tem prioridade sobre qualquer automação genérica.

### Sugestões:

- Usar `.ai/context.md` como guia inicial antes de cada tarefa.
- Sempre revisar `git diff` e `tasks.md` após qualquer alteração relevante.
- Manter a execução focada em uma tarefa por vez para reduzir regressão.

---

Use este arquivo como registro dos pontos de atenção e dos passos de revisão. Qualquer nova IA deve ler o conteúdo aqui antes de implementar correções.
