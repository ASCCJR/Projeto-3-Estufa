# Instrucoes Operacionais para IA (Projeto BitDogEstufa)

Este arquivo orienta agentes de IA no VS Code a preparar ambiente, validar dependencias e executar testes basicos deste projeto apos um clone limpo.

## Objetivo

Garantir que o projeto:
- compile sem erros;
- tenha broker MQTT e Node-RED prontos;
- permita teste de telemetria e comando MQTT;
- rode o fluxo de gravacao na BitDogLab quando o hardware estiver conectado.

## Contexto do projeto

- Firmware C para Raspberry Pi Pico W (BitDogLab).
- Build com CMake + Ninja (task de compilacao ja existe no workspace).
- Integracao com MQTT e Node-RED.
- Broker padrao: Mosquitto.

## Modo de execucao da IA (obrigatorio)

- Executar o fluxo na ordem, sem pular passos.
- Nao tentar gravar na placa antes de build aprovado.
- Nao tentar validar firmware MQTT antes de validar MQTT local no host.
- Quando faltar dado local (SSID/senha/IP/porta), solicitar ao usuario de forma objetiva e seguir apos resposta.
- Preferir `secrets.local.h` e `configura_local.h` para ajustes por maquina.
- Tratar falha de flash com build OK como bloqueio fisico/conexao, nao como erro de codigo.

## Fluxo que a IA deve seguir

0. Validar configuracao local obrigatoria (`secrets.local.h` e `configura_local.h`).
1. Detectar ferramentas instaladas.
2. Instalar somente o que faltar.
3. Validar versoes e servicos.
4. Executar build do firmware.
5. Validar MQTT local com publish/subscribe.
6. Iniciar Node-RED.
7. Orientar teste com hardware real (BOOTSEL ou CMSIS-DAP).

## Passo 0: Configuracao local obrigatoria (clone limpo)

- Verificar se existem `secrets.local.h` e `configura_local.h` na raiz do projeto.
- Se nao existirem, copiar:
  - `secrets.local.example.h` -> `secrets.local.h`
  - `configura_local.example.h` -> `configura_local.h`
- Antes de tentar conexao Wi-Fi/MQTT real, confirmar que `secrets.local.h` nao esta com placeholders.
- Se houver placeholder, pedir ao usuario SSID/senha e IP/porta do broker para preencher os arquivos locais.

## Passo 1: Deteccao inicial (Windows)

Verificar se existem:
- winget
- cmake
- ninja
- pasta `%USERPROFILE%\\.pico-sdk` (toolchain instalada pela extensao Raspberry Pi Pico)
- node
- npm
- mosquitto
- mosquitto_pub
- mosquitto_sub

Se node/npm nao forem encontrados logo apos instalacao, considerar PATH desatualizado na sessao atual e usar caminho absoluto:
- C:\Program Files\nodejs\node.exe
- C:\Program Files\nodejs\npm.cmd

## Passo 2: Instalacao automatica minima

Se faltar Node.js:
- Instalar `OpenJS.NodeJS.20` via winget.

Se faltar CMake:
- Instalar `Kitware.CMake` via winget.

Se faltar Ninja (e nao houver ninja local do Pico SDK):
- Instalar `Ninja-build.Ninja` via winget.

Se faltar toolchain da Pico (`%USERPROFILE%\\.pico-sdk` ausente):
- Solicitar instalacao/configuracao da extensao Raspberry Pi Pico no VS Code e concluir o setup de ferramentas da extensao.
- So prosseguir para o build apos a toolchain estar disponivel.

Se faltar broker MQTT:
- Instalar `EclipseFoundation.Mosquitto` via winget.

Se faltar Node-RED:
- Executar `npm install -g --unsafe-perm node-red`.

Boas praticas:
- Aceitar contratos de origem/pacote no winget para evitar prompt interativo.
- Nao instalar ferramentas extras sem necessidade.

## Passo 3: Validacao de runtime

- Confirmar versoes de node e npm.
- Confirmar servico `mosquitto` em execucao.
- Confirmar conectividade local na porta 1883.
- Testar MQTT real:
  - publicar em um topico de teste;
  - assinar o mesmo topico e validar recebimento.

Exemplo de teste rapido (ajustar porta para 1884 se necessario):
- Subscriber: `mosquitto_sub -h 127.0.0.1 -p 1883 -t copilot/test -C 1`
- Publisher: `mosquitto_pub -h 127.0.0.1 -p 1883 -t copilot/test -m ok`

  ### Troubleshooting Wi-Fi e MQTT (obrigatorio quando travar em "Conectando MQTT")

  Se a placa sair de Wi-Fi e ficar presa em "Conectando MQTT", a IA deve:

  1. Verificar IP atual da maquina host e confirmar se bate com `MQTT_BROKER_IP` (preferir `configura_local.h`).
  2. Confirmar que o broker esta escutando em rede local, e nao apenas localhost.
    - Se `Get-NetTCPConnection` mostrar apenas `127.0.0.1`/`::1`, a Pico nao conseguira conectar.
  3. Quando houver permissao de administrador, ajustar `mosquitto.conf` para listener em rede local (ex.: `listener 1883 0.0.0.0`) e reiniciar servico.
  4. Quando NAO houver permissao de administrador, usar fallback sem admin:
    - subir broker local com `mosquitto.local.conf` em porta 1884 (`listener 1884 0.0.0.0`, `allow_anonymous true`);
    - ajustar `MQTT_BROKER_PORT` em `configura_local.h` para 1884;
    - ajustar broker do Node-RED para 1884.
  5. Rodar broker em modo verboso e validar log de conexao do cliente (mensagem tipo `New client connected ... bitdoglab_02_client`).
  6. Se ainda falhar, testar hotspot 2.4 GHz no celular e verificar isolamento de clientes (AP/client isolation).

## Passo 4: Node-RED

- Perguntar ao usuario se deseja abrir o Node-RED no navegador interno do VS Code ou no navegador externo do sistema.
- Se preferir navegador interno, abrir no VS Code (Simple Browser) em http://127.0.0.1:1880/.
- Se preferir navegador externo, abrir o navegador padrao em http://127.0.0.1:1880/.
- Iniciar Node-RED.
- Confirmar que o editor responde em http://127.0.0.1:1880/.
- Importar `dashboard_estufa.json` e fazer `Deploy`.
- Para visualizar o dashboard, usar a URL http://127.0.0.1:1880/ui (o endereco sem /ui abre o editor de fluxos).

### Troubleshooting Node-RED Dashboard (obrigatorio quando houver erro de importacao)

Se ao importar o fluxo aparecer "Imported unrecognised types" para `ui_*` (ex.: `ui_tab`, `ui_group`, `ui_gauge`, `ui_chart`, `ui_text`, `ui_button`), a IA deve:

1. Verificar se o pacote de dashboard esta instalado no userDir ativo do Node-RED (`%USERPROFILE%\\.node-red`).
2. Se nao estiver instalado, executar:
  - `npm install node-red-dashboard` (dentro de `%USERPROFILE%\\.node-red`).
3. Reiniciar o Node-RED apos instalacao de nodes.
4. Confirmar no log que o dashboard carregou (mensagem semelhante a `Dashboard version ... started at /ui`).
5. Pedir para o usuario recarregar o editor (Ctrl+F5), reimportar o JSON e fazer `Deploy`.
6. Abrir/confirmar o dashboard em http://127.0.0.1:1880/ui.

Observacoes:
- Se o usuario vir apenas "Flow 1" e os quadrados dos nodes, ele esta no editor de fluxos, nao no dashboard.
- Nao concluir falha do projeto antes de validar o endpoint `/ui` e o deploy do fluxo.

### Troubleshooting de texto corrompido no dashboard (mojibake)

Se o dashboard mostrar textos com `Ã`, `ðŸ` ou acentos/emojis quebrados:

1. Garantir que o JSON do fluxo esteja salvo em UTF-8.
2. Reimportar o fluxo e fazer `Deploy`.
3. Se o deploy for por API REST, ler e enviar o JSON em UTF-8 explicito.
  - em PowerShell: `Get-Content -Raw -Encoding utf8` e enviar body como bytes UTF-8.
4. Validar visualmente no `/ui` se labels e textos voltaram ao normal.

## Passo 5: Build do firmware

Usar a task existente do workspace:
- Compile Project

Se for clone limpo e `build/build.ninja` nao existir (ou a task falhar por falta de configuracao):
1. Rodar configuracao CMake do projeto (preferencialmente pela extensao Raspberry Pi Pico no VS Code).
2. Se necessario, usar fallback via terminal: `cmake -S . -B build -G Ninja`.
3. Reexecutar a task `Compile Project`.

Criterio de sucesso:
- gerar `Projeto3Estufa.elf` sem erro de compilacao/link.

## Passo 6: Gravacao na placa (quando hardware conectado)

Tentar nesta ordem:
1. Run Project (modo BOOTSEL).
2. Flash (OpenOCD/CMSIS-DAP).

Se falhar por dispositivo nao detectado:
- informar causa objetiva;
- orientar usuario a conectar em BOOTSEL ou conectar probe CMSIS-DAP;
- nao marcar como erro de codigo enquanto build estiver OK.

## Passo 7: Teste funcional MQTT do firmware

Com firmware gravado e placa ligada:
- assinar `bitdoglab_02/#` no Node-RED;
- validar recebimento de:
  - sensores/temperatura
  - sensores/umidade
  - sensores/luminosidade
  - heartbeat
- publicar comando:
  - topico: `bitdoglab_02/comando/estado`
  - payload: `IRRIGAR`
- validar resposta fisica (servo, matriz/LED, display) e evento MQTT.

Checagem rapida para evitar confusao com firmware de outro projeto:
- Estufa publica em `sensores/temperatura`, `sensores/umidade`, `sensores/luminosidade` e `heartbeat`.
- Se aparecer apenas `status` e `historico`, a placa pode estar com firmware da Fechadura.

Quando houver mais de uma placa em paralelo:
- evitar usar o mesmo `DEVICE_ID` em projetos diferentes;
- preferir override local com IDs distintos (ex.: `bitdoglab_02` e `bitdoglab_03`).

## Passo 8: Teste de estabilidade

- Evitar enviar `IRRIGAR` varias vezes em sequencia curta durante a validacao.
- Testar um comando por vez e aguardar o ciclo completo terminar antes do proximo envio.
- Se o sistema travar apenas sob repeticao de comandos, tratar isso como problema de robustez do fluxo, nao como falha de sensores.
- Manter o dashboard final sem nodes de teste automáticos que publiquem em loop.

## Checklist rapido de clone limpo (gates)

Antes de encerrar, a IA deve marcar internamente estes gates como OK:

1. Arquivos locais criados e preenchidos (`secrets.local.h`, `configura_local.h`).
2. Dependencias minimas presentes (node/npm, mosquitto, cmake/ninja, toolchain Pico).
3. MQTT local validado com publish/subscribe no host.
4. Build concluido e ELF gerado.
5. Node-RED aberto, fluxo importado e dashboard acessivel em `/ui`.
6. Se hardware presente: tentativa de flash executada e resultado classificado (OK ou bloqueio fisico).

## Regras de alteracao de arquivos

- Evitar commits com credenciais. Preferir `secrets.local.h` (ignorado no git).
- Evitar alterar IP de broker em `configura_geral.h` sem avisar o usuario. Preferir `configura_local.h` (ignorado no git).
- Se os arquivos locais nao existirem, orientar o usuario a copiar `secrets.local.example.h` -> `secrets.local.h` e `configura_local.example.h` -> `configura_local.h`.
- Se IP do broker local mudar, propor ajuste explicito em `configura_local.h` e recompilar.
- Nao reverter mudancas do usuario sem solicitacao.

## Formato de retorno esperado da IA

Ao final, a IA deve reportar:
1. O que foi detectado como instalado.
2. O que foi instalado automaticamente.
3. Resultado do teste MQTT local.
4. Resultado do build.
5. Resultado da gravacao na placa (ou bloqueio fisico).
6. Proximos passos objetivos para validacao final em bancada.

Template recomendado de saida:

1. Detectado: ...
2. Instalado: ...
3. MQTT local: OK/FAIL (...)
4. Build: OK/FAIL (...)
5. Flash: OK/BLOQUEIO FISICO (...)
6. Proximos passos: ...
