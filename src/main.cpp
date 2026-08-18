#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h"
#include "zabbix_client.h"
#include "display.h"
#include "web_server.h"

// --- Configurações de tempo (ajuste ao seu gosto) ---
const unsigned long POLL_INTERVAL_MS   = 10000; // busca novos alertas a cada 10s
const unsigned long CYCLE_INTERVAL_MS  = 4000;  // troca de alerta na tela a cada 4s

ZabbixAlert alerts[MAX_ALERTS];
int alertCount = 0;
int currentAlertIndex = 0;

unsigned long lastPoll = 0;
unsigned long lastCycle = 0;

void connectWifi() {
    display::showBooting("Conectando ao WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
        delay(300);
    }

    if (WiFi.status() != WL_CONNECTED) {
        display::showWifiError();
    }
}

void syncTime() {
    // Necessário para calcular corretamente "há quanto tempo" o alerta está aberto,
    // já que o Zabbix trabalha com timestamps Unix (UTC).
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    time_t now = time(nullptr);
    int tries = 0;
    while (now < 100000 && tries < 20) { // espera até ter uma hora "real"
        delay(300);
        now = time(nullptr);
        tries++;
    }
}

void pollZabbix() {
    if (WiFi.status() != WL_CONNECTED) return;

    if (!zabbix.login()) {
        Serial.println("Falha ao autenticar no Zabbix");
        return;
    }

    int newCount = zabbix.fetchProblems(alerts, MAX_ALERTS);
    if (newCount != alertCount) {
        alertCount = newCount;
        if (currentAlertIndex >= alertCount) currentAlertIndex = 0;
    } else {
        alertCount = newCount;
    }
    webserver::updateAlerts(alerts, alertCount);
    Serial.printf("Alertas encontrados: %d\n", alertCount);
}

void setup() {
    Serial.begin(115200);
    display::begin();

    connectWifi();
    if (WiFi.status() == WL_CONNECTED) {
        syncTime();
        webserver::begin();
        pollZabbix();
    }
}

void loop() {
    unsigned long now = millis();

    // Reconecta o WiFi se cair
    if (WiFi.status() != WL_CONNECTED) {
        display::showWifiError();
        connectWifi();
        return;
    }

    // Busca novos alertas periodicamente
    if (now - lastPoll >= POLL_INTERVAL_MS) {
        lastPoll = now;
        pollZabbix();
    }

    // Alterna a tela entre os alertas encontrados
    if (alertCount == 0) {
        display::showAllClear();
    } else {
        if (now - lastCycle >= CYCLE_INTERVAL_MS) {
            lastCycle = now;
            currentAlertIndex = (currentAlertIndex + 1) % alertCount;
        }
        display::showAlert(alerts[currentAlertIndex], currentAlertIndex, alertCount);
    }

    delay(200);
}
