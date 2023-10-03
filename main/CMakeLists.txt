idf_component_register(
SRCS "app_main.c" 
SRCS "app_config.c"
SRCS "../common/smartconfig_lib/smartconfig_lib.c"
SRCS "../common/wifi_accessPoint/wifi_ap.c"
SRCS "../common/http_server_app/http_server_app.c"
SRCS "../common/wifi_station/wifi_station.c"
SRCS    "../common/dht11/dht11.c"
SRCS    "../common/output_iot/output_iot.c"  
SRCS    "../common/ledc_lib/ledc_mode.c"
INCLUDE_DIRS ../common/smartconfig_lib
INCLUDE_DIRS ../common/wifi_accessPoint
INCLUDE_DIRS ../common/http_server_app
INCLUDE_DIRS ../common/wifi_station
INCLUDE_DIRS ../common/dht11
INCLUDE_DIRS ../common/output_iot
INCLUDE_DIRS ../common/ledc_lib
INCLUDE_DIRS ".")

