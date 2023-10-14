#include "http_server_app.h"

#include <string.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"


#include <esp_http_server.h>
#include "dht11.h"


http_post_wifiInfor_callback http_post_wifiInfor_cb;


/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */
static const char *TAG = "http server";
static httpd_handle_t server = NULL;


extern const uint8_t loginWeb_html_start[] asm("_binary_loginWeb_html_start");
extern const uint8_t loginWeb_html_end[] asm("_binary_loginWeb_html_end");

extern const uint8_t mainWeb_html_start[] asm("_binary_mainWeb_html_start");
extern const uint8_t mainWeb_html_end[] asm("_binary_mainWeb_html_end");


http_post_callback http_post_switch_callback;
http_get_temp_callback http_get_temp_cb;
http_post_duty_callback http_post_duty_cb;
http_post_rgbColor_callback http_post_rgbColor_cb;


/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "api.thingspeak.com"
#define WEB_PORT "80"
#define WEB_PATH "/"

static const char *TAG2 = "httpClient";

char request[512];
char subrequest[200];
char recv_buf[512];

int tempp, humidd;



/* An HTTP GET handler */
static esp_err_t hello_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)loginWeb_html_start, loginWeb_html_end - loginWeb_html_start);
    // const char* resp_str = (const char*) req->user_ctx;
    // httpd_resp_send(req, resp_str, strlen(resp_str));

    return ESP_OK;
}

static const httpd_uri_t get_data = {
    .uri       = "/login",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "Hello ca the gioi!"
};

/* An HTTP GET handler */
static esp_err_t mainWeb_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)mainWeb_html_start, mainWeb_html_end - mainWeb_html_start);
    // const char* resp_str = (const char*) req->user_ctx;
    // httpd_resp_send(req, resp_str, strlen(resp_str));

    return ESP_OK;
}

static const httpd_uri_t main_web = {
    .uri       = "/main",
    .method    = HTTP_GET,
    .handler   = mainWeb_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "Hello ca the gioi!"
};

/* An HTTP GET handler */
static esp_err_t get_dht11_handler(httpd_req_t* req)
{
    char buff[100];
    int temp, humid;

    http_get_temp_cb(&temp, &humid);

    sprintf(buff, "{\"temperature\": \"%d\", \"humidity\": \"%d\"}", temp, humid);

    httpd_resp_send(req, buff, strlen(buff));
    return ESP_OK;
}

static const httpd_uri_t get_data_dht11 = {
    .uri       = "/getDataDHT11",
    .method    = HTTP_GET,
    .handler   = get_dht11_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

/* An HTTP POST handler */
static esp_err_t wifi_infor_data(httpd_req_t *req)
{
    char buf[100];
    char ssid[32], passw[64];
    char* token;
    uint8_t flag_cpl = 0;
    

    httpd_req_recv(req, buf, req->content_len);

    token = strtok(buf, "&");
    while(flag_cpl < 2)
    {
        if(flag_cpl == 0)
        {
            strcpy(ssid, token);
            flag_cpl++;
        }
        else if (flag_cpl == 1)
        {
            strcpy(passw, token);
            flag_cpl++;
        }
        token = strtok(NULL, "&");
    }

    if(strlen(ssid) != 0 && strlen(passw) != 0)
    {
        http_post_wifiInfor_cb(ssid, passw);
    }
    

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t wifi_infor = {
    .uri       = "/wifiInfor",
    .method    = HTTP_POST,
    .handler   = wifi_infor_data,
    .user_ctx  = NULL
};

/* An HTTP POST handler */
static esp_err_t change_led_state(httpd_req_t *req)
{
    char buf[100];

    httpd_req_recv(req, buf, req->content_len);

    http_post_switch_callback(buf, req->content_len);

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t switch_led = {
    .uri       = "/switchled",
    .method    = HTTP_POST,
    .handler   = change_led_state,
    .user_ctx  = NULL
};


/* An HTTP POST handler */
static esp_err_t change_duty(httpd_req_t *req)
{
    char buf[100];
    int num = 0;

    httpd_req_recv(req, buf, req->content_len);
    
    for(int i=0;i<3;i++)
    {
        if(buf[i] >= '0' && buf[i] <= '9')
        {
            num *= 10;
            num += buf[i] - '0';
        }
    }

    http_post_duty_cb(num);

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t duty_post = {
    .uri       = "/changeDuty",
    .method    = HTTP_POST,
    .handler   = change_duty,
    .user_ctx  = NULL
};



esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}



/* An HTTP POST handler */
static int hex4_ToDecimal(char c)
{
   if(c >= '0' && c <= '9')
   {
       return c - '0';
   }
   else if(c >= 'A' && c <= 'F')
   {
       return c - 'A' + 10;
   }
   else if(c >= 'a' && c <= 'f')
   {
       return c - 'a' + 10;
   }

   return 0;
}

static esp_err_t change_rgb_led_color(httpd_req_t *req)
{
    char buf[100];
    int rgb[3];

    httpd_req_recv(req, buf, req->content_len);

    for(int i = 0; i < 3; i++)
    {
        rgb[i] = 16*hex4_ToDecimal(buf[i*2 + 1]) + hex4_ToDecimal(buf[i*2 + 2]);
    }
    
    http_post_rgbColor_cb(rgb[0], rgb[1], rgb[2]);

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t rbg_color = {
    .uri       = "/rgbColor",
    .method    = HTTP_POST,
    .handler   = change_rgb_led_color,
    .user_ctx  = NULL
};


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
            ESP_LOGE(TAG2, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG2, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG2, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG2, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG2, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG2, "... connected");
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
            ESP_LOGE(TAG2, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG2, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG2, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG2, "... set socket receiving timeout success");

        /* Read HTTP response */
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                putchar(recv_buf[i]);
            }
        } while(r > 0);

        ESP_LOGI(TAG2, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        close(s);
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG2, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG2, "Starting again!");
    }
}


void start_webserver(void)
{      

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &get_data);
        httpd_register_uri_handler(server, &wifi_infor);
        httpd_register_uri_handler(server, &main_web);

        httpd_register_uri_handler(server, &get_data_dht11);
        httpd_register_uri_handler(server, &switch_led);
        httpd_register_uri_handler(server, &duty_post);
        httpd_register_uri_handler(server, &rbg_color);

        xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
    }
    else
    {
        ESP_LOGI(TAG, "Error starting server!");
    }
}

void stop_webserver(void)
{
    // Stop the httpd server
    httpd_stop(server);
}


void http_set_wifiInfor_callback(void* cb)
{
    http_post_wifiInfor_cb = cb;
}


void http_set_temp_callback(void* cb)
{
    http_get_temp_cb = cb;
}

void http_set_switch_callback(void* cb)
{
    http_post_switch_callback = cb;
}

void http_set_duty_callback(void* cb)
{
    http_post_duty_cb = cb;
}

void http_set_rgbColor_callback(void* cb)
{
    http_post_rgbColor_cb = cb;
}



