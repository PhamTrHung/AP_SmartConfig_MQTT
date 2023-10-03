#include "http_server_app.h"

#include <string.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>

#include <esp_http_server.h>

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



