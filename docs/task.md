# Task List: Wireless HID Control System

## 1. Firmware (ESP32-C6)
- [x] Initialize ESP-IDF project for ESP32-C6
- [x] Configure WiFi (STA mode) with hardcoded credentials / Kconfig
- [x] Implement BLE HID Service using NimBLE
    - [x] Define HID Report Descriptors (Keyboard + Mouse)
    - [x] Handle BLE advertising and connection events
- [x] Implement WebSocket Server
    - [x] Create HTTPd server with WS support
    - [x] Implement JSON parser for incoming commands
- [x] Connect Command Dispatcher to BLE HID functions
- [x] Test BLE HID connection with a PC

## 2. Web Interface (Self-Hosted)
- [x] Create `index.html` with modern Glassmorphism UI
- [x] Implement Keyboard layout with mapping to HID keycodes
- [x] Implement Mouse trackpad with Touch events
- [x] Implement WebSocket client in JavaScript
- [x] Embed the web assets into the ESP-IDF binary
- [x] Update `web_server.c` to serve static files

## 3. Integration & Testing
- [ ] Perform end-to-end test (App -> ESP32 -> PC)
- [ ] Refine latency and error handling
- [ ] Add visual feedback on the app (connection status, battery, etc.)
