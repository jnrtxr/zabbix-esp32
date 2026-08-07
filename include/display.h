#pragma once
#include "zabbix_client.h"

namespace display {
    void begin();
    void showBooting(const char* message);
    void showAllClear();                          // tela verde "tudo ok"
    void showAlert(const ZabbixAlert& alert, int index, int total);
    void showWifiError();
}
