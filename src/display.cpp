#include "display.h"
#include <TFT_eSPI.h>

static TFT_eSPI tft = TFT_eSPI();

// Paleta moderna
#define BG       0x0841   // cinza muito escuro (quase preto, mas nao puro)
#define SURFACE  0x10A2   // superficie elevada sutil
#define TEXT_PRI 0xFFFF   // branco
#define TEXT_SEC 0x9CF3   // cinza claro
#define TEXT_DIM 0x52AA   // cinza medio
#define ACCENT   0x07FF   // cyan
#define DIVIDER  0x2104   // linha divisoria sutil

// Severidade - cores modernas (mais suaves)
static uint16_t severityAccent(int severity) {
    switch (severity) {
        case 0: return 0x7BEF;  // cinza
        case 1: return 0x5D7F;  // azul suave
        case 2: return 0xFE60;  // amarelo quente
        case 3: return 0xFC60;  // laranja
        case 4: return 0xF8A0;  // vermelho coral
        case 5: return 0xF800;  // vermelho puro
        default: return 0x7BEF;
    }
}

static String formatAge(unsigned long seconds) {
    if (seconds < 60) return "<1m";
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    if (days > 0) return String(days) + "d";
    if (hours > 0) return String(hours) + "h";
    return String(minutes) + "m";
}

void display::begin() {
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(BG);
}

void display::showBooting(const char* message) {
    tft.fillScreen(BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(ACCENT, BG);
    tft.drawString("Z", 120, 50, 7); // "Z" grande como logo
    tft.setTextColor(TEXT_SEC, BG);
    tft.drawString(message, 120, 105, 2);
    tft.setTextDatum(TL_DATUM);
}

void display::showWifiError() {
    tft.fillScreen(BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(0xF800, BG);
    tft.drawString("WiFi", 120, 50, 4);
    tft.setTextColor(TEXT_DIM, BG);
    tft.drawString("desconectado", 120, 90, 2);
    tft.setTextDatum(TL_DATUM);
}

void display::showAllClear() {
    tft.fillScreen(BG);

    // Circulo de sucesso grande
    tft.fillCircle(120, 52, 22, 0x0400); // fundo verde escuro
    tft.drawCircle(120, 52, 22, 0x07E0);
    // Checkmark simples
    tft.drawLine(110, 52, 117, 60, 0x07E0);
    tft.drawLine(117, 60, 132, 44, 0x07E0);
    tft.drawLine(110, 53, 117, 61, 0x07E0);
    tft.drawLine(117, 61, 132, 45, 0x07E0);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(0x07E0, BG);
    tft.drawString("Sem alertas", 120, 95, 2);
    tft.setTextColor(TEXT_DIM, BG);
    tft.drawString("Todos os sistemas OK", 120, 118, 1);
    tft.setTextDatum(TL_DATUM);
}

void display::showAlert(const ZabbixAlert& alert, int index, int total) {
    uint16_t accent = severityAccent(alert.severity);

    tft.fillScreen(BG);

    // --- Top: indicador fino de severidade (2px no topo) ---
    tft.fillRect(0, 0, 240, 2, accent);

    // --- Severidade + tempo (linha superior) ---
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(accent, BG);
    // Dot de severidade
    tft.fillCircle(8, 14, 4, accent);
    // Texto da severidade
    tft.drawString(ZabbixClient::severityName(alert.severity), 18, 9, 2);

    // Tempo - direita
    String age = formatAge(alert.ageSeconds);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(TEXT_DIM, BG);
    tft.drawRightString(age, 235, 9, 2);

    // --- Linha divisoria sutil ---
    tft.drawFastHLine(0, 28, 240, DIVIDER);

    // --- Host (fonte pequena, cor secundaria) ---
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TEXT_SEC, BG);
    String host = alert.hostName;
    if (host.length() > 26) host = host.substring(0, 24) + "..";
    tft.drawString(host, 6, 34, 2);

    // ACK badge (se reconhecido)
    if (alert.acknowledged) {
        tft.setTextDatum(TR_DATUM);
        tft.setTextColor(0x07E0, BG);
        tft.drawRightString("ACK", 235, 34, 1);
        tft.setTextDatum(TL_DATUM);
    }

    // --- Problema (fonte maior, branco, area principal) ---
    tft.setTextColor(TEXT_PRI, BG);
    String prob = alert.problemName;

    // Calcula quebra de linha se necessario
    if (prob.length() <= 24) {
        tft.drawString(prob, 6, 56, 2);
    } else {
        // Quebra em 2 linhas
        int breakAt = 24;
        // Tenta quebrar num espaco
        for (int i = 24; i > 15; i--) {
            if (prob.charAt(i) == ' ') { breakAt = i; break; }
        }
        String line1 = prob.substring(0, breakAt);
        String line2 = prob.substring(breakAt + 1);
        if (line2.length() > 26) line2 = line2.substring(0, 24) + "..";
        tft.drawString(line1, 6, 54, 2);
        tft.drawString(line2, 6, 72, 2);
    }

    // --- Footer: progress dots / counter ---
    int footerY = 120;
    tft.drawFastHLine(0, footerY - 6, 240, DIVIDER);

    // Mini dots para indicar posicao (estilo pagination moderna)
    int dotStartX = 120 - (total * 5);
    for (int i = 0; i < total && i < 10; i++) {
        int dx = dotStartX + i * 10;
        if (i == index) {
            tft.fillCircle(dx, footerY + 5, 3, accent);
        } else {
            tft.fillCircle(dx, footerY + 5, 2, DIVIDER);
        }
    }

    // Counter discreto na direita
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(TEXT_DIM, BG);
    String counter = String(index + 1) + "/" + String(total);
    tft.drawRightString(counter, 235, footerY, 1);
    tft.setTextDatum(TL_DATUM);
}
