#pragma once
#include <Arduino.h>

#define MAX_ALERTS 10

struct ZabbixAlert {
    String hostName;
    String problemName;
    int severity;      // 0..5 (Zabbix: 0=Not classified ... 5=Disaster)
    unsigned long ageSeconds;
    bool acknowledged;
};

class ZabbixClient {
public:
    // Faz login (só necessário se não estiver usando API token).
    // Retorna true se conseguiu autenticar.
    bool login();

    // Busca os problemas ativos. Preenche 'alerts' e retorna a quantidade encontrada.
    int fetchProblems(ZabbixAlert alerts[], int maxAlerts);

    // Nome legível da severidade (em português)
    static const char* severityName(int severity);

private:
    String authToken = ""; // preenchido por login(), vazio = usa ZABBIX_API_TOKEN direto
    bool usingApiToken = false;

    String jsonRpcRequest(const String& body);
};

extern ZabbixClient zabbix;
