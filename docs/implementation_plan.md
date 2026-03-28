# Implementation Plan: Wireless HID Control System (ESP32-C6 & Flutter)

This plan outlines the creation of a production-ready wireless HID controller. Since the ESP32-C6 native USB port is fixed for Serial/JTAG and excludes native USB-OTG, we will implement **HID via Bluetooth Low Energy (BLE)** and **Control via WiFi (WebSocket)**.

> [!IMPORTANT]
> **Self-Hosted Web Remote**: The ESP32-C6 will host its own **Web Dashboard**. You won't need to install any mobile apps; just open your phone's browser and go to the ESP32's IP address.

> [!TIP]
> This "Zero-Install" approach is extremely robust and works on any device (iOS, Android, Windows, Mac).

## Proposed Changes

### 1. Firmware (ESP-IDF)
The firmware will be built using ESP-IDF v5.x.

- **WiFi Module**: Connects to a local network and hosts a WebSocket server.
- **WebSocket Server**: Listens for JSON commands from the Flutter app.
- **BLE HID Module**: Implements the HID over GATT (HoG) profile to emulate a Keyboard/Mouse.
- **Command Dispatcher**: Parses JSON and triggers BLE reports.

#### [NEW] [firmware/main/main.c](file:///C:/Projects/ESP-HID-Control/firmware/main/main.c)
Main entry point, initializes NVS, WiFi, BLE, and WebSocket.

#### [NEW] [firmware/main/hid_service.c](file:///C:/Projects/ESP-HID-Control/firmware/main/hid_service.c)
Handles BLE HID descriptor setup and sending input reports (keys, mouse).

#### [NEW] [firmware/main/web_server.c](file:///C:/Projects/ESP-HID-Control/firmware/main/web_server.c)
Implements the WebSocket server and JSON command parsing.

---

### 2. Web Interface (Self-Hosted)
A modern, responsive web application served directly by the ESP32.

- **Embedded Assets**: HTML, CSS, and JS files compressed and stored in the ESP32's internal Flash.
- **WebSocket Client**: Built-in JS logic to maintain a real-time connection to the ESP32.
- **Premium UI Features**:
    - **Dark Glassmorphism Design**: Using blur effects and gradients for a premium feel.
    - **Haptic Feedback**: Leveraging the browser's vibration API for physical button feel.
    - **Dynamic Layout**: Optimizes for both portrait (phone) and landscape (tablet) orientations.

#### [NEW] [firmware/main/web/index.html](file:///C:/Projects/ESP-HID-Control/firmware/main/web/index.html)
The complete UI and logic in a single optimized file.

---

### 3. Command Protocol (JSON)
Efficient and extensible format for HID actions (Same as previous plan).

```json
{
  "type": "keyboard",
  "action": "tap",
  "key": 4
}
```

## Open Questions

- None. Both WiFi mode (Home WiFi) and HID transport (BLE) have been confirmed.

## Verification Plan

### Automated Tests
-   Unit tests for the JSON parser in ESP-IDF.
-   WebSocket connectivity tests (Flutter ping/pong).

### Manual Verification
1.  Flash firmware and verify "ESP32-HID" appears in Bluetooth settings on a PC/Laptop.
2.  Connect Flutter app to ESP32 via WiFi.
3.  Send a "String" command and verify text appears in a text editor on the PC.
4.  Test multi-key combinations (Alt+Tab).
