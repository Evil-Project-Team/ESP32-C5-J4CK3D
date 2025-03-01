#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "sdkconfig.h"

/**
 * @file board_config.h
 * @brief Configuration for ESP32-C5 boards
 * 
 * This file contains hardware-specific settings for different ESP32-C5 boards.
 * Adjust these settings to match your specific board configuration.
 */

/*
 * Basic pin mapping for ESP32-C5
 * 
 * ESP32-C5 pin mapping based on the official ESP32-C5-DevKitC-1 but designed
 * to be compatible with generic ESP32-C5 boards. GPIO pins are mapped
 * according to common ESP32-C5 dev boards, but can be adjusted as needed.
 */

// GPIO pins (use -1 if not available on your board)
#define GPIO_BUTTON                  0   // Boot button (GPIO0 on most ESP32-C5 boards)

// UART pins - Usually these are fixed for ESP32-C5
#define GPIO_UART_TX                 11  // UART TX (GPIO11 on ESP32-C5)
#define GPIO_UART_RX                 12  // UART RX (GPIO12 on ESP32-C5)

// WiFi antenna configuration
// This is the default if no setting is found in NVS
// Set to 1 to use external antenna, 0 for internal antenna
#define USE_EXTERNAL_ANTENNA         0

// Board identification 
// Enable the appropriate board or define your own
#define BOARD_ESP32_C5_DEVKITC       1   // Official ESP32-C5-DevKitC 
#define BOARD_ESP32_C5_GENERIC       0   // Generic ESP32-C5 board

// Advanced settings
// Default channel for WiFi AP mode
#define DEFAULT_WIFI_CHANNEL         1

// Advanced WiFi settings - may need adjustment for different boards
#define WIFI_COUNTRY_POLICY WIFI_COUNTRY_POLICY_AUTO
#define WIFI_COUNTRY_CODE "US"  // Change according to your region

/**
 * Board-specific initializations
 * Enable or customize based on your specific board
 */
#if BOARD_ESP32_C5_DEVKITC
    // Settings specific to the official DevKitC
    #define BOARD_NAME "ESP32-C5-DevKitC"
#elif BOARD_ESP32_C5_GENERIC
    // Generic board settings
    #define BOARD_NAME "ESP32-C5-Generic"
#else
    // Default fallback
    #define BOARD_NAME "ESP32-C5"
#endif

#endif /* BOARD_CONFIG_H */ 