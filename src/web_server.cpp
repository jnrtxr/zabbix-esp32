#include "web_server.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

static AsyncWebServer server(80);
static ZabbixAlert cachedAlerts[MAX_ALERTS];
static int cachedCount = 0;

// Gera a cor CSS de acordo com a severidade do Zabbix
static const char* severityColor(int sev) {
    switch (sev) {
        case 0: return "#97AAB3"; // Nao classificado
        case 1: return "#7499FF"; // Informacao
        case 2: return "#FFC859"; // Atencao
        case 3: return "#FFA059"; // Media
        case 4: return "#E97659"; // Alta
        case 5: return "#E45959"; // Desastre
        default: return "#CCCCCC";
    }
}

static String formatAge(unsigned long seconds) {
    if (seconds < 60) return String(seconds) + "s";
    if (seconds < 3600) return String(seconds / 60) + "m";
    if (seconds < 86400) return String(seconds / 3600) + "h " + String((seconds % 3600) / 60) + "m";
    return String(seconds / 86400) + "d " + String((seconds % 86400) / 3600) + "h";
}

static String buildPage() {
    String html = R"rawhtml(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta http-equiv="refresh" content="15">
<title>Zabbix Monitor - ESP32</title>
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }
  body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #1a1a2e; color: #eee; padding: 16px; }
  h1 { text-align: center; margin-bottom: 16px; font-size: 1.4em; color: #00d4aa; }
  .info { text-align: center; margin-bottom: 12px; font-size: 0.85em; color: #888; }
  .no-alerts { text-align: center; padding: 40px; font-size: 1.2em; color: #00d4aa; }
  .alert-card { background: #16213e; border-radius: 8px; padding: 12px 16px; margin-bottom: 10px; border-left: 5px solid; }
  .alert-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 6px; }
  .severity { font-size: 0.75em; padding: 2px 8px; border-radius: 4px; color: #fff; font-weight: bold; }
  .host { font-size: 0.85em; color: #aaa; }
  .problem { font-size: 0.95em; margin-top: 4px; }
  .meta { font-size: 0.75em; color: #777; margin-top: 6px; }
  .ack { color: #00d4aa; }
</style>
</head>
<body>
<h1>Zabbix Alertas Ativos</h1>
<div class="info">Auto-refresh: 15s | IP: )rawhtml";

    html += WiFi.localIP().toString();
    html += R"rawhtml(</div>)rawhtml";

    if (cachedCount == 0) {
        html += "<div class=\"no-alerts\">Nenhum alerta ativo</div>";
    } else {
        for (int i = 0; i < cachedCount; i++) {
            ZabbixAlert& a = cachedAlerts[i];
            const char* color = severityColor(a.severity);
            html += "<div class=\"alert-card\" style=\"border-left-color: ";
            html += color;
            html += "\">";
            html += "<div class=\"alert-header\">";
            html += "<span class=\"severity\" style=\"background: ";
            html += color;
            html += "\">";
            html += ZabbixClient::severityName(a.severity);
            html += "</span>";
            html += "<span class=\"host\">";
            html += a.hostName;
            html += "</span></div>";
            html += "<div class=\"problem\">";
            html += a.problemName;
            html += "</div>";
            html += "<div class=\"meta\">Duracao: ";
            html += formatAge(a.ageSeconds);
            if (a.acknowledged) {
                html += " | <span class=\"ack\">ACK</span>";
            }
            html += "</div></div>";
        }
    }

    html += "</body></html>";
    return html;
}

namespace webserver {

void begin() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", buildPage());
    });

    server.on("/api/alerts", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "[";
        for (int i = 0; i < cachedCount; i++) {
            if (i > 0) json += ",";
            json += "{\"host\":\"" + cachedAlerts[i].hostName + "\"";
            json += ",\"problem\":\"" + cachedAlerts[i].problemName + "\"";
            json += ",\"severity\":" + String(cachedAlerts[i].severity);
            json += ",\"age\":" + String(cachedAlerts[i].ageSeconds);
            json += ",\"ack\":" + String(cachedAlerts[i].acknowledged ? "true" : "false");
            json += "}";
        }
        json += "]";
        request->send(200, "application/json", json);
    });

    server.begin();
    Serial.printf("[Web] Servidor HTTP em http://%s/\n", WiFi.localIP().toString().c_str());
}

void updateAlerts(ZabbixAlert alerts[], int count) {
    cachedCount = count > MAX_ALERTS ? MAX_ALERTS : count;
    for (int i = 0; i < cachedCount; i++) {
        cachedAlerts[i] = alerts[i];
    }
}

} // namespace webserver
