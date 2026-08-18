#include "display.h"
#include <TFT_eSPI.h>
#include "logo_bitmap.h"

static TFT_eSPI tft = TFT_eSPI();

// Dimensoes (landscape)
#define W 320
#define H 240

// Cores - paleta do mockup
#define BG        0x0841   // fundo escuro
#define HEADER_BG 0x0821   // header ligeiramente diferente
#define TEXT_W    0xFFFF   // branco
#define TEXT_SEC  0x9CF3   // cinza claro (host)
#define TEXT_DIM  0x52AA   // cinza medio (age, counter)
#define GREEN     0x07E0   // ACK, wifi dot
#define DIVIDER   0x18C3   // linhas divisorias
#define TAG_BG1   0x0931   // tag scope (azul escuro)
#define TAG_FG1   0x5DDF   // tag scope text
#define TAG_BG2   0x2012   // tag component (roxo escuro)
#define TAG_FG2   0xBB5F   // tag component text
#define TAG_BG3   0x0920   // tag class (verde escuro)
#define TAG_FG3   0x6FE6   // tag class text

// Severidade
static uint16_t sevColor(int s) {
    switch (s) {
        case 0: return 0x7BEF;
        case 1: return 0x5D7F;
        case 2: return 0xFE60;
        case 3: return 0xFC60;
        case 4: return 0xF8A0;
        case 5: return 0xF800;
        default: return 0x7BEF;
    }
}

static String fmtAge(unsigned long sec) {
    if (sec < 60) return "<1m";
    unsigned long m = sec / 60, h = m / 60, d = h / 24;
    if (d > 0) return String(d) + "d " + String(h % 24) + "h";
    if (h > 0) return String(h) + "h " + String(m % 60) + "m";
    return String(m) + "m";
}

// Flag pra evitar redesenho repetido
static int lastDrawnIndex = -1;
static int lastDrawnTotal = -1;

void display::begin() {
    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true); // necessario pra bitmap RGB565
    tft.fillScreen(BG);
    #ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
    #endif
}

void display::showBooting(const char* message) {
    lastDrawnIndex = -1;
    tft.fillScreen(BG);
    tft.setTextDatum(MC_DATUM);
    // Logo Soufer
    tft.pushImage(W/2 - LOGO_W/2, H/2 - 50, LOGO_W, LOGO_H, logo_soufer);
    // separador
    tft.drawFastHLine(W/2 - 40, H/2 - 20, 80, DIVIDER);
    // ZABBIX MONITOR
    tft.setTextColor(TEXT_DIM, BG);
    tft.drawString("ZABBIX MONITOR", W/2, H/2, 2);
    // Loading
    tft.setTextColor(0x07FF, BG);
    tft.drawString(message, W/2, H/2 + 30, 2);
    tft.setTextDatum(TL_DATUM);
}

void display::showWifiError() {
    lastDrawnIndex = -1;
    tft.fillScreen(BG);
    // Header com logo
    tft.fillRect(0, 0, W, 30, HEADER_BG);
    tft.pushImage(8, 7, LOGO_W, LOGO_H, logo_soufer);
    // Erro
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(0xF800, BG);
    tft.drawString("Sem conexao WiFi", W/2, H/2 - 10, 4);
    tft.setTextColor(TEXT_DIM, BG);
    tft.drawString("Tentando reconectar...", W/2, H/2 + 30, 2);
    tft.setTextDatum(TL_DATUM);
}

void display::showAllClear() {
    lastDrawnIndex = -1;
    tft.fillScreen(BG);
    // Header com logo
    tft.fillRect(0, 0, W, 30, HEADER_BG);
    tft.pushImage(8, 7, LOGO_W, LOGO_H, logo_soufer);

    // Check circle
    int cx = W/2, cy = H/2 - 10;
    tft.drawCircle(cx, cy, 28, GREEN);
    tft.drawCircle(cx, cy, 27, GREEN);
    tft.drawLine(cx-12, cy, cx-4, cy+10, GREEN);
    tft.drawLine(cx-4, cy+10, cx+14, cy-12, GREEN);
    tft.drawLine(cx-12, cy+1, cx-4, cy+11, GREEN);
    tft.drawLine(cx-4, cy+11, cx+14, cy-11, GREEN);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TEXT_W, BG);
    tft.drawString("Sem alertas", cx, cy + 50, 4);
    tft.setTextColor(TEXT_DIM, BG);
    tft.drawString("Todos os sistemas operacionais", cx, cy + 75, 2);
    tft.setTextDatum(TL_DATUM);
}

void display::showAlert(const ZabbixAlert& alert, int index, int total) {
    // Evita redesenhar se nada mudou
    if (index == lastDrawnIndex && total == lastDrawnTotal) return;
    lastDrawnIndex = index;
    lastDrawnTotal = total;

    uint16_t accent = sevColor(alert.severity);
    tft.fillScreen(BG);

    // ===== HEADER (y 0-30) =====
    tft.fillRect(0, 0, W, 30, HEADER_BG);
    // Logo bitmap
    tft.pushImage(8, 7, LOGO_W, LOGO_H, logo_soufer);
    // Wifi dot
    tft.fillCircle(W - 80, 15, 4, GREEN);
    // Alert count
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(0x9CF3, HEADER_BG);
    String countStr = String(total) + " alertas";
    tft.drawString(countStr, W - 72, 8, 1);

    // ===== ACCENT LINE (y 30-33) =====
    tft.fillRect(0, 30, W, 3, accent);

    // ===== SEVERITY + AGE (y 40) =====
    tft.setTextDatum(TL_DATUM);
    tft.fillCircle(14, 48, 5, accent);
    tft.setTextColor(accent, BG);
    tft.drawString(ZabbixClient::severityName(alert.severity), 26, 41, 2);

    String age = fmtAge(alert.ageSeconds);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(TEXT_DIM, BG);
    tft.drawRightString(age, W - 10, 41, 2);

    // ===== DIVIDER =====
    tft.drawFastHLine(10, 62, W - 20, DIVIDER);

    // ===== HOST + ACK (y 70) =====
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TEXT_SEC, BG);
    String host = alert.hostName;
    if (host.length() > 28) host = host.substring(0, 26) + "..";
    tft.drawString(host, 10, 70, 2);

    if (alert.acknowledged) {
        tft.setTextDatum(TR_DATUM);
        tft.setTextColor(GREEN, BG);
        tft.drawRightString("ACK", W - 10, 70, 2);
        tft.setTextDatum(TL_DATUM);
    }

    // ===== PROBLEMA (y 95, fonte grande) =====
    tft.setTextColor(TEXT_W, BG);
    String prob = alert.problemName;

    if (prob.length() <= 22) {
        tft.drawString(prob, 10, 95, 4);
    } else if (prob.length() <= 44) {
        int br = 22;
        for (int i = 22; i > 14; i--) { if (prob.charAt(i) == ' ') { br = i; break; } }
        tft.drawString(prob.substring(0, br), 10, 92, 4);
        String l2 = prob.substring(br + 1);
        if (l2.length() > 24) l2 = l2.substring(0, 22) + "..";
        tft.drawString(l2, 10, 120, 4);
    } else {
        int br = 22;
        for (int i = 22; i > 14; i--) { if (prob.charAt(i) == ' ') { br = i; break; } }
        tft.drawString(prob.substring(0, br), 10, 88, 4);
        String rest = prob.substring(br + 1);
        int br2 = min(22, (int)rest.length());
        for (int i = min(22, (int)rest.length()); i > 14; i--) { if (rest.charAt(i) == ' ') { br2 = i; break; } }
        tft.drawString(rest.substring(0, br2), 10, 114, 4);
        String l3 = rest.substring(br2 + 1);
        if (l3.length() > 24) l3 = l3.substring(0, 22) + "..";
        tft.drawString(l3, 10, 140, 4);
    }

    // ===== DIVIDER LOWER =====
    tft.drawFastHLine(10, 170, W - 20, DIVIDER);

    // ===== DETALHES (y 178) =====
    tft.setTextColor(TEXT_DIM, BG);
    tft.drawString("Inicio:", 10, 178, 1);
    tft.setTextColor(TEXT_SEC, BG);
    // Mostra data de inicio a partir do clock
    time_t startTime = (time_t)(millis()/1000); // simplificado
    tft.drawString(age + " atras", 50, 178, 1);

    tft.setTextColor(TEXT_DIM, BG);
    tft.drawString("Duracao:", 170, 178, 1);
    tft.setTextColor(TEXT_SEC, BG);
    tft.drawString(age, 215, 178, 1);

    // ===== PAGINATION DOTS (y 210) =====
    int footerY = H - 24;
    tft.drawFastHLine(10, footerY - 6, W - 20, DIVIDER);

    int dotStartX = W/2 - (total * 7);
    for (int i = 0; i < total && i < 10; i++) {
        int dx = dotStartX + i * 14;
        if (i == index) {
            // Dot ativo: retangulo arredondado
            tft.fillRoundRect(dx - 8, footerY + 2, 16, 8, 4, accent);
        } else {
            tft.fillCircle(dx, footerY + 6, 3, DIVIDER);
        }
    }

    // Counter
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(TEXT_DIM, BG);
    String counter = String(index + 1) + "/" + String(total);
    tft.drawRightString(counter, W - 10, footerY, 2);
    tft.setTextDatum(TL_DATUM);
}
