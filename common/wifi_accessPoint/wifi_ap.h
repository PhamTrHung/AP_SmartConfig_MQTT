#ifndef WIFI_AP_H
#define WIFI_AP_H

void wifi_init_softap(char* SSID, char* PASSW);
esp_netif_t* get_ap_netif_wifi();

#endif
