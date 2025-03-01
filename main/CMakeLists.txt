idf_component_register(
    SRCS "main.c" "menu.c" "web_server.c" "wifi_init.c" "wifi_sniffer.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_system esp_wifi nvs_flash esp_netif esp_http_server json
) 