<div align="center">

# ESP32-C5 J4CK3D

![ESP32-C5 Wifi](https://img.shields.io/badge/ESP32--C5-WiFi-blue.svg) ![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg) ![Platform](https://img.shields.io/badge/Platform-ESP--IDF-red.svg) ![GitHub issues](https://img.shields.io/github/issues/Evil-Project-Team/ESP32-C5-J4CK3D) ![GitHub stars](https://img.shields.io/github/stars/Evil-Project-Team/ESP32-C5-J4CK3D?style=social) ![GitHub forks](https://img.shields.io/github/forks/Evil-Project-Team/ESP32-C5-J4CK3D?style=social)

</div>

<p align="center">
  <i>An advanced WiFi scanning and packet sniffing toolkit for the ESP32-C5, featuring a cyberpunk-inspired web interface.</i>
</p>

## 📡 Overview

ESP32-C5 J4CK3D transforms your ESP32-C5 device into a powerful WiFi analysis toolkit. With an intuitive web interface designed with a cyberpunk aesthetic, it provides real-time network scanning and packet sniffing capabilities in a portable form factor.

Designed for network administrators, security researchers, and IoT developers, this toolkit offers valuable insights into your wireless environment.

## ✨ Features

- **R4d4r Sc4n**: Scan and analyze WiFi networks in your vicinity
  - Detect hidden networks
  - View detailed information (SSID, MAC address, signal strength, channel, security)
  - Identify WiFi spectrum usage
  
- **P4ck3t Sn1ff3r**: Capture and analyze WiFi packets
  - Monitor traffic across all channels or focus on specific ones
  - Filter packets by type (management frames, data frames, control frames)
  - Real-time packet capture and display in table format
  
- **C0mm4nd D3ck**: System dashboard with device information
  - Display runtime statistics
  - System status monitoring
  
- **Responsive Web Interface**:
  - Access all features through any device with a web browser
  - Cyberpunk-themed UI with visual effects
  - Mobile-friendly layout

## 🛠️ Hardware Requirements

- ESP32-C5 based development board (all variants supported)
- USB-C cable for power and programming
- Power supply (battery or USB power)

### 🔄 Supported Boards

This project is designed to work with any ESP32-C5 based board:

- **ESP32-C5-DevKitC-1** (official Espressif development board)
- **ESP32-C5-MINI-1** modules and development boards
- **Generic ESP32-C5** boards from various manufacturers

A board configuration system is included to support the different GPIO mappings across various ESP32-C5 boards.

## 📦 Installation

### Prerequisites

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c5/get-started/index.html) (Espressif IoT Development Framework)
- Git

### Build and Flash

1. Clone the repository:
   ```bash
   git clone https://github.com/Evil-Project-Team/ESP32-C5-J4CK3D
   cd ESP32-C5-J4CK3D
   ```

2. Configure, build and flash:
   ```bash
   idf.py --preview set-target esp32c5
   idf.py menuconfig  # Optional: Configure WiFi credentials and other parameters
   idf.py build
   idf.py -p [PORT] flash
   ```

3. Monitor the device output:
   ```bash
   idf.py -p [PORT] monitor
   ```

### 🔧 Adapting for Your ESP32-C5 Board

If you're using a different ESP32-C5 board than the default ESP32-C5-DevKitC:

1. Edit the `main/board_config.h` file to match your board's specifications:
   - Set the appropriate board define (`BOARD_ESP32_C5_DEVKITC` or `BOARD_ESP32_C5_GENERIC`)
   - Adjust GPIO pin assignments if they differ from the defaults
   - Configure WiFi parameters as needed

2. Common adjustments needed:
   - External/internal antenna selection
   - Regional WiFi settings

## 🚀 Usage

1. Power on your ESP32-C5 device
2. Connect to the "ESP32-C5-J4CK3D" WiFi network (default password: "h4ck3rm4n")
3. Navigate to `http://192.168.4.1` in your web browser
4. Access the different tools through the side navigation menu:
   - **C0mm4nd D3ck**: View system information
   - **R4d4r Sc4n**: Scan for WiFi networks
   - **P4ck3t Sn1ff3r**: Capture and analyze WiFi packets
   - **Sy5t3m C0nfig**: Configure device settings

### WiFi Scanning

- Click the "Sc4n F0r N3tw0rk5" button to initiate a scan
- View detected networks in the table, including hidden networks
- Results show SSID, BSSID, band, channel, signal strength, and security details

### Packet Sniffing

1. Select the desired channel (1-13 or all channels)
2. Choose a filter type (all packets, management frames, data frames, etc.)
3. Click "ST4RT SN1FF1NG" to begin capturing packets
4. View captured packets in the table display
5. Use "CL34R L0G" to reset the packet display
6. Click "ST0P SN1FF1NG" when finished

## 📊 Project Structure

```
ESP32-C5-J4CK3D/
├── main/
│   ├── CMakeLists.txt     # Project configuration
│   ├── main.c             # Application entry point
│   ├── menu.c             # Console menu system
│   ├── web_server.c       # Web interface and API endpoints
│   ├── wifi_init.c        # WiFi initialization and configuration
│   ├── wifi_sniffer.c     # Packet sniffing implementation
│   ├── board_config.h     # Hardware-specific board configuration
│   └── headers (.h files) # Component headers
├── CMakeLists.txt         # Project configuration
└── README.md              # Project documentation
```

## 🔒 Security Notice

This tool is intended for:
- Network debugging
- Educational purposes
- Security research on your own networks

Always respect privacy and adhere to local regulations when using WiFi monitoring tools. Unauthorized network access may be illegal in your jurisdiction.

## 🤝 Contributions

Contributions are welcome! Please feel free to submit a Pull Request.

## 📜 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🔧 Created By

Developed by the [Evil Project Team](https://github.com/Evil-Project-Team)
  - 🛠 [7h30th3r0n3](https://github.com/7h30th3r0n3)  
  - 🤖 [dagnazty](https://github.com/dagnazty)  
  - 🦾 [Hosseios](https://github.com/Hosseios)

---

<p align="center">
  <i>ESP32-C5 J4CK3D - Explore. Analyze. Understand.</i>
</p> 