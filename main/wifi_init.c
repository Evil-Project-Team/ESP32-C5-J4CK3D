#include <stdint.h>
#include <stdbool.h>
#include "esp_wifi.h"
#include "esp_log.h"

static const char *TAG = "wifi_init";

// These functions MUST be linked before the WiFi stack initializes
__attribute__((constructor))
static void init_frame_bypass(void) {
    // This function runs before main()
    ESP_LOGI(TAG, "Initializing WiFi frame bypass");
}

// Core frame bypass functions
__attribute__((used))
int ieee80211_raw_frame_sanity_check(void* frame_ctrl, int32_t frame_len, int32_t ampdu_flag) {
    // Always allow frame to pass the sanity check
    return 0;
}

__attribute__((used))
bool ieee80211_is_frame_type_supported(uint8_t frame_type) {
    // Support all frame types for packet sniffing
    return true;
}

__attribute__((used))
int ieee80211_handle_frame_type(void* frame_ctrl, uint32_t* dport) {
    // Allow all frame types
    return 0;
} 