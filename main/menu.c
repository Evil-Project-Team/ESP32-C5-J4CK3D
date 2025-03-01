#include "menu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"

static const char* TAG = "menu";

void menu_init(Menu* menu, const char* title, int initial_capacity) {
    menu->items = (MenuItem*)malloc(initial_capacity * sizeof(MenuItem));
    menu->size = 0;
    menu->capacity = initial_capacity;
    menu->current_selection = 0;
    menu->parent = NULL;
    strncpy(menu->title, title, MAX_MENU_NAME_LENGTH - 1);
    menu->title[MAX_MENU_NAME_LENGTH - 1] = '\0';
    
    ESP_LOGI(TAG, "Menu '%s' initialized with capacity %d", title, initial_capacity);
}

void menu_add_item(Menu* menu, const char* name, void (*callback)(void)) {
    if (menu->size >= menu->capacity) {
        // Expand menu capacity if needed
        int new_capacity = menu->capacity * 2;
        MenuItem* new_items = (MenuItem*)realloc(menu->items, new_capacity * sizeof(MenuItem));
        if (new_items == NULL) {
            ESP_LOGE(TAG, "Failed to expand menu capacity");
            return;
        }
        menu->items = new_items;
        menu->capacity = new_capacity;
    }
    
    MenuItem* item = &menu->items[menu->size];
    strncpy(item->name, name, MAX_MENU_NAME_LENGTH - 1);
    item->name[MAX_MENU_NAME_LENGTH - 1] = '\0';
    item->callback.simple_callback = callback;
    item->has_context = false;
    item->submenu = NULL;
    item->submenu_size = 0;
    item->context = NULL;
    
    menu->size++;
    ESP_LOGI(TAG, "Added item '%s' to menu '%s'", name, menu->title);
}

void menu_add_context_item(Menu* menu, const char* name, void (*callback)(void* context), void* context) {
    if (menu->size >= menu->capacity) {
        // Expand menu capacity if needed
        int new_capacity = menu->capacity * 2;
        MenuItem* new_items = (MenuItem*)realloc(menu->items, new_capacity * sizeof(MenuItem));
        if (new_items == NULL) {
            ESP_LOGE(TAG, "Failed to expand menu capacity");
            return;
        }
        menu->items = new_items;
        menu->capacity = new_capacity;
    }
    
    MenuItem* item = &menu->items[menu->size];
    strncpy(item->name, name, MAX_MENU_NAME_LENGTH - 1);
    item->name[MAX_MENU_NAME_LENGTH - 1] = '\0';
    item->callback.context_callback = callback;
    item->has_context = true;
    item->submenu = NULL;
    item->submenu_size = 0;
    item->context = context;
    
    menu->size++;
    ESP_LOGI(TAG, "Added context item '%s' to menu '%s'", name, menu->title);
}

void menu_add_submenu(Menu* menu, const char* name, Menu* submenu) {
    if (menu->size >= menu->capacity) {
        // Expand menu capacity if needed
        int new_capacity = menu->capacity * 2;
        MenuItem* new_items = (MenuItem*)realloc(menu->items, new_capacity * sizeof(MenuItem));
        if (new_items == NULL) {
            ESP_LOGE(TAG, "Failed to expand menu capacity");
            return;
        }
        menu->items = new_items;
        menu->capacity = new_capacity;
    }
    
    MenuItem* item = &menu->items[menu->size];
    strncpy(item->name, name, MAX_MENU_NAME_LENGTH - 1);
    item->name[MAX_MENU_NAME_LENGTH - 1] = '\0';
    item->callback.simple_callback = NULL;
    item->has_context = false;
    item->submenu = submenu;
    item->submenu_size = submenu->size;
    item->context = NULL;
    
    // Link submenu to parent
    submenu->parent = menu;
    
    menu->size++;
    ESP_LOGI(TAG, "Added submenu '%s' to menu '%s'", name, menu->title);
}

void menu_display(Menu* menu) {
    printf("\n=== %s ===\n", menu->title);
    for (int i = 0; i < menu->size; i++) {
        if (i == menu->current_selection) {
            printf("> %d) %s\n", i+1, menu->items[i].name);
        } else {
            printf("  %d) %s\n", i+1, menu->items[i].name);
        }
    }
    printf("====================\n");
    printf("Enter number to select, 'b' to go back, 'm' for main menu\n> ");
}

void menu_next(Menu* menu) {
    if (menu->size > 0) {
        menu->current_selection = (menu->current_selection + 1) % menu->size;
        ESP_LOGI(TAG, "Moved to next item: %s", menu->items[menu->current_selection].name);
    }
}

void menu_previous(Menu* menu) {
    if (menu->size > 0) {
        menu->current_selection = (menu->current_selection + menu->size - 1) % menu->size;
        ESP_LOGI(TAG, "Moved to previous item: %s", menu->items[menu->current_selection].name);
    }
}

void menu_select(Menu* menu) {
    if (menu->size > 0) {
        MenuItem* selected_item = &menu->items[menu->current_selection];
        ESP_LOGI(TAG, "Selected item: %s", selected_item->name);
        
        if (selected_item->submenu != NULL) {
            ESP_LOGI(TAG, "Entering submenu: %s", selected_item->submenu->title);
            // Handled by the caller who will change the current_menu pointer
        } else if (selected_item->has_context) {
            ESP_LOGI(TAG, "Executing context callback for: %s", selected_item->name);
            selected_item->callback.context_callback(selected_item->context);
        } else if (selected_item->callback.simple_callback != NULL) {
            ESP_LOGI(TAG, "Executing callback for: %s", selected_item->name);
            selected_item->callback.simple_callback();
        }
    }
}

void menu_back(Menu* menu) {
    if (menu->parent != NULL) {
        ESP_LOGI(TAG, "Going back from '%s' to '%s'", menu->title, menu->parent->title);
        // Handled by the caller who will change the current_menu pointer
    }
}

void menu_cleanup(Menu* menu) {
    if (menu->items != NULL) {
        free(menu->items);
        menu->items = NULL;
    }
    menu->size = 0;
    menu->capacity = 0;
    ESP_LOGI(TAG, "Cleaned up menu '%s'", menu->title);
}