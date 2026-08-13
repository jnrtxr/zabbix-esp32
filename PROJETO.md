# Documentacao Completa do Projeto - Zabbix ESP32 Monitor

## Visao Geral

Este projeto implementa um monitor de alertas do Zabbix em hardware embarcado
usando ESP32 com display TFT. O dispositivo consulta a API JSON-RPC do Zabbix
em tempo real, exibe os alertas ativos na tela fisica e disponibiliza um
dashboard web acessivel por qualquer dispositivo na rede local.

O objetivo e dar visibilidade imediata dos problemas de infraestrutura em
ambientes de NOC, salas de TI, mesas de operacao ou qualquer local onde um
painel fisico de monitoramento seja util.

---

## Contexto e Motivacao

Em ambientes corporativos de TI, os alertas do Zabbix ficam restritos ao
painel web que precisa ser acessado ativamente. Este projeto resolve isso
colocando um display dedicado que mostra os problemas em tempo real, sem
necessidade de interacao — basta olhar.

Ambiente de producao: **monitoramento.gruposoufer.com.br** (Grupo Soufer)

---

## Arquitetura do Sistema

```
┌─────────────┐         HTTP/JSON-RPC         ┌──────────────────┐
│  Zabbix     │◄────────────────────────────── │     ESP32        │
│  Server     │                                │  (WiFi 2.4GHz)   │
│  (API)      │─────────────────────────────►  │                  │
└─────────────┘       Resposta JSON            │  ┌────────────┐  │
                                               │  │ Display TFT│  │
┌─────────────┐         HTTP GET               │  │ 320x240    │  │
│ Navegador   │◄────────────────────────────── │  └────────────┘  │
│ (PC/Celular)│                                │                  │
└─────────────┘    Dashboard HTML/JSON         └──────────────────┘
```

### Fluxo de Dados

1. ESP32 conecta no WiFi (2.4GHz)
2. Sincroniza relogio via NTP (pool.ntp.org)
3. Autentica na API do Zabbix (Bearer token ou user/pass)
4. Consulta `problem.get` a cada 30 segundos
5. Exibe alertas no display TFT (cicla entre eles a cada 4s)
6. Serve dashboard web na porta 80 (auto-refresh 15s)

---

## Hardware

### Configuracao Principal (Recomendada)

**Modulo ESP32 com LCD TFT 2.8" 320x240 + Touch**

| Especificacao | Valor |
|---|---|
| Display | TFT ILI9341 2.8" 320x240 |
| CPU | ESP32 Dual-core 240MHz |
| SRAM | 520 KB |
| Flash | 32Mbit (4MB) |
| WiFi | 802.11 b/g/n 2.4GHz |
| Bluetooth | 4.2 BR/EDR + BLE |
| Alimentacao | 5V USB |
| Touch | Capacitivo |
| Interface Display | SPI |

Modelo: SHCHV ESP32 2.8" TFT Touch (AliExpress)

### Configuracao Alternativa (Compacta)

**LilyGO TTGO T-Display**

| Especificacao | Valor |
|---|---|
| Display | ST7789 1.14" 240x135 |
| CPU | ESP32 Dual-core 240MHz |
| Flash | 4MB |
| Botoes | 2 (BOOT + RST) |
| USB | CP2102/CH340 |

---

## Software

### Stack Tecnologica

| Camada | Tecnologia |
|---|---|
| Framework | Arduino (PlatformIO) |
| Plataforma | Espressif32 7.0.1 |
| Display | TFT_eSPI 2.5.43 |
| JSON | ArduinoJson 7.4.3 |
| Web Server | ESPAsyncWebServer 1.2.4 |
| TCP | AsyncTCP 1.1.1 |
| Protocolo Zabbix | JSON-RPC 2.0 sobre HTTP |
| Autenticacao | Bearer Token (Zabbix 5.4+) ou user/pass |

### Estrutura de Arquivos

```
zabbix-esp32-monitor/
├── .gitignore                 # Protege secrets.h
├── platformio.ini             # Configuracao PlatformIO (board, libs, flags)
├── README.md                  # Documentacao resumida
├── PROJETO.md                 # Este arquivo (documentacao completa)
├── mockup_display.html        # Simulador visual interativo do display
│
├── include/
│   ├── User_Setup.h           # Pinagem do display (TFT_eSPI)
│   ├── secrets.h.example      # Template de credenciais
│   ├── secrets.h              # Credenciais reais (NAO VERSIONAR)
│   ├── zabbix_client.h        # Header do client Zabbix
│   ├── display.h              # Header do modulo de display
│   └── web_server.h           # Header do web server
│
└── src/
    ├── main.cpp               # Setup, loop, WiFi, NTP, orquestracao
    ├── zabbix_client.cpp      # Chamadas JSON-RPC (login, problem.get)
    ├── display.cpp            # Renderizacao na tela TFT
    └── web_server.cpp         # Servidor HTTP async (dashboard + API)
```

---

## Modulos do Codigo

### main.cpp — Orquestracao

Responsavel por:
- Inicializar Serial, Display e WiFi
- Sincronizar relogio via NTP
- Iniciar o web server
- Loop principal: polling do Zabbix + ciclo de exibicao no display
- Reconexao automatica se WiFi cair

Parametros ajustaveis:
- `POLL_INTERVAL_MS` = 30000 (consulta Zabbix a cada 30s)
- `CYCLE_INTERVAL_MS` = 4000 (troca alerta na tela a cada 4s)

### zabbix_client.cpp — Comunicacao com Zabbix

Responsavel por:
- Autenticacao via Bearer token (Zabbix 5.4+) ou user.login (versoes anteriores)
- Consulta de problemas ativos via `problem.get`
- Parse do JSON de resposta
- Calculo de duracao dos alertas usando timestamp Unix + NTP

Compatibilidade:
- Zabbix 5.4+: API token (recomendado)
- Zabbix 7.0+: sem `selectHosts` em problem.get (tratado)
- Zabbix < 5.4: fallback user/password

### display.cpp — Renderizacao no Display TFT

Responsavel por:
- Inicializacao do display (TFT_eSPI)
- Tela de boot com logo
- Tela de erro WiFi
- Tela "Sem alertas" (tudo OK)
- Tela de alerta com: severidade, host, problema, duracao, pagination dots

Layout moderno com:
- Barra fina de cor no topo (indica severidade)
- Dot colorido + nome da severidade
- Host em cor secundaria
- Problema em destaque (branco, fonte grande)
- Dots de paginacao no footer

Cores por severidade:
| Nivel | Nome | Cor |
|---|---|---|
| 0 | Nao classificado | Cinza |
| 1 | Informacao | Azul |
| 2 | Atencao | Amarelo |
| 3 | Media | Laranja |
| 4 | Alta | Vermelho coral |
| 5 | Desastre | Vermelho puro |

### web_server.cpp — Dashboard Web

Responsavel por:
- Servidor HTTP assincrono na porta 80
- Endpoint `/` — pagina HTML com todos os alertas (auto-refresh 15s)
- Endpoint `/api/alerts` — JSON array dos alertas para integracoes

A pagina web mostra:
- Todos os alertas simultaneamente (lista completa)
- Cards com cor por severidade
- Host, problema, duracao, status ACK
- Design responsivo (funciona no celular)

---

## API do Zabbix — Detalhes Tecnicos

### Autenticacao

```
Header: Authorization: Bearer <API_TOKEN>
Content-Type: application/json-rpc
```

### Metodo: problem.get

```json
{
    "jsonrpc": "2.0",
    "method": "problem.get",
    "params": {
        "output": "extend",
        "recent": false,
        "sortfield": ["eventid"],
        "sortorder": "DESC",
        "limit": 10
    },
    "id": 2
}
```

### Resposta (campos usados)

| Campo | Tipo | Descricao |
|---|---|---|
| name | string | Descricao do problema/trigger |
| severity | int | 0-5 (Not classified ate Disaster) |
| clock | unix timestamp | Quando o problema abriu |
| acknowledged | "0"/"1" | Se foi reconhecido |
| opdata | string | Dados operacionais (ex: "Space used: 86%") |
| hosts[].name | string | Nome do host (Zabbix < 7.0) |

### Metodo: alert.get (para notificacoes)

```json
{
    "jsonrpc": "2.0",
    "method": "alert.get",
    "params": {
        "output": ["alertid","clock","sendto","status","error"],
        "selectUsers": ["username","name","surname"],
        "selectMediatypes": ["name"],
        "eventids": ["9998994"],
        "limit": 6
    },
    "id": 1
}
```

Status das notificacoes:
- `1` = Sent (enviado com sucesso)
- `2` = Failed (falhou)

---

## Informacoes Exibidas por Alerta

### No display fisico (2.8" 320x240)

- Logo da empresa (header)
- Indicador WiFi (bolinha verde)
- Total de alertas ativos
- Severidade (cor + texto)
- Duracao desde abertura
- Nome do host
- Badge ACK (se reconhecido)
- Descricao completa do problema
- Data/hora de inicio
- Numero do chamado GLPI
- Dados operacionais (opdata)
- Tags: scope, component, class, target
- Status das notificacoes (usuario, meio, sent/failed)
- Pagination dots (indicador de posicao no ciclo)

### No dashboard web

- Todos os alertas em lista
- Cards com cor de severidade
- Host, problema, duracao, ACK
- Auto-refresh a cada 15 segundos
- API JSON em /api/alerts

---

## Integracao com GLPI

O Zabbix esta configurado para abrir chamados automaticamente no GLPI
quando um alerta dispara. O numero do chamado e obtido via tag
`__zbx_glpi_problem_id` nos dados do evento.

O display mostra o numero do chamado GLPI associado a cada alerta,
facilitando a rastreabilidade entre o monitoramento e o helpdesk.

---

## Seguranca

- **secrets.h**: arquivo de credenciais que NAO e versionado (.gitignore)
- **API Token**: usuario dedicado so-leitura no Zabbix
- **Rede**: comunicacao HTTP em rede local (usar HTTPS se exposto)
- **Sem escrita**: o dispositivo nunca modifica dados no Zabbix

---

## Deploy e Configuracao

### Pre-requisitos

1. PlatformIO instalado (CLI ou extensao VS Code)
2. Driver USB: CP2102 ou CH340 (conforme o modulo)
3. Acesso a rede WiFi 2.4GHz
4. Zabbix com API habilitada + token de leitura

### Configuracao

1. Copiar `include/secrets.h.example` para `include/secrets.h`
2. Preencher SSID, senha WiFi, URL da API Zabbix e token
3. Ajustar pinagem em `include/User_Setup.h` (se usar display diferente)

### Compilacao e Upload

```bash
pio run                          # compilar
pio run -t upload --upload-port COMx   # gravar no ESP32
pio device monitor --port COMx   # ver log serial
```

### Boot Mode (se necessario)

Se o ESP32 nao entrar em download mode automaticamente:
1. Segurar botao BOOT
2. Pressionar e soltar RST (enquanto segura BOOT)
3. Soltar BOOT

---

## Mockup Interativo

O arquivo `mockup_display.html` e um simulador visual que reproduz
exatamente o layout do display fisico no navegador.

Para visualizar:
```
file:///caminho/para/mockup_display.html
```

Funcionalidades do mockup:
- Alertas reais do ambiente Zabbix
- Botoes para navegar entre alertas
- Ciclo automatico (simula comportamento real)
- Telas de boot, WiFi error e "Tudo OK"
- Logo da empresa Soufer integrado

---

## Roadmap / Melhorias Futuras

- [ ] Adaptar pinagem para modulo ESP32 2.8" ILI9341 (quando chegar)
- [ ] Implementar toque para navegar entre alertas manualmente
- [ ] Buscar hostname via `event.get` + `selectHosts` (Zabbix 7.0+)
- [ ] Adicionar som/buzzer para alertas de severidade Alta/Desastre
- [ ] OTA (Over-The-Air update) para atualizar firmware remotamente
- [ ] Suporte HTTPS para comunicacao com Zabbix
- [ ] Tela de resumo (total por severidade) antes de iniciar o ciclo
- [ ] Integracao com LVGL para UI mais rica no display touch

---

## Ambiente de Producao

| Item | Valor |
|---|---|
| Empresa | Grupo Soufer |
| Zabbix URL | monitoramento.gruposoufer.com.br |
| Versao Zabbix | 7.0+ |
| GLPI | helpdesk.gruposoufer.com.br |
| Rede WiFi | 2.4GHz interna |
| Repositorio | https://github.com/jnrtxr/zabbix-esp32 |

---

## Autor

Projeto desenvolvido para o NOC/TI do Grupo Soufer.

Repositorio: https://github.com/jnrtxr/zabbix-esp32
