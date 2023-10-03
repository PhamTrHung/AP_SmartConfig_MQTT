#include "app_config.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi_ap.h"
#include "smartconfig_lib.h"

static const char *TAG = "app_config";

provisoned_type_t provisioned_type = PROVISIONED_ACCESSPOINT;

bool is_provisioned(void)
{

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_config;
    esp_wifi_get_config(WIFI_IF_STA, &wifi_config);

    if(wifi_config.sta.ssid[0] != 0)
    {
        return 1;
    }

    return 0;
}


void app_config(void)
{
    bool checkWifi = is_provisioned();

    

    if(checkWifi == 0)
    {
        esp_wifi_deinit();
        if(provisioned_type == PROVISIONED_ACCESSPOINT)
        {
            wifi_init_softap("PhamHung", "123456789");
        }
        else if(provisioned_type == PROVISIONED_SMARTCONFIG)
        {
            ESP_LOGI(TAG, "ESP is not be configed");
            smartconfig_start();
        }
    }
    else
    {
        ESP_LOGI(TAG, "ESP has been configed");
        ESP_ERROR_CHECK(esp_wifi_start());
    }

}

