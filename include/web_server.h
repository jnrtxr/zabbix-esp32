#pragma once
#include "zabbix_client.h"

namespace webserver {
    // Inicializa o servidor HTTP na porta 80.
    void begin();

    // Atualiza os dados exibidos na página web.
    void updateAlerts(ZabbixAlert alerts[], int count);
}
