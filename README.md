# Monitor de Alertas Zabbix em ESP32

Projeto que consulta a API do Zabbix em tempo real e exibe os alertas ativos
num display TFT conectado ao ESP32. Também disponibiliza um dashboard web
acessível por qualquer dispositivo na mesma rede local.

## Funcionalidades

- Mostra alertas ativos do Zabbix com cor por severidade (usa trigger.get — exatamente o "Current Problems")
- Exibe: host, problema, duração, severidade
- Logo da empresa no header do display
- Dashboard web (HTTP) acessível via navegador no IP do ESP32
- API JSON em `/api/alerts` para integrações externas
- Tela "Tudo OK" quando não há problemas ativos
- Auto-refresh: consulta o Zabbix a cada 10 segundos

## Hardware suportado

### Display principal (recomendado)

**Módulo ESP32-2432S028R (Cheap Yellow Display / CYD)**
- Resolução: 320x240 pixels
- Driver: ILI9341
- Interface: SPI
- Touch screen resistivo (XPT2046)
- ESP32 integrado com WiFi/Bluetooth
- Chip USB: CH340
- Alimentação: 5V USB

Com essa resolução, todas as informações cabem em uma única tela:
severidade, host, problema, data de início, duração, GLPI, opdata, tags
e status das notificações.

### Display alternativo (compacto)

**LilyGO TTGO T-Display** (ESP32 + tela ST7789 240x135 embutida)
- Resolução: 240x135 pixels
- Layout compacto: um alerta por vez, informações essenciais
- Não necessita fiação

### Usando outro display?

O código está separado em `src/display.cpp` para facilitar a troca:

- **TFT 2.8" ILI9341 (320x240)**: layout completo com todas as informações
- **TFT 1.14" ST7789 (240x135)**: layout compacto, informações essenciais
- **OLED SSD1306**: troque TFT_eSPI por `Adafruit_SSD1306` (só texto, sem cores)

Ajuste os pinos em `include/User_Setup.h` conforme a fiação do seu módulo.

## O que aparece na tela (display 2.8")

Cada alerta exibe:
- **Header**: Logo da empresa + indicador WiFi + total de alertas
- **Severidade**: cor + nome (Alta, Média, Atenção, etc.) + duração
- **Host**: nome do equipamento + badge ACK (se reconhecido)
- **Problema**: descrição completa do trigger
- **Detalhes**: data/hora de início, número do chamado GLPI, operational data
- **Tags**: scope, component, class, target
- **Notificações**: quem foi notificado, por qual meio, e se foi enviado ou falhou

## Dashboard Web

Além do display físico, o ESP32 serve uma página web na porta 80:

- `http://<IP_DO_ESP32>/` — Dashboard HTML com todos os alertas (auto-refresh 15s)
- `http://<IP_DO_ESP32>/api/alerts` — API JSON para integrações

O IP é mostrado no Serial Monitor ao iniciar. Pode ser acessado de qualquer
dispositivo (celular, PC) na mesma rede WiFi.

## Passo a passo

### 1. Criar um usuário/token só de leitura no Zabbix

Por segurança, **não use o usuário Admin**. Crie um usuário com permissão
apenas de leitura:

1. `Administração > Usuários > Usuários` → criar usuário, ex: `esp32-monitor`
2. Dê a ele um grupo de permissão só leitura (`Read-only`)
3. `Administração > Usuários > Tokens de API` → gere um token para esse
   usuário (Zabbix 5.4+). Copie o token — ele só aparece uma vez.

Se sua versão do Zabbix for anterior a 5.4, use usuário/senha — o código
tem esse fallback pronto em `zabbix_client.cpp` (`user.login`).

### 2. Configurar as credenciais

```bash
cp include/secrets.h.example include/secrets.h
```

Edite `include/secrets.h` com:
- SSID e senha do WiFi
- URL da API do Zabbix (`http://SEU_ZABBIX/zabbix/api_jsonrpc.php`)
- O token de API (ou usuário/senha)

O arquivo `secrets.h` **não deve ir para repositório** — adicione
`include/secrets.h` no `.gitignore`.

### 3. Compilar e enviar para o ESP32

Com PlatformIO instalado:

```bash
pio run                      # compilar
pio run -t upload            # gravar no ESP32
pio device monitor           # ver log serial
```

Se o ESP32 não entrar em modo download automaticamente, segure o botão
BOOT enquanto pressiona RST, depois solte BOOT.

### 4. Acessar o dashboard web

Após o boot, o Serial Monitor mostra:
```
[Web] Servidor HTTP em http://192.168.x.x/
Alertas encontrados: 5
```

Abra esse IP no navegador de qualquer dispositivo na mesma rede.

### 5. Ajustes finos

- `POLL_INTERVAL_MS` (em `main.cpp`): intervalo entre consultas ao Zabbix (padrão: 10s)
- `CYCLE_INTERVAL_MS`: tempo de exibição de cada alerta na tela (padrão: 4s)
- `tft.setRotation()` em `display.cpp`: rotação do display (0 a 3)

## Compatibilidade com versões do Zabbix

- **Zabbix 5.4+**: usa API token (recomendado)
- **Zabbix 7.0+**: compatível (parâmetro `selectHosts` removido de `problem.get`)
- **Zabbix < 5.4**: usa login com usuário/senha

## Sobre segurança

- Use HTTPS na `ZABBIX_API_URL` se seu servidor tiver certificado.
  Nesse caso adicione o certificado root ou use `setInsecure()` (ok para rede interna).
- O usuário do token deve ter **apenas leitura**. O dispositivo nunca
  precisa escrever no Zabbix.

## Estrutura do projeto

```
├── platformio.ini
├── mockup_display.html     # simulador visual do display (abrir no navegador)
├── include/
│   ├── User_Setup.h        # pinagem do display (TFT_eSPI)
│   ├── secrets.h.example   # template de credenciais
│   ├── secrets.h           # credenciais reais (não versionar)
│   ├── zabbix_client.h
│   ├── display.h
│   └── web_server.h
└── src/
    ├── main.cpp            # WiFi, NTP, loop principal
    ├── zabbix_client.cpp   # chamadas JSON-RPC ao Zabbix
    ├── display.cpp         # desenho na tela TFT
    └── web_server.cpp      # servidor HTTP async
```

## Dependências

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) — driver do display
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) — parser JSON
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) — web server async
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP) — TCP async para ESP32

Todas são instaladas automaticamente pelo PlatformIO via `lib_deps` no `platformio.ini`.

## Mockup do display

O arquivo `mockup_display.html` é um simulador visual interativo que mostra
exatamente como os alertas aparecem no display físico. Abra no navegador para
testar o layout sem precisar do hardware:

```
file:///caminho/para/mockup_display.html
```
