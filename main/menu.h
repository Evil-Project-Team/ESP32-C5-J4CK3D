#ifndef MENU_H
#define MENU_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_wifi_types.h"  // We need the real wifi_auth_mode_t

#define MAX_MENU_NAME_LENGTH 32
#define MAX_NETWORKS 128  // Can be larger now since we're using heap

typedef struct {
    char ssid[33];
    uint8_t bssid[6];
    int8_t rssi;
    uint8_t channel;
    wifi_auth_mode_t auth_mode;
    bool is_hidden;
    bool selected;
} NetworkInfo;

typedef struct MenuItem {
    char name[MAX_MENU_NAME_LENGTH];
    union {
        void (*simple_callback)(void);
        void (*context_callback)(void* context);
    } callback;
    bool has_context;
    struct Menu* submenu;
    int submenu_size;
    void* context;
} MenuItem;

typedef struct Menu {
    MenuItem* items;      // Dynamically allocated array
    int size;            // Current number of items
    int capacity;        // Maximum items that can be stored
    int current_selection;
    struct Menu* parent;
    char title[MAX_MENU_NAME_LENGTH];
} Menu;

// Function declarations
void menu_init(Menu* menu, const char* title, int initial_capacity);
void menu_add_item(Menu* menu, const char* name, void (*callback)(void));
void menu_add_context_item(Menu* menu, const char* name, void (*callback)(void* context), void* context);
void menu_add_submenu(Menu* menu, const char* name, Menu* submenu);
void menu_display(Menu* menu);
void menu_next(Menu* menu);
void menu_previous(Menu* menu);
void menu_select(Menu* menu);
void menu_back(Menu* menu);
void menu_cleanup(Menu* menu);  // New function to free memory
void network_info_display(void* network_info);

#endif // MENU_H 