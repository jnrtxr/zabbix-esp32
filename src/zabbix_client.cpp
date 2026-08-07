#include "zabbix_client.h"
#include "secrets.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

ZabbixClient zabbix;

// Faz a chamada HTTP POST JSON-RPC e devolve o corpo da resposta como String.
// Usa Bearer token quando disponível (Zabbix 5.4+), senão manda "auth" no corpo.
String ZabbixClient::jsonRpcRequest(const String& body) {
    HTTPClient http;
    http.begin(ZABBIX_API_URL);
    http.addHeader("Content-Type", "application/json-rpc");

    bool haveToken = strlen(ZABBIX_API_TOKEN) > 0;
    if (haveToken) {
        http.addHeader("Authorization", String("Bearer ") + ZABBIX_API_TOKEN);
    }

    int httpCode = http.POST(body);
    String response = "";
    if (httpCode == 200) {
        response = http.getString();
    } else {
        Serial.printf("[Zabbix] Erro HTTP: %d\n", httpCode);
    }
    http.end();
    return response;
}

bool ZabbixClient::login() {
    // Se já tem API token configurado, não precisa fazer login.
    if (strlen(ZABBIX_API_TOKEN) > 0) {
        usingApiToken = true;
        return true;
    }

    // Fallback: login clássico com usuário/senha.
    StaticJsonDocument<512> req;
    req["jsonrpc"] = "2.0";
    req["method"] = "user.login";
    JsonObject params = req.createNestedObject("params");
    params["username"] = ZABBIX_USER;
    params["password"] = ZABBIX_PASS;
    req["id"] = 1;

    String body;
    serializeJson(req, body);
    String response = jsonRpcRequest(body);

    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        Serial.println("[Zabbix] Falha ao decodificar resposta de login");
        return false;
    }
    if (doc.containsKey("result")) {
        authToken = doc["result"].as<String>();
        return true;
    }
    Serial.println("[Zabbix] Login falhou (usuário/senha incorretos?)");
    return false;
}

int ZabbixClient::fetchProblems(ZabbixAlert alerts[], int maxAlerts) {
    StaticJsonDocument<768> req;
    req["jsonrpc"] = "2.0";
    req["method"] = "problem.get";

    JsonObject params = req.createNestedObject("params");
    params["output"] = "extend";
    params["recent"] = false;          // false = só problemas ainda abertos
    params["sortfield"][0] = "eventid";
    params["sortorder"] = "DESC";
    params["limit"] = maxAlerts;

    // Nota: "selectHosts" não existe em problem.get no Zabbix 7.0+.
    // O hostname será obtido via eventid/objectid se necessário no futuro.

    if (!usingApiToken) {
        req["auth"] = authToken;
    }
    req["id"] = 2;

    String body;
    serializeJson(req, body);
    String response = jsonRpcRequest(body);

    // O JSON de resposta pode ser grande dependendo de quantos problemas existem.
    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        Serial.print("[Zabbix] Falha ao decodificar problem.get: ");
        Serial.println(err.c_str());
        return 0;
    }

    if (doc.containsKey("error")) {
        Serial.print("[Zabbix] Erro da API: ");
        Serial.println(doc["error"]["data"].as<String>());
        return 0;
    }

    JsonArray result = doc["result"].as<JsonArray>();
    int count = 0;
    unsigned long now = millis() / 1000; // aproximação; ver nota no README sobre NTP

    for (JsonObject problem : result) {
        if (count >= maxAlerts) break;

        ZabbixAlert& a = alerts[count];
        a.problemName = problem["name"].as<String>();
        a.severity = problem["severity"].as<int>();
        a.acknowledged = problem["acknowledged"].as<int>() == 1;

        long clock = problem["clock"].as<long>();
        // "clock" vem em epoch Unix (segundos). Calculado corretamente
        // quando o ESP32 sincroniza a hora via NTP (ver setup() no main.cpp).
        time_t nowEpoch;
        time(&nowEpoch);
        a.ageSeconds = (unsigned long)(nowEpoch - clock);

        // No Zabbix 7.0+ sem selectHosts, o campo "hosts" não vem em problem.get.
        // Usamos o opdata se disponível, senão marcamos como desconhecido.
        if (problem.containsKey("hosts")) {
            JsonArray hosts = problem["hosts"].as<JsonArray>();
            if (hosts.size() > 0) {
                a.hostName = hosts[0]["name"].as<String>();
            } else {
                a.hostName = "?";
            }
        } else {
            a.hostName = "?";
        }

        count++;
    }

    return count;
}

const char* ZabbixClient::severityName(int severity) {
    switch (severity) {
        case 0: return "Nao classificado";
        case 1: return "Informacao";
        case 2: return "Atencao";
        case 3: return "Media";
        case 4: return "Alta";
        case 5: return "Desastre";
        default: return "Desconhecido";
    }
}
