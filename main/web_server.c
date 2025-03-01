#include "web_server.h"
#include <stdio.h>
#include <string.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "cJSON.h"
#include "esp_chip_info.h"
#include "esp_system.h"
#include "nvs_flash.h"

// Include our wifi sniffer functions
extern bool start_wifi_sniffer(uint8_t channel, uint8_t filter_type);
extern bool stop_wifi_sniffer(void);
extern int get_captured_packets(void **packets, int max_packets);

// Define struct to avoid including wifi_init.c header
typedef struct {
    uint8_t data[1024];
    uint16_t length;
    int8_t rssi;
    uint8_t channel;
    wifi_pkt_rx_ctrl_t rx_ctrl;
} packet_info_t;

static const char *TAG = "web_server";

// Web server handle
static httpd_handle_t server = NULL;

// Embedded web content
static const char index_html[] = 
"<!DOCTYPE html>\n"
"<html lang=\"en\">\n"
"<head>\n"
"    <meta charset=\"UTF-8\">\n"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"    <title>ESP32-C5 J4CK3D</title>\n"
"    <style>\n"
"        @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&display=swap');\n"
"        * { margin: 0; padding: 0; box-sizing: border-box; }\n"
"        body {\n"
"            font-family: 'Share Tech Mono', monospace;\n"
"            line-height: 1.6;\n"
"            color: #0f0;\n"
"            background-color: #0a0a0a;\n"
"            text-shadow: 0 0 5px rgba(0, 255, 0, 0.5);\n"
"        }\n"
"        .container {\n"
"            display: flex;\n"
"            min-height: 100vh;\n"
"        }\n"
"        .sidebar {\n"
"            width: 250px;\n"
"            background-color: #111;\n"
"            color: #0f0;\n"
"            padding: 20px 0;\n"
"            border-right: 1px solid #0f0;\n"
"            box-shadow: 0 0 10px #0f0;\n"
"        }\n"
"        .sidebar h1 {\n"
"            padding: 0 20px 20px;\n"
"            font-size: 1.5rem;\n"
"            border-bottom: 1px solid #0f0;\n"
"            text-transform: uppercase;\n"
"            letter-spacing: 2px;\n"
"        }\n"
"        .sidebar ul {\n"
"            list-style: none;\n"
"            margin-top: 20px;\n"
"        }\n"
"        .sidebar ul li {\n"
"            padding: 10px 20px;\n"
"            cursor: pointer;\n"
"            transition: all 0.3s;\n"
"            position: relative;\n"
"        }\n"
"        .sidebar ul li:before {\n"
"            content: '> ';\n"
"            opacity: 0;\n"
"            transition: opacity 0.3s;\n"
"        }\n"
"        .sidebar ul li:hover:before,\n"
"        .sidebar ul li.active:before {\n"
"            opacity: 1;\n"
"        }\n"
"        .sidebar ul li:hover,\n"
"        .sidebar ul li.active {\n"
"            background-color: #1a1a1a;\n"
"            transform: translateX(5px);\n"
"        }\n"
"        .content {\n"
"            flex: 1;\n"
"            padding: 20px;\n"
"            background-color: #0a0a0a;\n"
"            border: 1px solid #0f0;\n"
"            margin: 10px;\n"
"        }\n"
"        .page {\n"
"            display: none;\n"
"        }\n"
"        .page.active {\n"
"            display: block;\n"
"            animation: glitch 0.5s linear;\n"
"        }\n"
"        h2 {\n"
"            margin-bottom: 20px;\n"
"            color: #0f0;\n"
"            text-transform: uppercase;\n"
"            letter-spacing: 2px;\n"
"            border-bottom: 1px solid #0f0;\n"
"            padding-bottom: 5px;\n"
"        }\n"
"        table {\n"
"            width: 100%;\n"
"            border-collapse: collapse;\n"
"            margin-bottom: 20px;\n"
"            background-color: rgba(0, 20, 0, 0.3);\n"
"        }\n"
"        table, th, td {\n"
"            border: 1px solid #0f0;\n"
"        }\n"
"        th, td {\n"
"            padding: 12px 15px;\n"
"            text-align: left;\n"
"        }\n"
"        th {\n"
"            background-color: rgba(0, 50, 0, 0.5);\n"
"            text-transform: uppercase;\n"
"        }\n"
"        tr:nth-child(even) {\n"
"            background-color: rgba(0, 30, 0, 0.3);\n"
"        }\n"
"        tr:hover {\n"
"            background-color: rgba(0, 80, 0, 0.3);\n"
"        }\n"
"        .status {\n"
"            margin-bottom: 20px;\n"
"            padding: 15px;\n"
"            border-radius: 0;\n"
"            display: none;\n"
"            border: 1px dashed #0f0;\n"
"            font-family: 'Share Tech Mono', monospace;\n"
"        }\n"
"        .status.success {\n"
"            background-color: rgba(0, 40, 0, 0.3);\n"
"            color: #0f0;\n"
"            display: block;\n"
"        }\n"
"        .status.error {\n"
"            background-color: rgba(40, 0, 0, 0.3);\n"
"            color: #f00;\n"
"            display: block;\n"
"            text-shadow: 0 0 5px rgba(255, 0, 0, 0.5);\n"
"        }\n"
"        .card {\n"
"            border: 1px solid #0f0;\n"
"            border-radius: 0;\n"
"            padding: 20px;\n"
"            margin-bottom: 20px;\n"
"            background-color: rgba(0, 20, 0, 0.3);\n"
"            box-shadow: 0 0 10px rgba(0, 255, 0, 0.2);\n"
"        }\n"
"        .loader {\n"
"            border: 4px solid #111;\n"
"            border-top: 4px solid #0f0;\n"
"            border-radius: 50%;\n"
"            width: 30px;\n"
"            height: 30px;\n"
"            animation: spin 1s linear infinite;\n"
"            margin: 20px auto;\n"
"        }\n"
"        @keyframes spin {\n"
"            0% { transform: rotate(0deg); }\n"
"            100% { transform: rotate(360deg); }\n"
"        }\n"
"        @keyframes glitch {\n"
"            0% { transform: skew(0deg); }\n"
"            20% { transform: skew(3deg); filter: brightness(1.1); }\n"
"            40% { transform: skew(-3deg); filter: brightness(0.9); }\n"
"            60% { transform: skew(2deg); filter: brightness(1.1); }\n"
"            80% { transform: skew(-2deg); filter: brightness(0.9); }\n"
"            100% { transform: skew(0deg); }\n"
"        }\n"
"        .btn {\n"
"            background-color: #111;\n"
"            color: #0f0;\n"
"            border: 1px solid #0f0;\n"
"            padding: 10px 15px;\n"
"            cursor: pointer;\n"
"            font-size: 16px;\n"
"            font-family: 'Share Tech Mono', monospace;\n"
"            text-transform: uppercase;\n"
"            letter-spacing: 2px;\n"
"            transition: all 0.3s;\n"
"            box-shadow: 0 0 5px rgba(0, 255, 0, 0.5);\n"
"        }\n"
"        .btn:hover {\n"
"            background-color: #0f0;\n"
"            color: #000;\n"
"            box-shadow: 0 0 15px rgba(0, 255, 0, 0.8);\n"
"        }\n"
"        .btn:disabled {\n"
"            background-color: #111;\n"
"            color: #333;\n"
"            border-color: #333;\n"
"            box-shadow: none;\n"
"        }\n"
"        @media (max-width: 768px) {\n"
"            .container {\n"
"                flex-direction: column;\n"
"            }\n"
"            .sidebar {\n"
"                width: 100%;\n"
"                padding: 10px 0;\n"
"                border-right: none;\n"
"                border-bottom: 1px solid #0f0;\n"
"            }\n"
"            .sidebar h1 {\n"
"                font-size: 1.2rem;\n"
"                padding: 0 15px 15px;\n"
"            }\n"
"        }\n"
"        /* CRT effect */\n"
"        body::after {\n"
"            content: '';\n"
"            position: fixed;\n"
"            top: 0;\n"
"            left: 0;\n"
"            width: 100vw;\n"
"            height: 100vh;\n"
"            background: repeating-linear-gradient(\n"
"                0deg,\n"
"                rgba(0, 0, 0, 0.15),\n"
"                rgba(0, 0, 0, 0.15) 1px,\n"
"                transparent 1px,\n"
"                transparent 2px\n"
"            );\n"
"            pointer-events: none;\n"
"            z-index: 999;\n"
"        }\n"
"        ::selection {\n"
"            background: #0f0;\n"
"            color: #000;\n"
"        }\n"
"        .form-group {\n"
"            margin-bottom: 15px;\n"
"        }\n"
"        .form-control {\n"
"            background-color: #111;\n"
"            border: 1px solid #0f0;\n"
"            padding: 8px 12px;\n"
"            color: #0f0;\n"
"            font-family: 'Share Tech Mono', monospace;\n"
"            width: 250px;\n"
"        }\n"
"        label {\n"
"            display: block;\n"
"            margin-bottom: 5px;\n"
"        }\n"
"        .terminal-log {\n"
"            background-color: #000;\n"
"            border: 1px solid #0f0;\n"
"            padding: 10px;\n"
"            height: 300px;\n"
"            overflow-y: auto;\n"
"            font-family: 'Share Tech Mono', monospace;\n"
"            font-size: 0.9em;\n"
"            white-space: pre-wrap;\n"
"            margin-top: 10px;\n"
"            scrollbar-width: none; /* Firefox */\n"
"            -ms-overflow-style: none; /* IE and Edge */\n"
"        }\n"
"        .terminal-log::-webkit-scrollbar {\n"
"            display: none; /* Chrome, Safari, Opera */\n"
"        }\n"
"        .packet-entry {\n"
"            margin-bottom: 5px;\n"
"            padding-bottom: 5px;\n"
"            border-bottom: 1px dotted #033;\n"
"        }\n"
"        .packet-time {\n"
"            color: #0aa;\n"
"        }\n"
"        .packet-type {\n"
"            color: #f80;\n"
"        }\n"
"        .packet-info {\n"
"            color: #0f0;\n"
"        }\n"
"        .packet-data {\n"
"            color: #aaa;\n"
"            font-size: 0.8em;\n"
"        }\n"
"        .packet-table {\n"
"            width: 100%;\n"
"            border-collapse: collapse;\n"
"            font-family: 'Share Tech Mono', monospace;\n"
"            font-size: 0.9em;\n"
"        }\n"
"        .packet-table th {\n"
"            background-color: rgba(0, 50, 0, 0.7);\n"
"            padding: 8px 12px;\n"
"            text-align: left;\n"
"            color: #0f0;\n"
"            border: 1px solid #0f0;\n"
"        }\n"
"        .packet-table td {\n"
"            padding: 6px 10px;\n"
"            border: 1px solid rgba(0, 255, 0, 0.3);\n"
"        }\n"
"        .packet-table tbody tr:nth-child(odd) {\n"
"            background-color: rgba(0, 20, 0, 0.3);\n"
"        }\n"
"        .packet-table tbody tr:nth-child(even) {\n"
"            background-color: rgba(0, 30, 0, 0.3);\n"
"        }\n"
"        .packet-table tbody tr:hover {\n"
"            background-color: rgba(0, 80, 0, 0.4);\n"
"        }\n"
"        .packet-table .time-col {\n"
"            color: #0aa;\n"
"        }\n"
"        .packet-table .type-col {\n"
"            color: #f80;\n"
"        }\n"
"        .packet-table .addr-col {\n"
"            color: #0f0;\n"
"        }\n"
"        .packet-table .rssi-col {\n"
"            color: #0f0;\n"
"            text-align: center;\n"
"        }\n"
"        .packet-table .data-col {\n"
"            color: #aaa;\n"
"            font-size: 0.8em;\n"
"            max-width: 250px;\n"
"            overflow: hidden;\n"
"            text-overflow: ellipsis;\n"
"            white-space: nowrap;\n"
"        }\n"
"        .packet-controls {\n"
"            margin: 10px 0;\n"
"        }\n"
"        #packet-container {\n"
"            overflow-x: auto;\n"
"            margin-top: 15px;\n"
"            max-height: 400px;\n"
"            overflow-y: auto;\n"
"            scrollbar-width: none; /* Firefox */\n"
"            -ms-overflow-style: none; /* IE and Edge */\n"
"        }\n"
"        #packet-container::-webkit-scrollbar {\n"
"            display: none; /* Chrome, Safari, Opera */\n"
"        }\n"
"        .glitch {\n"
"            animation: glitch 0.3s linear infinite;\n"
"        }\n"
"        \n"
"        @keyframes glitch {\n"
"            0% { transform: translate(0, 0); }\n"
"            25% { transform: translate(5px, 5px); }\n"
"            50% { transform: translate(-5px, 5px); }\n"
"            75% { transform: translate(5px, -5px); }\n"
"            100% { transform: translate(0, 0); }\n"
"        }\n"
"        \n"
"        /* Settings page styles */\n"
"        .setting-group {\n"
"            margin: 15px 0;\n"
"            display: flex;\n"
"            align-items: center;\n"
"            justify-content: space-between;\n"
"        }\n"
"        \n"
"        .toggle-switch {\n"
"            display: flex;\n"
"            align-items: center;\n"
"        }\n"
"        \n"
"        .toggle-input {\n"
"            display: none;\n"
"        }\n"
"        \n"
"        .toggle-label {\n"
"            position: relative;\n"
"            display: inline-block;\n"
"            width: 50px;\n"
"            height: 26px;\n"
"            background-color: #222;\n"
"            border-radius: 13px;\n"
"            border: 1px solid #666;\n"
"            cursor: pointer;\n"
"            margin-right: 10px;\n"
"        }\n"
"        \n"
"        .toggle-label:after {\n"
"            content: '';\n"
"            position: absolute;\n"
"            width: 22px;\n"
"            height: 22px;\n"
"            border-radius: 50%;\n"
"            background-color: #666;\n"
"            top: 1px;\n"
"            left: 1px;\n"
"            transition: all 0.3s;\n"
"        }\n"
"        \n"
"        .toggle-input:checked + .toggle-label {\n"
"            background-color: #032b11;\n"
"            border-color: #0f0;\n"
"        }\n"
"        \n"
"        .toggle-input:checked + .toggle-label:after {\n"
"            transform: translateX(24px);\n"
"            background-color: #0f0;\n"
"        }\n"
"        \n"
"        .toggle-text {\n"
"            color: #0f0;\n"
"            font-weight: bold;\n"
"            min-width: 65px;\n"
"        }\n"
"        \n"
"        .status-message {\n"
"            margin-top: 10px;\n"
"            padding: 8px;\n"
"            border-radius: 4px;\n"
"            display: none;\n"
"        }\n"
"        \n"
"        .status-message:not(:empty) {\n"
"            display: block;\n"
"        }\n"
"        \n"
"        .status-message.success {\n"
"            background-color: #032b11;\n"
"            color: #0f0;\n"
"            border: 1px solid #0f0;\n"
"        }\n"
"        \n"
"        .status-message.error {\n"
"            background-color: #2b0303;\n"
"            color: #f00;\n"
"            border: 1px solid #f00;\n"
"        }\n"
"    </style>\n"
"</head>\n"
"<body>\n"
"    <div class=\"container\">\n"
"        <div class=\"sidebar\">\n"
"            <h1>ESP32-C5 J4CK3D</h1>\n"
"            <ul id=\"nav\">\n"
"                <li class=\"active\" data-page=\"dashboard\">C0mm4nd D3ck</li>\n"
"                <li data-page=\"wifi-scan\">R4d4r Sc4n</li>\n"
"                <li data-page=\"packet-sniff\">P4ck3t Sn1ff3r</li>\n"
"                <li data-page=\"settings\">Sy5t3m C0nfig</li>\n"
"            </ul>\n"
"        </div>\n"
"        <div class=\"content\">\n"
"            <!-- Dashboard Page -->\n"
"            <div id=\"dashboard\" class=\"page active\">\n"
"                <h2>C0mm4nd D3ck</h2>\n"
"                <div class=\"card\">\n"
"                    <h3>Sy5t3m St4tu5</h3>\n"
"                    <div id=\"system-info\">\n"
"                        <div class=\"loader\"></div>\n"
"                    </div>\n"
"                </div>\n"
"                <div class=\"card\">\n"
"                    <h3>CR3D1TS</h3>\n"
"                    <p>Cr34t3d by <span style=\"color: #f00; text-shadow: 0 0 5px rgba(255, 0, 0, 0.7);\">3V1L PR0J3CT T34M</span></p>\n"
"                </div>\n"
"            </div>\n"
"            \n"
"            <!-- WiFi Scanner Page -->\n"
"            <div id=\"wifi-scan\" class=\"page\">\n"
"                <h2>R4d4r Sc4n</h2>\n"
"                <button id=\"scan-btn\" class=\"btn\">Sc4n F0r N3tw0rk5</button>\n"
"                <div id=\"scan-status\" class=\"status\"></div>\n"
"                <div id=\"scan-results\">\n"
"                    <div class=\"loader\" style=\"display: none;\"></div>\n"
"                    <table id=\"networks-table\" style=\"display: none;\">\n"
"                        <thead>\n"
"                            <tr>\n"
"                                <th>SSID</th>\n"
"                                <th>BSSID</th>\n"
"                                <th>B4nd</th>\n"
"                                <th>Ch4nn3l</th>\n"
"                                <th>RSSI</th>\n"
"                                <th>PHY M0d3</th>\n"
"                                <th>S3cur1ty</th>\n"
"                            </tr>\n"
"                        </thead>\n"
"                        <tbody></tbody>\n"
"                    </table>\n"
"                </div>\n"
"            </div>\n"
"            \n"
"            <!-- Packet Sniffer Page -->\n"
"            <div id=\"packet-sniff\" class=\"page\">\n"
"                <h2>P4ck3t Sn1ff3r</h2>\n"
"                <div class=\"card\">\n"
"                    <h3>C0nfigur4t10n</h3>\n"
"                    <div class=\"form-group\">\n"
"                        <label for=\"sniff-channel\">Ch4nn3l:</label>\n"
"                        <select id=\"sniff-channel\" class=\"form-control\">\n"
"                            <option value=\"0\">4ll Ch4nn3ls (H0pp1ng)</option>\n"
"                            <option value=\"1\">1</option>\n"
"                            <option value=\"2\">2</option>\n"
"                            <option value=\"3\">3</option>\n"
"                            <option value=\"4\">4</option>\n"
"                            <option value=\"5\">5</option>\n"
"                            <option value=\"6\">6</option>\n"
"                            <option value=\"7\">7</option>\n"
"                            <option value=\"8\">8</option>\n"
"                            <option value=\"9\">9</option>\n"
"                            <option value=\"10\">10</option>\n"
"                            <option value=\"11\">11</option>\n"
"                            <option value=\"12\">12</option>\n"
"                            <option value=\"13\">13</option>\n"
"                        </select>\n"
"                    </div>\n"
"                    <div class=\"form-group\">\n"
"                        <label for=\"packet-filter\">F1lt3r Typ3:</label>\n"
"                        <select id=\"packet-filter\" class=\"form-control\">\n"
"                            <option value=\"all\">4ll P4ck3ts</option>\n"
"                            <option value=\"management\">M4n4g3m3nt Fr4m3s</option>\n"
"                            <option value=\"data\">D4t4 Fr4m3s</option>\n"
"                            <option value=\"control\">C0ntr0l Fr4m3s</option>\n"
"                            <option value=\"beacon\">B34c0n Fr4m3s</option>\n"
"                            <option value=\"probe\">Pr0b3 R3qu3sts/R3sp0ns3s</option>\n"
"                        </select>\n"
"                    </div>\n"
"                    <div class=\"form-group\">\n"
"                        <button id=\"start-sniff\" class=\"btn\">ST4RT SN1FF1NG</button>\n"
"                        <button id=\"stop-sniff\" class=\"btn\" disabled>ST0P SN1FF1NG</button>\n"
"                        <button id=\"clear-packets\" class=\"btn\">CL34R L0G</button>\n"
"                    </div>\n"
"                </div>\n"
"                <div id=\"sniff-status\" class=\"status\">R34DY T0 SN1FF</div>\n"
"                <div class=\"card\">\n"
"                    <h3>C4ptur3d P4ck3ts</h3>\n"
"                    <div id=\"packet-stats\">\n"
"                        <span id=\"packet-count\">0</span> p4ck3ts c4ptur3d\n"
"                        <button id=\"clear-packets-top\" class=\"btn\" style=\"margin-left: 15px;\">CL34R L0G</button>\n"
"                    </div>\n"
"                    <div id=\"packet-container\" style=\"overflow-x: auto; margin-top: 15px; max-height: 400px; overflow-y: auto; scrollbar-width: none; -ms-overflow-style: none;\">\n"
"                        <table id=\"packet-table\" class=\"packet-table\">\n"
"                            <thead>\n"
"                                <tr>\n"
"                                    <th>#</th>\n"
"                                    <th>T1m3</th>\n"
"                                    <th>Typ3</th>\n"
"                                    <th>S0urc3</th>\n"
"                                    <th>D3st</th>\n"
"                                    <th>Ch4nn3l</th>\n"
"                                    <th>RSSI</th>\n"
"                                    <th>D4t4</th>\n"
"                                </tr>\n"
"                            </thead>\n"
"                            <tbody id=\"packet-log\">\n"
"                                <!-- Packet data will be inserted here -->\n"
"                            </tbody>\n"
"                        </table>\n"
"                    </div>\n"
"                </div>\n"
"            </div>\n"
"            \n"
"            <!-- Settings Page -->\n"
"            <div id=\"settings\" class=\"page\">\n"
"                <h2>Sy5t3m C0nfig</h2>\n"
"                <div class=\"card\">\n"
"                    <h3>D3vic3 C0nfigur4ti0n</h3>\n"
"                    <p>C0nfigur3 d3vic3 s3tting5:</p>\n"
"                    <div class=\"setting-group\">\n"
"                        <label for=\"antenna-switch\">Antenna Selection:</label>\n"
"                        <div class=\"toggle-switch\">\n"
"                            <input type=\"checkbox\" id=\"antenna-switch\" class=\"toggle-input\">\n"
"                            <label for=\"antenna-switch\" class=\"toggle-label\"></label>\n"
"                            <span class=\"toggle-text\">Internal</span>\n"
"                        </div>\n"
"                    </div>\n"
"                    <button id=\"save-settings\" class=\"btn\">Save Settings</button>\n"
"                    <div id=\"settings-status\" class=\"status-message\"></div>\n"
"                </div>\n"
"            </div>\n"
"        </div>\n"
"    </div>\n"
"    \n"
"    <script>\n"
"        // Add some hacker style console messages\n"
"        console.log('%c [SYSTEM] ESP32-C5 J4CK3D CONSOLE INITIALIZED', 'color: #0f0; font-weight: bold; background: #000');\n"
"        console.log('%c [WARNING] UNAUTHORIZED ACCESS WILL BE TRACKED AND REPORTED', 'color: #f00; font-weight: bold; background: #000');\n"
"        \n"
"        // Navigation\n"
"        document.querySelectorAll('#nav li').forEach(item => {\n"
"            item.addEventListener('click', () => {\n"
"                // Add glitchy console output\n"
"                console.log('%c [NAVIGATION] Accessing ' + item.textContent, 'color: #0f0; background: #000');\n"
"                \n"
"                // Remove active class from all items\n"
"                document.querySelectorAll('#nav li').forEach(i => i.classList.remove('active'));\n"
"                // Add active class to clicked item\n"
"                item.classList.add('active');\n"
"                \n"
"                // Hide all pages\n"
"                document.querySelectorAll('.page').forEach(page => page.classList.remove('active'));\n"
"                // Show clicked page\n"
"                document.getElementById(item.getAttribute('data-page')).classList.add('active');\n"
"            });\n"
"        });\n"
"        \n"
"        // Load system info\n"
"        function loadSystemInfo() {\n"
"            console.log('%c [SYSTEM] Retrieving system information...', 'color: #0f0; background: #000');\n"
"            fetch('/api/system-info')\n"
"                .then(response => response.json())\n"
"                .then(data => {\n"
"                    console.log('%c [SYSTEM] Information retrieved successfully', 'color: #0f0; background: #000');\n"
"                    const sysInfoElement = document.getElementById('system-info');\n"
"                    sysInfoElement.innerHTML = `\n"
"                        <p><strong>IDF V3r5i0n:</strong> ${data.idf_version}</p>\n"
"                        <p><strong>Fr33 H34p:</strong> ${formatBytes(data.free_heap)}</p>\n"
"                        <p><strong>M4C 4ddr355:</strong> ${data.mac_address}</p>\n"
"                        <p><strong>Chip M0d3l:</strong> ${data.chip_model}</p>\n"
"                        <p><strong>C0r35:</strong> ${data.chip_cores}</p>\n"
"                        <p><strong>F34tur35:</strong> ${data.features}</p>\n"
"                    `;\n"
"                })\n"
"                .catch(error => {\n"
"                    console.error('%c [ERROR] Failed to retrieve system information', 'color: #f00; background: #000');\n"
"                    document.getElementById('system-info').innerHTML = `<p class=\"error\">3rr0r l04ding 5y5t3m inf0: ${error.message}</p>`;\n"
"                });\n"
"        }\n"
"        \n"
"        // Format bytes\n"
"        function formatBytes(bytes) {\n"
"            if (bytes < 1024) return bytes + ' byt35';\n"
"            else if (bytes < 1048576) return (bytes / 1024).toFixed(2) + ' KB';\n"
"            else return (bytes / 1048576).toFixed(2) + ' MB';\n"
"        }\n"
"        \n"
"        // WiFi Scan\n"
"        document.getElementById('scan-btn').addEventListener('click', function() {\n"
"            console.log('%c [SCAN] Initiating network scan...', 'color: #0f0; background: #000');\n"
"            const statusElement = document.getElementById('scan-status');\n"
"            const loaderElement = document.querySelector('#scan-results .loader');\n"
"            const tableElement = document.getElementById('networks-table');\n"
"            const buttonElement = this;\n"
"            \n"
"            // Disable button during scan\n"
"            buttonElement.disabled = true;\n"
"            buttonElement.textContent = 'SC4NNING...';\n"
"            \n"
"            statusElement.textContent = 'SC4NNING F0R N3TW0RK5...';\n"
"            statusElement.className = 'status success';\n"
"            loaderElement.style.display = 'block';\n"
"            tableElement.style.display = 'none';\n"
"            \n"
"            // Set a timeout for the scan request\n"
"            const controller = new AbortController();\n"
"            const timeoutId = setTimeout(() => controller.abort(), 30000); // 30 seconds timeout\n"
"            \n"
"            fetch('/api/scan', {\n"
"                signal: controller.signal\n"
"            })\n"
"                .then(response => {\n"
"                    clearTimeout(timeoutId);\n"
"                    if (!response.ok) {\n"
"                        throw new Error(`HTTP error: ${response.status}`);\n"
"                    }\n"
"                    return response.json();\n"
"                })\n"
"                .then(data => {\n"
"                    loaderElement.style.display = 'none';\n"
"                    console.log('%c [SCAN] Scan completed', 'color: #0f0; background: #000');\n"
"                    \n"
"                    if (data.status === 'success') {\n"
"                        const tbody = tableElement.querySelector('tbody');\n"
"                        tbody.innerHTML = '';\n"
"                        \n"
"                        if (!data.networks || data.networks.length === 0) {\n"
"                            statusElement.textContent = 'N0 N3TW0RK5 F0UND!';\n"
"                            console.log('%c [SCAN] No networks found', 'color: #f00; background: #000');\n"
"                        } else {\n"
"                            console.log(`%c [SCAN] Found ${data.networks.length} networks`, 'color: #0f0; background: #000');\n"
"                            data.networks.forEach(network => {\n"
"                                const row = document.createElement('tr');\n"
"                                row.innerHTML = `\n"
"                                    <td>${network.ssid || '<HIDD3N 55ID>'}</td>\n"
"                                    <td>${network.bssid}</td>\n"
"                                    <td>${network.band}</td>\n"
"                                    <td>${network.channel}${network.second_channel ? '+' + network.second_channel : ''}</td>\n"
"                                    <td>${network.rssi} dBm</td>\n"
"                                    <td>${network.phy_mode}</td>\n"
"                                    <td>${network.security}</td>\n"
"                                `;\n"
"                                tbody.appendChild(row);\n"
"                            });\n"
"                            \n"
"                            statusElement.textContent = `F0UND ${data.networks.length} N3TW0RK5`;\n"
"                            tableElement.style.display = 'table';\n"
"                        }\n"
"                    } else {\n"
"                        console.error('%c [ERROR] Scan failed: ' + (data.message || 'unknown'), 'color: #f00; background: #000');\n"
"                        statusElement.textContent = '5C4N F41L3D: ' + (data.message || 'UNKN0WN 3RR0R');\n"
"                        statusElement.className = 'status error';\n"
"                    }\n"
"                    \n"
"                    // Re-enable button\n"
"                    buttonElement.disabled = false;\n"
"                    buttonElement.textContent = 'SC4N F0R N3TW0RK5';\n"
"                })\n"
"                .catch(error => {\n"
"                    clearTimeout(timeoutId);\n"
"                    loaderElement.style.display = 'none';\n"
"                    console.error('%c [ERROR] ' + error.message, 'color: #f00; background: #000');\n"
"                    \n"
"                    if (error.name === 'AbortError') {\n"
"                        statusElement.textContent = '5C4N T1M3D 0UT. TH3 5C4N 0P3R4T10N M4Y 5T1LL B3 RUNN1NG 0N TH3 D3V1C3.';\n"
"                    } else {\n"
"                        statusElement.textContent = '3RR0R: ' + error.message;\n"
"                    }\n"
"                    statusElement.className = 'status error';\n"
"                    \n"
"                    // Re-enable button\n"
"                    buttonElement.disabled = false;\n"
"                    buttonElement.textContent = 'SC4N F0R N3TW0RK5';\n"
"                });\n"
"        });\n"
"        \n"
"        // Packet Sniffing\n"
"        let packetCount = 0;\n"
"        let sniffing = false;\n"
"        let packetLogElement = document.getElementById('packet-log');\n"
"        let packetCountElement = document.getElementById('packet-count');\n"
"        let sniffIntervalId = null;\n"
"        \n"
"        // Translate filter type string to numeric value\n"
"        function get_filter_type(filter_str) {\n"
"            if (filter_str === 'management') return 1;\n"
"            if (filter_str === 'data') return 2;\n"
"            if (filter_str === 'control') return 3;\n"
"            if (filter_str === 'beacon') return 4;\n"
"            if (filter_str === 'probe') return 5;\n"
"            return 0; // default: all packets\n"
"        }\n"
"        \n"
"        // Start sniffing button\n"
"        document.getElementById('start-sniff').addEventListener('click', function() {\n"
"            if (sniffing) return;\n"
"            \n"
"            const channel = document.getElementById('sniff-channel').value;\n"
"            const filter = document.getElementById('packet-filter').value;\n"
"            const statusElement = document.getElementById('sniff-status');\n"
"            \n"
"            console.log(`%c [SNIFF] Starting packet capture on channel ${channel} with filter ${filter}`, 'color: #0f0; background: #000');\n"
"            \n"
"            statusElement.textContent = `ST4RT1NG P4CK3T C4PTUR3 0N CH4NN3L ${channel === '0' ? 'ALL' : channel}...`;\n"
"            statusElement.className = 'status success';\n"
"            \n"
"            // Disable start button, enable stop button\n"
"            this.disabled = true;\n"
"            document.getElementById('stop-sniff').disabled = false;\n"
"            \n"
"            // Start sniffing API call\n"
"            fetch(`/api/sniff/start?channel=${channel}&filter=${filter}`)\n"
"                .then(response => response.json())\n"
"                .then(data => {\n"
"                    if (data.status === 'success') {\n"
"                        sniffing = true;\n"
"                        statusElement.textContent = `P4CK3T C4PTUR3 RUNN1NG 0N CH4NN3L ${channel === '0' ? 'ALL' : channel}`;\n"
"                        console.log('%c [SNIFF] Packet capture started', 'color: #0f0; background: #000');\n"
"                        \n"
"                        // Set up polling for new packets\n"
"                        sniffIntervalId = setInterval(fetchNewPackets, 1000);\n"
"                    } else {\n"
"                        statusElement.textContent = `F41L3D T0 ST4RT C4PTUR3: ${data.message}`;\n"
"                        statusElement.className = 'status error';\n"
"                        this.disabled = false;\n"
"                        document.getElementById('stop-sniff').disabled = true;\n"
"                        console.error('%c [ERROR] Failed to start packet capture', 'color: #f00; background: #000');\n"
"                    }\n"
"                })\n"
"                .catch(error => {\n"
"                    statusElement.textContent = `3RR0R: ${error.message}`;\n"
"                    statusElement.className = 'status error';\n"
"                    this.disabled = false;\n"
"                    document.getElementById('stop-sniff').disabled = true;\n"
"                    console.error('%c [ERROR] ' + error.message, 'color: #f00; background: #000');\n"
"                });\n"
"        });\n"
"        \n"
"        // Stop sniffing button\n"
"        document.getElementById('stop-sniff').addEventListener('click', function() {\n"
"            if (!sniffing) return;\n"
"            \n"
"            const statusElement = document.getElementById('sniff-status');\n"
"            statusElement.textContent = 'ST0PP1NG C4PTUR3...';\n"
"            \n"
"            // Clear the polling interval\n"
"            if (sniffIntervalId) {\n"
"                clearInterval(sniffIntervalId);\n"
"                sniffIntervalId = null;\n"
"            }\n"
"            \n"
"            // Stop sniffing API call\n"
"            fetch('/api/sniff/stop')\n"
"                .then(response => response.json())\n"
"                .then(data => {\n"
"                    sniffing = false;\n"
"                    statusElement.textContent = 'C4PTUR3 ST0PP3D';\n"
"                    this.disabled = true;\n"
"                    document.getElementById('start-sniff').disabled = false;\n"
"                    console.log('%c [SNIFF] Packet capture stopped', 'color: #0f0; background: #000');\n"
"                })\n"
"                .catch(error => {\n"
"                    sniffing = false;\n"
"                    statusElement.textContent = `3RR0R: ${error.message}`;\n"
"                    statusElement.className = 'status error';\n"
"                    this.disabled = true;\n"
"                    document.getElementById('start-sniff').disabled = false;\n"
"                    console.error('%c [ERROR] ' + error.message, 'color: #f00; background: #000');\n"
"                });\n"
"        });\n"
"        \n"
"        // Clear packet log (support both buttons with same functionality)\n"
"        document.getElementById('clear-packets').addEventListener('click', clearPacketTable);\n"
"        document.getElementById('clear-packets-top').addEventListener('click', clearPacketTable);\n"
"        \n"
"        // Function to clear the packet table\n"
"        function clearPacketTable() {\n"
"            // Keep the header row but clear all packet rows\n"
"            const table = document.getElementById('packet-table');\n"
"            while (table.rows.length > 1) { // Keep header row\n"
"                table.deleteRow(1);\n"
"            }\n"
"            packetCount = 0;\n"
"            packetCountElement.textContent = '0';\n"
"            console.log('%c [SNIFF] Packet log cleared', 'color: #0f0; background: #000');\n"
"        }\n"
"        \n"
"        // Function to fetch new packets\n"
"        function fetchNewPackets() {\n"
"            if (!sniffing) return;\n"
"            \n"
"            fetch('/api/sniff/packets')\n"
"                .then(response => response.json())\n"
"                .then(data => {\n"
"                    if (data.packets && data.packets.length > 0) {\n"
"                        // Add new packets to the log\n"
"                        data.packets.forEach(packet => {\n"
"                            addPacketToLog(packet);\n"
"                        });\n"
"                    }\n"
"                })\n"
"                .catch(error => {\n"
"                    console.error('%c [ERROR] Failed to fetch packets: ' + error.message, 'color: #f00; background: #000');\n"
"                });\n"
"        }\n"
"        \n"
"        // Function to add a packet to the log\n"
"        function addPacketToLog(packet) {\n"
"            packetCount++;\n"
"            packetCountElement.textContent = packetCount;\n"
"            \n"
"            const timestamp = new Date().toISOString().substring(11, 23);\n"
"            const packetType = packet.type || 'UNKNOWN';\n"
"            const sourceAddr = packet.src || 'FF:FF:FF:FF:FF:FF';\n"
"            const destAddr = packet.dst || 'FF:FF:FF:FF:FF:FF';\n"
"            const rssi = packet.rssi || '-';\n"
"            const channel = packet.channel || '?';\n"
"            const packetData = packet.data || '';\n"
"            \n"
"            // Create a new row\n"
"            const newRow = document.createElement('tr');\n"
"            \n"
"            // Add cells with data\n"
"            newRow.innerHTML = `\n"
"                <td>${packetCount}</td>\n"
"                <td class=\"time-col\">${timestamp}</td>\n"
"                <td class=\"type-col\">${packetType}</td>\n"
"                <td class=\"addr-col\">${sourceAddr}</td>\n"
"                <td class=\"addr-col\">${destAddr}</td>\n"
"                <td class=\"rssi-col\">${channel}</td>\n"
"                <td class=\"rssi-col\">${rssi} dBm</td>\n"
"                <td class=\"data-col\" title=\"${packetData}\">${packetData}</td>\n"
"            `;\n"
"            \n"
"            // Add to table\n"
"            packetLogElement.appendChild(newRow);\n"
"            \n"
"            // Keep table to a reasonable size (latest 100 packets)\n"
"            if (packetLogElement.children.length > 100) {\n"
"                packetLogElement.removeChild(packetLogElement.firstChild);\n"
"            }\n"
"            \n"
"            // Scroll to bottom to see new packets\n"
"            const packetContainer = document.getElementById('packet-container');\n"
"            packetContainer.scrollTop = packetContainer.scrollHeight;\n"
"        }\n"
"        \n"
"        // Add random glitch effect to elements periodically\n"
"        setInterval(() => {\n"
"            const elements = document.querySelectorAll('h1, h2, h3');\n"
"            const randomElement = elements[Math.floor(Math.random() * elements.length)];\n"
"            randomElement.style.animation = 'glitch 0.3s linear';\n"
"            setTimeout(() => {\n"
"                randomElement.style.animation = '';\n"
"            }, 300);\n"
"        }, 5000);\n"
"        \n"
"        // Initialize\n"
"        \n"
"        // Handle settings page functionality\n"
"        function initSettingsPage() {\n"
"            const antennaSwitch = document.getElementById('antenna-switch');\n"
"            const toggleText = antennaSwitch.nextElementSibling.nextElementSibling;\n"
"            const saveButton = document.getElementById('save-settings');\n"
"            const statusMessage = document.getElementById('settings-status');\n"
"            \n"
"            // Load current antenna settings\n"
"            fetch('/api/antenna')\n"
"                .then(response => response.json())\n"
"                .then(data => {\n"
"                    if (data.status === 'ok') {\n"
"                        antennaSwitch.checked = data.external_antenna;\n"
"                        toggleText.textContent = data.external_antenna ? 'External' : 'Internal';\n"
"                    }\n"
"                })\n"
"                .catch(error => {\n"
"                    console.error('Error loading antenna settings:', error);\n"
"                    statusMessage.textContent = 'Failed to load settings';\n"
"                    statusMessage.className = 'status-message error';\n"
"                });\n"
"            \n"
"            // Update toggle text when switch is changed\n"
"            antennaSwitch.addEventListener('change', () => {\n"
"                toggleText.textContent = antennaSwitch.checked ? 'External' : 'Internal';\n"
"            });\n"
"            \n"
"            // Save settings\n"
"            saveButton.addEventListener('click', () => {\n"
"                statusMessage.textContent = 'Saving...';\n"
"                statusMessage.className = 'status-message';\n"
"                \n"
"                fetch('/api/antenna', {\n"
"                    method: 'POST',\n"
"                    headers: {\n"
"                        'Content-Type': 'application/json',\n"
"                    },\n"
"                    body: JSON.stringify({\n"
"                        external_antenna: antennaSwitch.checked\n"
"                    }),\n"
"                })\n"
"                .then(response => response.json())\n"
"                .then(data => {\n"
"                    if (data.status === 'ok') {\n"
"                        statusMessage.textContent = data.message || 'Settings saved successfully!';\n"
"                        statusMessage.className = 'status-message success';\n"
"                    } else {\n"
"                        statusMessage.textContent = 'Error: ' + (data.message || 'Unknown error');\n"
"                        statusMessage.className = 'status-message error';\n"
"                    }\n"
"                })\n"
"                .catch(error => {\n"
"                    console.error('Error saving settings:', error);\n"
"                    statusMessage.textContent = 'Failed to save settings';\n"
"                    statusMessage.className = 'status-message error';\n"
"                });\n"
"            });\n"
"        }\n"
"        \n"
"        document.addEventListener('DOMContentLoaded', function() {\n"
"            loadSystemInfo();\n"
"            initSettingsPage();\n"
"        });\n"
"    </script>\n"
"</body>\n"
"</html>";

// Function to get content type based on file extension
static __attribute__((unused)) const char* get_content_type(const char *filepath) {
    const char *ext = strrchr(filepath, '.');
    if (ext) {
        ext++; // Move past the dot
        if (strcasecmp(ext, "html") == 0) return "text/html";
        if (strcasecmp(ext, "css") == 0) return "text/css";
        if (strcasecmp(ext, "js") == 0) return "application/javascript";
        if (strcasecmp(ext, "json") == 0) return "application/json";
        if (strcasecmp(ext, "png") == 0) return "image/png";
        if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0) return "image/jpeg";
        if (strcasecmp(ext, "ico") == 0) return "image/x-icon";
    }
    return "text/plain";
}

// Handler for serving static files
static esp_err_t http_serve_file(httpd_req_t *req) {
    // Basic URI sanitization
    const char *req_uri = req->uri;
    
    // Root path or /index.html serves the embedded index.html
    if (strcmp(req_uri, "/") == 0 || strcmp(req_uri, "/index.html") == 0) {
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, index_html, strlen(index_html));
        return ESP_OK;
    }
    
    // For other files, we don't have them embedded, so serve a 404
    // The JavaScript will handle API calls directly
    httpd_resp_send_404(req);
    return ESP_FAIL;
}

// API handler for scanning WiFi networks
static esp_err_t api_scan_networks_handler(httpd_req_t *req) {
    // Set response type to JSON
    httpd_resp_set_type(req, "application/json");
    
    ESP_LOGI(TAG, "Starting WiFi scan...");
    
    // Get current WiFi mode and save it
    wifi_mode_t original_mode;
    esp_err_t mode_err = esp_wifi_get_mode(&original_mode);
    if (mode_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get WiFi mode: %s", esp_err_to_name(mode_err));
        httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"Failed to get WiFi mode\"}");
        return ESP_OK;
    }
    
    // Set to APSTA mode to ensure we keep the AP running while scanning
    esp_err_t set_mode_err = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (set_mode_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode to APSTA: %s", esp_err_to_name(set_mode_err));
        httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"Failed to set WiFi mode\"}");
        return ESP_OK;
    }
    
    // Configure scan with shorter scan times to prevent timeouts
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 50,
        .scan_time.active.max = 100,
        .scan_time.passive = 100
    };
    
    // Start scan (blocking but faster)
    ESP_LOGI(TAG, "Executing WiFi scan with scan_config...");
    esp_err_t scan_status = esp_wifi_scan_start(&scan_config, true);
    if (scan_status != ESP_OK) {
        ESP_LOGE(TAG, "WiFi scan failed to start: %s", esp_err_to_name(scan_status));
        
        esp_err_t restore_err = esp_wifi_set_mode(original_mode);
        if (restore_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to restore original WiFi mode: %s", esp_err_to_name(restore_err));
        }
        
        // Send error response with specific error message
        char error_msg[100];
        snprintf(error_msg, sizeof(error_msg), "{\"status\":\"error\",\"message\":\"Scan failed: %s\"}", esp_err_to_name(scan_status));
        httpd_resp_sendstr(req, error_msg);
        return ESP_OK;
    }
    
    // Get results
    uint16_t ap_count = 0;
    esp_err_t count_err = esp_wifi_scan_get_ap_num(&ap_count);
    if (count_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get AP count: %s", esp_err_to_name(count_err));
        
        esp_err_t restore_err = esp_wifi_set_mode(original_mode);
        if (restore_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to restore original WiFi mode: %s", esp_err_to_name(restore_err));
        }
        
        // Send error response with specific error message
        char error_msg[100];
        snprintf(error_msg, sizeof(error_msg), "{\"status\":\"error\",\"message\":\"Failed to get AP count: %s\"}", esp_err_to_name(count_err));
        httpd_resp_sendstr(req, error_msg);
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Scan completed, found %d networks", ap_count);
    
    if (ap_count == 0) {
        // No networks found
        esp_err_t restore_err = esp_wifi_set_mode(original_mode);
        if (restore_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to restore original WiFi mode: %s", esp_err_to_name(restore_err));
        }
        httpd_resp_sendstr(req, "{\"status\":\"success\",\"networks\":[]}");
        return ESP_OK;
    }
    
    // Allocate memory for scan results
    wifi_ap_record_t *ap_records = malloc(sizeof(wifi_ap_record_t) * ap_count);
    if (ap_records == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for scan results");
        
        esp_err_t restore_err = esp_wifi_set_mode(original_mode);
        if (restore_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to restore original WiFi mode: %s", esp_err_to_name(restore_err));
        }
        
        httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"Out of memory\"}");
        return ESP_OK;
    }
    
    // Get AP records
    esp_err_t get_ap_err = esp_wifi_scan_get_ap_records(&ap_count, ap_records);
    if (get_ap_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get AP records: %s", esp_err_to_name(get_ap_err));
        
        // Clean up and restore
        free(ap_records);
        esp_err_t restore_err = esp_wifi_set_mode(original_mode);
        if (restore_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to restore original WiFi mode: %s", esp_err_to_name(restore_err));
        }
        
        // Send error response with specific error message
        char error_msg[100];
        snprintf(error_msg, sizeof(error_msg), "{\"status\":\"error\",\"message\":\"Failed to get AP records: %s\"}", esp_err_to_name(get_ap_err));
        httpd_resp_sendstr(req, error_msg);
        return ESP_OK;
    }
    
    // Create JSON response
    cJSON *root = cJSON_CreateObject();
    cJSON *networks = cJSON_AddArrayToObject(root, "networks");
    
    if (!root || !networks) {
        ESP_LOGE(TAG, "Failed to create JSON objects");
        free(ap_records);
        esp_err_t restore_err = esp_wifi_set_mode(original_mode);
        if (restore_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to restore original WiFi mode: %s", esp_err_to_name(restore_err));
        }
        httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"JSON creation failed\"}");
        return ESP_OK;
    }
    
    cJSON_AddStringToObject(root, "status", "success");
    
    for (int i = 0; i < ap_count; i++) {
        cJSON *network = cJSON_CreateObject();
        if (!network) {
            ESP_LOGE(TAG, "Failed to create network JSON object");
            continue;
        }
        
        // Add network details to JSON
        cJSON_AddStringToObject(network, "ssid", (char*)ap_records[i].ssid);
        
        char bssid_str[18];
        snprintf(bssid_str, sizeof(bssid_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                 ap_records[i].bssid[0], ap_records[i].bssid[1], ap_records[i].bssid[2],
                 ap_records[i].bssid[3], ap_records[i].bssid[4], ap_records[i].bssid[5]);
        cJSON_AddStringToObject(network, "bssid", bssid_str);
        
        cJSON_AddNumberToObject(network, "rssi", ap_records[i].rssi);
        cJSON_AddNumberToObject(network, "channel", ap_records[i].primary);
        
        // Determine WiFi band (2.4GHz or 5GHz) based on channel
        const char* band = (ap_records[i].primary > 14) ? "5 GHz" : "2.4 GHz";
        cJSON_AddStringToObject(network, "band", band);
        
        // Add second channel if using 40MHz bandwidth
        if (ap_records[i].second) {
            cJSON_AddNumberToObject(network, "second_channel", ap_records[i].second);
        }
        
        // Add PHY mode info
        const char* phy_mode;
        switch (ap_records[i].phy_11n) {
            case true:
                phy_mode = "802.11n";
                break;
            default:
                if (band[0] == '5') {
                    phy_mode = "802.11a";
                } else {
                    phy_mode = "802.11b/g";
                }
        }
        cJSON_AddStringToObject(network, "phy_mode", phy_mode);
        
        // Convert auth mode to string
        const char* auth_mode_str;
        switch (ap_records[i].authmode) {
            case WIFI_AUTH_OPEN:
                auth_mode_str = "Open";
                break;
            case WIFI_AUTH_WEP:
                auth_mode_str = "WEP";
                break;
            case WIFI_AUTH_WPA_PSK:
                auth_mode_str = "WPA PSK";
                break;
            case WIFI_AUTH_WPA2_PSK:
                auth_mode_str = "WPA2 PSK";
                break;
            case WIFI_AUTH_WPA_WPA2_PSK:
                auth_mode_str = "WPA/WPA2 PSK";
                break;
            case WIFI_AUTH_WPA3_PSK:
                auth_mode_str = "WPA3 PSK";
                break;
            case WIFI_AUTH_WPA2_WPA3_PSK:
                auth_mode_str = "WPA2/WPA3 PSK";
                break;
            default:
                auth_mode_str = "Unknown";
        }
        cJSON_AddStringToObject(network, "security", auth_mode_str);
        
        // Check if network is hidden
        bool is_hidden = (ap_records[i].ssid[0] == 0);
        cJSON_AddBoolToObject(network, "is_hidden", is_hidden);
        
        // Add this network to the networks array
        cJSON_AddItemToArray(networks, network);
    }
    
    // Generate JSON string
    char *json_response = cJSON_PrintUnformatted(root);
    if (!json_response) {
        ESP_LOGE(TAG, "Failed to print JSON");
        httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"JSON printing failed\"}");
    } else {
        ESP_LOGI(TAG, "Sending scan results with %d networks", ap_count);
        httpd_resp_sendstr(req, json_response);
        free(json_response);
    }
    
    // Clean up
    cJSON_Delete(root);
    free(ap_records);
    
    esp_err_t restore_err = esp_wifi_set_mode(original_mode);
    if (restore_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to restore original WiFi mode: %s", esp_err_to_name(restore_err));
    }
    
    return ESP_OK;
}

// API handler for system information
static esp_err_t api_system_info_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    
    // Create JSON response
    cJSON *root = cJSON_CreateObject();
    
    // Add IDF version
    cJSON_AddStringToObject(root, "idf_version", esp_get_idf_version());
    
    // Add heap info
    cJSON_AddNumberToObject(root, "free_heap", esp_get_free_heap_size());
    
    // Add MAC address
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    cJSON_AddStringToObject(root, "mac_address", mac_str);
    
    // Add chip info
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    cJSON_AddNumberToObject(root, "chip_model", chip_info.model);
    cJSON_AddNumberToObject(root, "chip_cores", chip_info.cores);
    
    char features[64] = {0};
    if (chip_info.features & CHIP_FEATURE_WIFI_BGN) {
        strcat(features, "WiFi 802.11bgn");
    }
    if (chip_info.features & CHIP_FEATURE_BLE) {
        if (strlen(features) > 0) strcat(features, ", ");
        strcat(features, "BLE");
    }
    cJSON_AddStringToObject(root, "features", features);
    
    // Generate JSON string
    char *json_response = cJSON_PrintUnformatted(root);
    httpd_resp_sendstr(req, json_response);
    
    // Clean up
    free(json_response);
    cJSON_Delete(root);
    
    return ESP_OK;
}

// API handler for antenna settings
static esp_err_t api_antenna_settings_handler(httpd_req_t *req) {
    esp_err_t ret = ESP_OK;
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1);
    
    httpd_resp_set_type(req, "application/json");
    
    int ret_len = httpd_req_recv(req, content, recv_size);
    if (ret_len <= 0) {
        // Handle error
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive data");
        return ESP_FAIL;
    }
    
    content[ret_len] = '\0';
    
    // Parse JSON request
    cJSON *root = cJSON_Parse(content);
    if (!root) {
        httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
        return ESP_FAIL;
    }
    
    // Get the "external_antenna" field from the JSON
    cJSON *ext_antenna_json = cJSON_GetObjectItem(root, "external_antenna");
    if (cJSON_IsBool(ext_antenna_json)) {
        bool use_external = cJSON_IsTrue(ext_antenna_json);
        
        // Store the setting in NVS
        nvs_handle_t nvs_handle;
        ret = nvs_open("storage", NVS_READWRITE, &nvs_handle);
        if (ret == ESP_OK) {
            ret = nvs_set_u8(nvs_handle, "ext_antenna", use_external ? 1 : 0);
            if (ret == ESP_OK) {
                ret = nvs_commit(nvs_handle);
            }
            nvs_close(nvs_handle);
        }
        
        // Log the new antenna setting
        ESP_LOGI(TAG, "Antenna setting changed: %s", use_external ? "External" : "Internal");
        // Note: For ESP32-C5, we can't directly set antenna mode through normal API
        
        // Send response
        if (ret == ESP_OK) {
            httpd_resp_sendstr(req, 
                use_external ? 
                "{\"status\":\"ok\",\"message\":\"External antenna selected. Please reboot for changes to take effect\",\"external_antenna\":true}" : 
                "{\"status\":\"ok\",\"message\":\"Internal antenna selected. Please reboot for changes to take effect\",\"external_antenna\":false}"
            );
        } else {
            httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"Failed to save antenna setting\"}");
        }
    } else {
        httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"Missing external_antenna field\"}");
    }
    
    cJSON_Delete(root);
    return ESP_OK;
}

// API handler to get current antenna settings
static esp_err_t api_get_antenna_settings_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    
    // Get the setting from NVS
    nvs_handle_t nvs_handle;
    uint8_t use_external = 0;
    esp_err_t ret = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (ret == ESP_OK) {
        nvs_get_u8(nvs_handle, "ext_antenna", &use_external);
        nvs_close(nvs_handle);
    }
    
    // Create JSON response
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddBoolToObject(root, "external_antenna", use_external);
    
    char *json_response = cJSON_PrintUnformatted(root);
    if (!json_response) {
        httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"JSON printing failed\"}");
    } else {
        httpd_resp_sendstr(req, json_response);
        free(json_response);
    }
    
    cJSON_Delete(root);
    return ESP_OK;
}

// Translate filter type string to numeric value
static uint8_t get_filter_type(const char* filter_str) {
    if (strcmp(filter_str, "management") == 0) return 1;
    if (strcmp(filter_str, "data") == 0) return 2;
    if (strcmp(filter_str, "control") == 0) return 3;
    if (strcmp(filter_str, "beacon") == 0) return 4;
    if (strcmp(filter_str, "probe") == 0) return 5;
    return 0; // default: all packets
}

// API handler for packet sniffing start
static esp_err_t api_sniff_start_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    
    ESP_LOGI(TAG, "Starting packet sniffer");
    
    // Extract query parameters
    char buf[100];
    buf[0] = '\0';
    
    // Get channel parameter
    int channel = 0; // Default: channel hopping
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char param[32];
        if (httpd_query_key_value(buf, "channel", param, sizeof(param)) == ESP_OK) {
            channel = atoi(param);
        }
    }
    
    // Get filter parameter
    char filter[32] = "all"; // Default: all packets
    if (buf[0] != '\0') { // If we have query parameters
        if (httpd_query_key_value(buf, "filter", filter, sizeof(filter)) != ESP_OK) {
            strcpy(filter, "all"); // Reset to default if not found
        }
    }
    
    // Convert filter string to numeric value
    uint8_t filter_type = get_filter_type(filter);
    
    // Start the sniffer
    bool success = start_wifi_sniffer(channel, filter_type);
    
    // Create response
    cJSON *root = cJSON_CreateObject();
    
    if (success) {
        cJSON_AddStringToObject(root, "status", "success");
        cJSON_AddNumberToObject(root, "channel", channel);
        cJSON_AddStringToObject(root, "filter", filter);
        cJSON_AddStringToObject(root, "message", "Packet capture started");
    } else {
        cJSON_AddStringToObject(root, "status", "error");
        cJSON_AddStringToObject(root, "message", "Failed to start packet capture");
    }
    
    char *json_response = cJSON_PrintUnformatted(root);
    httpd_resp_sendstr(req, json_response);
    
    free(json_response);
    cJSON_Delete(root);
    
    return ESP_OK;
}

// API handler for packet sniffing stop
static esp_err_t api_sniff_stop_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    
    ESP_LOGI(TAG, "Stopping packet sniffer");
    
    // Stop the sniffer
    bool success = stop_wifi_sniffer();
    
    // Create response
    cJSON *root = cJSON_CreateObject();
    
    if (success) {
        cJSON_AddStringToObject(root, "status", "success");
        cJSON_AddStringToObject(root, "message", "Packet capture stopped");
    } else {
        cJSON_AddStringToObject(root, "status", "error");
        cJSON_AddStringToObject(root, "message", "Sniffer was not running");
    }
    
    char *json_response = cJSON_PrintUnformatted(root);
    httpd_resp_sendstr(req, json_response);
    
    free(json_response);
    cJSON_Delete(root);
    
    return ESP_OK;
}

// Helper function to format MAC address
static void format_mac_addr(char *dest, const uint8_t *addr) {
    snprintf(dest, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
             addr[0], addr[1], addr[2],
             addr[3], addr[4], addr[5]);
}

// Helper function to determine packet type from frame control
static const char* get_frame_type_str(uint16_t frame_ctrl) {
    uint8_t type = (frame_ctrl & 0x000C) >> 2;
    uint8_t subtype = (frame_ctrl & 0x00F0) >> 4;
    
    switch (type) {
        case 0: // Management
            switch (subtype) {
                case 0: return "ASSOC_REQ";
                case 1: return "ASSOC_RES";
                case 2: return "REASSOC_REQ";
                case 3: return "REASSOC_RES";
                case 4: return "PROBE_REQ";
                case 5: return "PROBE_RES";
                case 8: return "BEACON";
                case 9: return "ATIM";
                case 10: return "DISASSOC";
                case 11: return "AUTH";
                case 12: return "DEAUTH";
                case 13: return "ACTION";
                default: return "MGMT";
            }
        case 1: // Control
            switch (subtype) {
                case 8: return "BLOCK_ACK_REQ";
                case 9: return "BLOCK_ACK";
                case 10: return "PS_POLL";
                case 11: return "RTS";
                case 12: return "CTS";
                case 13: return "ACK";
                case 14: return "CF_END";
                default: return "CTRL";
            }
        case 2: // Data
            return "DATA";
        default:
            return "UNKNOWN";
    }
}

// API handler for getting captured packets
static esp_err_t api_sniff_packets_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    
    // Allocate memory for packet pointers
    packet_info_t *packets[10];
    int num_packets = get_captured_packets((void**)packets, 10);
    
    // Create response
    cJSON *root = cJSON_CreateObject();
    cJSON *packets_array = cJSON_AddArrayToObject(root, "packets");
    
    for (int i = 0; i < num_packets; i++) {
        packet_info_t *pkt = packets[i];
        
        // Create JSON object for this packet
        cJSON *packet = cJSON_CreateObject();
        
        // Extract header info
        uint16_t frame_ctrl = *(uint16_t*)pkt->data;
        uint8_t *addr1 = pkt->data + 4;  // dst
        uint8_t *addr2 = pkt->data + 10; // src
        
        // Format MAC addresses
        char src_mac[18], dst_mac[18];
        format_mac_addr(dst_mac, addr1);
        format_mac_addr(src_mac, addr2);
        
        // Add packet information
        cJSON_AddStringToObject(packet, "type", get_frame_type_str(frame_ctrl));
        cJSON_AddStringToObject(packet, "src", src_mac);
        cJSON_AddStringToObject(packet, "dst", dst_mac);
        cJSON_AddNumberToObject(packet, "channel", pkt->channel);
        cJSON_AddNumberToObject(packet, "rssi", pkt->rssi);
        
        // Format data as hex
        if (pkt->length > 0) {
            char *hex_data = malloc(pkt->length * 3 + 1);
            if (hex_data) {
                int pos = 0;
                for (int j = 0; j < pkt->length && j < 64; j++) { // Only show first 64 bytes
                    pos += snprintf(hex_data + pos, 4, "%02x ", pkt->data[j]);
                }
                if (pkt->length > 64) {
                    strcat(hex_data, "...");
                }
                cJSON_AddStringToObject(packet, "data", hex_data);
                free(hex_data);
            }
        }
        
        // Add packet to array
        cJSON_AddItemToArray(packets_array, packet);
        
        // Free the packet
        free(pkt);
    }
    
    char *json_response = cJSON_PrintUnformatted(root);
    httpd_resp_sendstr(req, json_response);
    
    free(json_response);
    cJSON_Delete(root);
    
    return ESP_OK;
}

// Register URI handlers
esp_err_t register_uri_handlers(httpd_handle_t server) {
    // API handler for scanning networks
    httpd_uri_t scan_handler = {
        .uri = "/api/scan",
        .method = HTTP_GET,
        .handler = api_scan_networks_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &scan_handler);
    
    // API handler for system info
    httpd_uri_t sysinfo_handler = {
        .uri = "/api/system-info",
        .method = HTTP_GET,
        .handler = api_system_info_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &sysinfo_handler);
    
    // Register packet sniffing handlers
    httpd_uri_t sniff_start_handler = {
        .uri = "/api/sniff/start",
        .method = HTTP_GET,
        .handler = api_sniff_start_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &sniff_start_handler);
    
    httpd_uri_t sniff_stop_handler = {
        .uri = "/api/sniff/stop",
        .method = HTTP_GET,
        .handler = api_sniff_stop_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &sniff_stop_handler);
    
    httpd_uri_t sniff_packets_handler = {
        .uri = "/api/sniff/packets",
        .method = HTTP_GET,
        .handler = api_sniff_packets_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &sniff_packets_handler);
    
    // Register antenna settings endpoints
    httpd_uri_t antenna_settings_uri = {
        .uri = "/api/antenna",
        .method = HTTP_POST,
        .handler = api_antenna_settings_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &antenna_settings_uri);
    
    httpd_uri_t get_antenna_settings_uri = {
        .uri = "/api/antenna",
        .method = HTTP_GET,
        .handler = api_get_antenna_settings_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &get_antenna_settings_uri);
    
    // Handler for static files (this will only handle index.html from embedded string)
    httpd_uri_t file_handler = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = http_serve_file,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &file_handler);
    
    return ESP_OK;
}

// Start the HTTP server
httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 16;  // Increased to handle all our routes
    
    // Increase timeout values to prevent connection issues
    config.recv_wait_timeout = 10;    // Default is 5 seconds
    config.send_wait_timeout = 10;    // Default is 5 seconds
    config.lru_purge_enable = true;   // Enable LRU purging to free sockets when necessary
    
    ESP_LOGI(TAG, "Starting HTTP server on port: '%d'", config.server_port);
    
    if (httpd_start(&server, &config) == ESP_OK) {
        register_uri_handlers(server);
        return server;
    }
    
    ESP_LOGE(TAG, "Failed to start HTTP server!");
    return NULL;
}

// Stop the HTTP server
void stop_webserver(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
}

// Init web server
void init_web_server(void) {
    ESP_LOGI(TAG, "Initializing web server");
    
    // Start server
    server = start_webserver();
    
    if (server) {
        ESP_LOGI(TAG, "Web server started successfully");
    } else {
        ESP_LOGE(TAG, "Failed to start web server");
    }
} 