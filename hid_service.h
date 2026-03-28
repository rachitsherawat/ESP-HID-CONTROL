#ifndef HID_SERVICE_H
#define HID_SERVICE_H

#include <stdint.h>
#include <esp_err.h>

void hid_init(void);

// Keyboard Send Functions
void send_keyboard_report(uint8_t modifier, uint8_t keycode[6]);
void key_press(uint8_t keycode);
void key_release(void);

// Mouse Send Functions
void send_mouse_report(uint8_t buttons, int8_t x, int8_t y, int8_t wheel);

#endif // HID_SERVICE_H
