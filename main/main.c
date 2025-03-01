#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_random.h"
#include "menu.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_wifi_types.h"
#include "hal/efuse_hal.h"
#include "esp_mac.h"
#include "esp_chip_info.h"
#include "web_server.h"
#include "esp_netif.h"
#include "lwip/ip4_addr.h"
#include "board_config.h"

// Define MACSTR and MAC2STR
#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

static const char* TAG = "esp32_c5_app";

// WiFi configuration - using values from board_config.h
#define CONFIG_ESP_WIFI_SSID "ESP32-C5-J4CK3D"
#define CONFIG_ESP_WIFI_PASSWORD "h4ck3rm4n"
#define CONFIG_ESP_WIFI_CHANNEL DEFAULT_WIFI_CHANNEL
#define CONFIG_ESP_MAX_STA_CONN 4

static esp_netif_t *ap_netif = NULL;

// WiFi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_STACONNECTED:
                {
                    wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
                    ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d",
                            MAC2STR(event->mac), event->aid);
                }
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                {
                    wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
                    ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d",
                            MAC2STR(event->mac), event->aid);
                }
                break;
            default:
            break;
        }
    }
}

// Initialize WiFi in AP mode
static void wifi_init_ap(void) {
    // Initialize default event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Create default AP netif instance
    ap_netif = esp_netif_create_default_wifi_ap();
    
    // Get the default IP info
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(ap_netif, &ip_info);
    
    // Initialize WiFi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    
    // Configure AP settings
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = CONFIG_ESP_WIFI_SSID,
            .ssid_len = strlen(CONFIG_ESP_WIFI_SSID),
            .password = CONFIG_ESP_WIFI_PASSWORD,
            .max_connection = CONFIG_ESP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .channel = CONFIG_ESP_WIFI_CHANNEL
        },
    };
    
    if (strlen(CONFIG_ESP_WIFI_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    
    // Set mode and config
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    
    // Check for antenna setting in NVS
    nvs_handle_t nvs_handle;
    uint8_t use_external = 0;
    esp_err_t ret = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (ret == ESP_OK) {
        nvs_get_u8(nvs_handle, "ext_antenna", &use_external);
        nvs_close(nvs_handle);
    }
    
    // Configure antenna based on NVS setting
    // Note: ESP32-C5 uses different antenna configuration methods
    // For simplicity, we'll just log the antenna setting but not try to set it
    if (use_external) {
        ESP_LOGI(TAG, "Using external antenna (from NVS)");
        // ESP32-C5 doesn't appear to support antenna selection directly
        // via the same API as other ESP chips
    } else {
        // Fall back to config.h setting if no NVS setting
#if USE_EXTERNAL_ANTENNA
        ESP_LOGI(TAG, "Using external antenna (from config)");
#else
        ESP_LOGI(TAG, "Using internal antenna");
#endif
    }
    
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi AP initialized. SSID:%s password:%s channel:%d",
             CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD, CONFIG_ESP_WIFI_CHANNEL);
    
    ESP_LOGI(TAG, "AP IP address: " IPSTR, IP2STR(&ip_info.ip));
}

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Print system info
    ESP_LOGI(TAG, "Starting ESP32-C5 J4CK3D on %s", BOARD_NAME);
    ESP_LOGI(TAG, "IDF Version: %s", esp_get_idf_version());
    
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "Chip model: %d, cores: %d", chip_info.model, chip_info.cores);
        
    // Initialize WiFi
    wifi_init_ap();
    
    // Initialize and start web server
    init_web_server();
    
    // Display access instructions
    ESP_LOGI(TAG, "Web interface is now available at http://192.168.4.1");
    ESP_LOGI(TAG, "Connect to WiFi SSID: %s, Password: %s", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
} 