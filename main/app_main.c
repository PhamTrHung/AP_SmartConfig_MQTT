#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "app_mqtt.h"
#include "app_config.h"
#include "smartconfig_lib.h"
#include "wifi_ap.h"
#include "wifi_station.h"
#include "http_server_app.h"
#include "output_iot.h"
#include "dht11.h"
#include "ledc_mode.h"
#include "simple_ota.h"
#include "gatt_server.h"



static const char *TAG = "IN MAIN";

// static char* ssid = "Pham Trong Thuy";
// static char* passw = "06022001"; 


#define MYLED 2
#define RED_CHANNEL 0
#define GREEN_CHANNEL 1
#define BLUE_CHANNEL 2
#define RED_PIN 4
#define GREEN_PIN 16
#define BLUE_PIN 17
#define DHT11_PIN 19
#define LED_PWM 21
#define LED_CHANNEL 3


#define GATTS_TAG "GATTS_DEMO_IN_MAIN"

// Con trỏ nhận giá trị trả về từ esp_netif_create_default_wifi_ap();
esp_netif_t* ap;


static void event_handle(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)

{
    if(event_base == MQTT_DEV_EVENT)
    {
        if(event_id == MQTT_DEV_EVENT_CONNECTED)
        {
            ESP_LOGW(TAG, "MQTT_DEV_EVENT_CONNECTED");
        }
        else if(event_id == MQTT_DEV_EVENT_DISCONNECT)
        {
            ESP_LOGW(TAG, "MQTT_DEV_EVENT_DISCONNECT");
        }
        else if(event_id == MQTT_DEV_EVENT_SUBSCRIBED)
        {
            ESP_LOGW(TAG, "MQTT_DEV_EVENT_SUBSCRIBED");
        }
        else if(event_id == MQTT_DEV_EVENT_UNSUBSCRIBED)
        {
            ESP_LOGW(TAG, "MQTT_DEV_EVENT_UNSUBSCRIBED");
        }
        else if(event_id == MQTT_DEV_EVENT_DATA)
        {
            ESP_LOGW(TAG, "MQTT_DEV_EVENT_DATA");
        }

    }
}



void handle_data_switch(char* buf, int length)
{
    if(strstr(buf,"on"))
    {
        output_io_set_level(MYLED, 1);
    }
    else if(strstr(buf,"off"))
    {
        output_io_set_level(MYLED, 0);
    }

}

void handle_data_dht11(int* temp, int* humid)
{
    *temp = DHT11_read().temperature; 
    *humid = DHT11_read().humidity;
}

void handle_data_duty(int duty)
{
    printf("%d\n", duty);
    ledc_create_duty(LED_CHANNEL,duty);
}

void handle_data_rgb_color(int red, int green, int blue)
{
    printf("%d %d %d\n", red, green, blue);
    ledc_create_duty(RED_CHANNEL, red);
    ledc_create_duty(GREEN_CHANNEL, green);
    ledc_create_duty(BLUE_CHANNEL, blue);
}

void http_handle_wifiInfor_data_cb(char* ssid, char* passw)
{

    ESP_ERROR_CHECK(esp_wifi_stop());

    ap = get_ap_netif_wifi();
    esp_netif_destroy(ap);

    printf("%s\n%s\n", ssid, passw);

    wifi_init_sta(ssid, passw);

    printf("Connect to access point successly");
    
}




void gatts_handle_write_event_callback(uint8_t* revdata, uint16_t length)
{
    printf("%s, %d\n", revdata, length);
    esp_log_buffer_hex(GATTS_TAG, revdata, length);
}

void mqtt_handle_data_callback(char* dta, int len)
{
    if(strstr(dta, "on"))
    {
        output_io_set_level(2, 1);
    }
    else if(strstr(dta, "off"))
    {
        output_io_set_level(2, 0);
    }
    printf("Led state = %s\n", dta);
    
}

void app_main(void)
{
    output_io_config(MYLED);
    ledc_config(RED_PIN, 0);
    ledc_config(GREEN_PIN, 1);
    ledc_config(BLUE_PIN, 2);
    ledc_config(LED_PWM, 3);
    DHT11_init(DHT11_PIN);


    http_set_switch_callback(&handle_data_switch);
    http_set_temp_callback(&handle_data_dht11);
    http_set_duty_callback(&handle_data_duty);
    http_set_rgbColor_callback(&handle_data_rgb_color);
    http_set_wifiInfor_callback(&http_handle_wifiInfor_data_cb);

    mqtt_set_data_callback(&mqtt_handle_data_callback);

    // gatt_server_init();
    // gatts_read_event((uint8_t*)"Hello World", 12);
    // gatts_notify_event((uint8_t*)"123456789", 9);

    // gatt_server_set_write_event_callback(&gatts_handle_write_event_callback);


    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    ESP_ERROR_CHECK(esp_event_handler_register(MQTT_DEV_EVENT, ESP_EVENT_ANY_ID, &event_handle, NULL));
    
    app_config();

    //start_webserver();    
    
    // ESP_LOGI(TAG, "[APP] Startup..");
    // ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    // ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    mqtt_app_start();

    //ota_update();
}


