#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "app_config.h"
#include "smartconfig_lib.h"
#include "wifi_ap.h"
#include "wifi_station.h"
#include "http_server_app.h"
#include "output_iot.h"
#include "dht11.h"
#include "ledc_mode.h"

//static const char *TAG = "MQTTS_EXAMPLE";

// static char* ssid = "Pham Trong Thuy";
// static char* passw = "06022001"; 


/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "api.thingspeak.com"
#define WEB_PORT "80"
#define WEB_PATH "/"

static const char *TAG = "httpClient";

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


char ssid[32] = "PhamHung202";
char passw[64] = "01234567";

esp_netif_t* ap;


char request[512];
char subrequest[200];
char recv_buf[512];

int tempp, humidd;

static void http_get_task(void *pvParameters)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    //char recv_buf[64];

    while(1) {
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);

        //sprintf(subrequest, "api_key=5KQXIMNVK5M4CF4R&field1=%d",3100);
        //sprintf(request, "POST /update.json HTTP/1.1\nHost: api.thingspeak.com\nConnection: close\nContent-Type: application/x-www-form-urlencoded\nContent-Length:%d\n\n%s\n", strlen(subrequest), subrequest);

        if(DHT11_read().status == DHT11_OK)
        {
            tempp = DHT11_read().temperature;
            humidd = DHT11_read().humidity;
        }

        sprintf(request, "GET http://api.thingspeak.com/update?api_key=5KQXIMNVK5M4CF4R&field1=%d&field2=%d\n\n", tempp, humidd);

        //sprintf(request, "GET https://api.thingspeak.com/channels/2254350/feeds.json?api_key=AS5EN0WG6ADIMH78&results=2\n\n");

        if (write(s, request, strlen(request)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... set socket receiving timeout success");

        /* Read HTTP response */
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                putchar(recv_buf[i]);
            }
        } while(r > 0);

        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        close(s);
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");
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
    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
    
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

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    http_set_wifiInfor_callback(http_handle_wifiInfor_data_cb);
    
    
    app_config();


    start_webserver();    
    
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

}
