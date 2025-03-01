#include "wifi_sniffer.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "wifi_sniffer";

// Maximum number of packets to store
#define MAX_PACKETS_QUEUE 32
#define MAX_PACKET_SIZE 1024

// Structure to hold packet info
typedef struct {
    uint8_t data[MAX_PACKET_SIZE];
    uint16_t length;
    int8_t rssi;
    uint8_t channel;
    wifi_pkt_rx_ctrl_t rx_ctrl;
} packet_info_t;

// Global variables
static QueueHandle_t packet_queue = NULL;
static SemaphoreHandle_t sniffer_running_mutex = NULL;
static bool is_sniffer_running = false;
static uint8_t current_channel = 0;
static uint8_t current_filter = 0;
static TaskHandle_t channel_hopper_task_handle = NULL;

// Channel hopping settings
#define CHANNEL_HOP_INTERVAL_MS 200
static const uint8_t channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
static const uint8_t channels_count = sizeof(channels)/sizeof(channels[0]);

// Forward declaration
static void wifi_sniffer_packet_handler(void *buf, wifi_promiscuous_pkt_type_t type);
static void channel_hopper_task(void *pvParameters);
static void single_channel_retry_task(void *pvParameters);

// Start WiFi sniffer
bool start_wifi_sniffer(uint8_t channel, uint8_t filter_type) {
    ESP_LOGI(TAG, "Starting WiFi sniffer on channel %d with filter type %d", channel, filter_type);
    
    // Create mutex if not already created
    if (sniffer_running_mutex == NULL) {
        sniffer_running_mutex = xSemaphoreCreateMutex();
        if (sniffer_running_mutex == NULL) {
            ESP_LOGI(TAG, "Failed to create sniffer mutex");
            return false;
        }
    }
    
    // Take mutex
    if (xSemaphoreTake(sniffer_running_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGI(TAG, "Failed to take sniffer mutex");
        return false;
    }
    
    // If sniffer is already running, stop it first
    if (is_sniffer_running) {
        ESP_LOGW(TAG, "Sniffer already running, stopping first");
        stop_wifi_sniffer();
    }
    
    // Create packet queue if not already created
    if (packet_queue == NULL) {
        packet_queue = xQueueCreate(MAX_PACKETS_QUEUE, sizeof(packet_info_t*));
        if (packet_queue == NULL) {
            ESP_LOGI(TAG, "Failed to create packet queue");
            xSemaphoreGive(sniffer_running_mutex);
            return false;
        }
    } else {
        // Queue exists, make sure it's empty
        packet_info_t *packet;
        while (xQueueReceive(packet_queue, &packet, 0) == pdTRUE) {
            if (packet) free(packet);
        }
    }
    
    // Save configuration
    current_channel = channel;
    current_filter = filter_type;
    
    // Get current WiFi mode and save it
    wifi_mode_t original_mode;
    ESP_ERROR_CHECK(esp_wifi_get_mode(&original_mode));
    
    // Set to APSTA mode to ensure we keep the AP running while scanning
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    
    // Set sniffer filter based on packet type
    wifi_promiscuous_filter_t filter = {0};
    switch (filter_type) {
        case 1: // Management frames
            filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
            break;
        case 2: // Data frames
            filter.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA;
            break;
        case 3: // Control frames
            filter.filter_mask = WIFI_PROMIS_FILTER_MASK_CTRL;
            break;
        case 4: // Beacon frames only
            filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
            // We'll filter in the callback for beacon frames only
            break;
        case 5: // Probe frames only
            filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
            // We'll filter in the callback for probe frames only
            break;
        default: // All packets
            filter.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL;
    }
    
    esp_wifi_set_promiscuous_filter(&filter);
    
    // Register packet handler
    esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler);
    
    // Enable promiscuous mode
    esp_wifi_set_promiscuous(true);
    
    // Set the channel or start channel hopping
    if (channel == 0) {
        // Start channel hopping task
        xTaskCreate(channel_hopper_task, "channel_hopper", 2048, NULL, 5, &channel_hopper_task_handle);
    } else {
        // Set specific channel
        esp_err_t err = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to set initial channel %d: %s", channel, esp_err_to_name(err));
            ESP_LOGI(TAG, "Will retry setting channel in background");
            
            // Create a parameter structure to pass the channel
            uint8_t *channel_param = malloc(sizeof(uint8_t));
            if (channel_param) {
                *channel_param = channel;
                
                // Start a task to keep trying to set the channel
                xTaskCreate(single_channel_retry_task, "channel_retry", 2048, channel_param, 5, &channel_hopper_task_handle);
            }
        }
    }
    
    is_sniffer_running = true;
    xSemaphoreGive(sniffer_running_mutex);
    
    ESP_LOGI(TAG, "WiFi sniffer started successfully");
    return true;
}

// Stop WiFi sniffer
bool stop_wifi_sniffer(void) {
    ESP_LOGI(TAG, "Stopping WiFi sniffer");
    
    // Take mutex
    if (sniffer_running_mutex == NULL || xSemaphoreTake(sniffer_running_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGI(TAG, "Failed to take sniffer mutex");
        return false;
    }
    
    // Check if sniffer is running
    if (!is_sniffer_running) {
        ESP_LOGW(TAG, "Sniffer not running");
        xSemaphoreGive(sniffer_running_mutex);
        return false;
    }
    
    // Disable promiscuous mode
    esp_wifi_set_promiscuous(false);
    
    // Stop channel hopping task if running
    if (channel_hopper_task_handle != NULL) {
        vTaskDelete(channel_hopper_task_handle);
        channel_hopper_task_handle = NULL;
    }
    
    is_sniffer_running = false;
    xSemaphoreGive(sniffer_running_mutex);
    
    ESP_LOGI(TAG, "WiFi sniffer stopped successfully");
    return true;
}

// Get captured packets
int get_captured_packets(void **packets, int max_packets) {
    // Take mutex
    if (sniffer_running_mutex == NULL || xSemaphoreTake(sniffer_running_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGI(TAG, "Failed to take sniffer mutex");
        return 0;
    }
    
    // Check if sniffer is running
    if (!is_sniffer_running) {
        xSemaphoreGive(sniffer_running_mutex);
        return 0;
    }
    
    // Get packets from queue (up to max_packets)
    int count = 0;
    packet_info_t *packet;
    
    while (count < max_packets && xQueueReceive(packet_queue, &packet, 0) == pdTRUE) {
        packets[count++] = packet;
    }
    
    xSemaphoreGive(sniffer_running_mutex);
    return count;
}

// Channel hopper task
static void channel_hopper_task(void *pvParameters) {
    int current_idx = 0;
    int failed_attempts = 0;
    
    ESP_LOGI(TAG, "Channel hopper task started");
    
    while (1) {
        // Use each channel in sequence
        uint8_t new_channel = channels[current_idx];
        
        // Try to set the channel and check for errors
        esp_err_t err = esp_wifi_set_channel(new_channel, WIFI_SECOND_CHAN_NONE);
        
        if (err == ESP_OK) {
            // Channel set successfully
            ESP_LOGD(TAG, "Hopped to channel %d", new_channel);
            current_idx = (current_idx + 1) % channels_count;
            failed_attempts = 0;
        } else {
            // Failed to set channel
            failed_attempts++;
            ESP_LOGD(TAG, "Failed to hop to channel %d: %s (attempt %d)", 
                    new_channel, esp_err_to_name(err), failed_attempts);
            
            // If we've failed multiple times, wait longer before trying again
            if (failed_attempts > 5) {
                ESP_LOGW(TAG, "Multiple channel hop failures, waiting longer...");
                vTaskDelay((CHANNEL_HOP_INTERVAL_MS * 5) / portTICK_PERIOD_MS);
                
                // Reset failed attempts counter after waiting
                failed_attempts = 0;
                continue;
            }
        }
        
        // Wait before hopping again
        vTaskDelay(CHANNEL_HOP_INTERVAL_MS / portTICK_PERIOD_MS);
    }
}

// Packet handler
static void wifi_sniffer_packet_handler(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (!buf) return;
    
    // Check if sniffer is running
    if (!is_sniffer_running) return;
    
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    wifi_pkt_rx_ctrl_t *rx_ctrl = &pkt->rx_ctrl;
    
    // If we have a specific filter for beacon or probe, check it here
    if (current_filter == 4 || current_filter == 5) {
        // Get frame control field to determine if it's a beacon or probe
        const uint8_t *frame = pkt->payload;
        uint16_t frame_control = frame[0] | (frame[1] << 8);
        uint8_t type = (frame_control & 0x000C) >> 2;
        uint8_t subtype = (frame_control & 0x00F0) >> 4;
        
        if (current_filter == 4) { // Beacon frames only
            if (!(type == 0 && subtype == 8)) { // Not a beacon
                return;
            }
        } else if (current_filter == 5) { // Probe frames only
            if (!(type == 0 && (subtype == 4 || subtype == 5))) { // Not a probe request/response
                return;
            }
        }
    }
    
    // Allocate memory for packet info
    packet_info_t *packet_info = malloc(sizeof(packet_info_t));
    if (!packet_info) {
        ESP_LOGI(TAG, "Failed to allocate memory for packet info");
        return;
    }
    
    // Copy packet data
    uint16_t payload_len = rx_ctrl->sig_len - 4; // Remove FCS
    if (payload_len > MAX_PACKET_SIZE) {
        payload_len = MAX_PACKET_SIZE;
    }
    
    // Fill packet info
    packet_info->rx_ctrl = *rx_ctrl;
    packet_info->length = payload_len;
    packet_info->rssi = rx_ctrl->rssi;
    packet_info->channel = rx_ctrl->channel;
    memcpy(packet_info->data, pkt->payload, payload_len);
    
    // Add to queue, if queue is full, discard oldest packet
    packet_info_t *old_packet;
    if (xQueueSend(packet_queue, &packet_info, 0) != pdTRUE) {
        if (xQueueReceive(packet_queue, &old_packet, 0) == pdTRUE) {
            if (old_packet) free(old_packet);
            xQueueSend(packet_queue, &packet_info, 0);
        } else {
            // This shouldn't happen, but free the packet if we can't add it
            free(packet_info);
        }
    }
}

// Task to retry setting a single channel
static void single_channel_retry_task(void *pvParameters) {
    uint8_t target_channel = *(uint8_t*)pvParameters;
    int retry_count = 0;
    
    // Free the parameter memory
    free(pvParameters);
    
    ESP_LOGI(TAG, "Channel retry task started for channel %d", target_channel);
    
    while (is_sniffer_running && retry_count < 20) { // Limit retries to avoid infinite loop
        esp_err_t err = esp_wifi_set_channel(target_channel, WIFI_SECOND_CHAN_NONE);
        
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Successfully set channel to %d after %d retries", target_channel, retry_count);
            break;
        }
        
        retry_count++;
        ESP_LOGD(TAG, "Retry %d: Failed to set channel %d: %s", 
                retry_count, target_channel, esp_err_to_name(err));
        
        // Exponential backoff for retries
        int delay_ms = CHANNEL_HOP_INTERVAL_MS * (1 << (retry_count > 5 ? 5 : retry_count));
        vTaskDelay(delay_ms / portTICK_PERIOD_MS);
    }
    
    if (retry_count >= 20) {
        ESP_LOGW(TAG, "Failed to set channel %d after maximum retries", target_channel);
    }
    
    // Delete self
    channel_hopper_task_handle = NULL;
    vTaskDelete(NULL);
} 