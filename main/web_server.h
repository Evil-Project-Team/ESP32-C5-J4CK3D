#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// Initialize the web server
void init_web_server(void);

// Start the HTTP server
httpd_handle_t start_webserver(void);

// Stop the HTTP server
void stop_webserver(void);

#endif /* WEB_SERVER_H */ 