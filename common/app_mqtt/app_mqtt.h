#ifndef __APP_MQTT_H
#define __APP_MQTT_H

#include "esp_event.h"

typedef void (*mqtt_get_data_callback)(char* data, int len);

ESP_EVENT_DECLARE_BASE(MQTT_DEV_EVENT);

enum{
    MQTT_DEV_EVENT_CONNECTED,
    MQTT_DEV_EVENT_DISCONNECT,
    MQTT_DEV_EVENT_DATA,
    MQTT_DEV_EVENT_SUBSCRIBED,
    MQTT_DEV_EVENT_UNSUBSCRIBED,
    MQTT_DEV_EVENT_PUBLISHED,
};

void mqtt_app_start(void);
void mqtt_set_data_callback(void* cb);
void app_mqtt_publish(char* topic, char* data, int len);
void app_mqtt_subscribe(char* topic);

#endif
