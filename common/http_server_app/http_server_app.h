#ifndef __HTTP_SERVER_APP_H
#define __HTTP_SERVER_APP_H

typedef void (*http_post_wifiInfor_callback)(char* ssid, char* password);

typedef void (*http_post_callback)(char* data, int content_length);
typedef void (*http_post_duty_callback)(int duty);
typedef void (*http_get_temp_callback)(int* temp, int* humid);
typedef void (*http_post_rgbColor_callback)(int red, int green, int blue);

void start_webserver(void);
void stop_webserver(void);

void http_set_wifiInfor_callback(void* cb);

void http_set_temp_callback(void* cb);
void http_set_switch_callback(void* cb);
void http_set_duty_callback(void* cb);
void http_set_rgbColor_callback(void* cb);



#endif

