#include "hid_service.h"
#include <string.h>
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "HID_SERVICE";
static uint8_t hid_addr_type;
static uint16_t hid_h_conn;
static int ble_hid_gap_event(struct ble_gap_event *event, void *arg);
static void ble_host_task(void *param);

// Handles for notifications
static uint16_t keyboard_report_handle;
static uint16_t mouse_report_handle;

// HID Report Map (Keyboard + Mouse)
static const uint8_t hid_report_map[] = {
    0x05, 0x01,                    // Usage Page (Generic Desktop)
    0x09, 0x06,                    // Usage (Keyboard)
    0xa1, 0x01,                    // Collection (Application)
    0x85, 0x01,                    //   Report ID (1)
    0x05, 0x07,                    //   Usage Page (Keyboard/Keypad)
    0x19, 0xe0,                    //   Usage Minimum (Keyboard LeftControl)
    0x29, 0xe7,                    //   Usage Maximum (Keyboard Right GUI)
    0x15, 0x00,                    //   Logical Minimum (0)
    0x25, 0x01,                    //   Logical Maximum (1)
    0x75, 0x01,                    //   Report Size (1)
    0x95, 0x08,                    //   Report Count (8)
    0x81, 0x02,                    //   Input (Data,Var,Abs)
    0x95, 0x01,                    //   Report Count (1)
    0x75, 0x08,                    //   Report Size (8)
    0x81, 0x01,                    //   Input (Cnst,Ary,Abs)
    0x95, 0x06,                    //   Report Count (6)
    0x75, 0x08,                    //   Report Size (8)
    0x15, 0x00,                    //   Logical Minimum (0)
    0x25, 0x65,                    //   Logical Maximum (101)
    0x05, 0x07,                    //   Usage Page (Keyboard/Keypad)
    0x19, 0x00,                    //   Usage Minimum (Reserved (no event indicated))
    0x29, 0x65,                    //   Usage Maximum (Keyboard Application)
    0x81, 0x00,                    //   Input (Data,Ary,Abs)
    0xc0,                          // End Collection

    0x05, 0x01,                    // Usage Page (Generic Desktop)
    0x09, 0x02,                    // Usage (Mouse)
    0xa1, 0x01,                    // Collection (Application)
    0x85, 0x02,                    //   Report ID (2)
    0x09, 0x01,                    //   Usage (Pointer)
    0xa1, 0x00,                    //   Collection (Physical)
    0x05, 0x09,                    //     Usage Page (Button)
    0x19, 0x01,                    //     Usage Minimum (Button 1)
    0x29, 0x03,                    //     Usage Maximum (Button 3)
    0x15, 0x00,                    //     Logical Minimum (0)
    0x25, 0x01,                    //     Logical Maximum (1)
    0x75, 0x01,                    //     Report Size (1)
    0x95, 0x03,                    //     Report Count (3)
    0x81, 0x02,                    //     Input (Data,Var,Abs)
    0x95, 0x01,                    //     Report Count (1)
    0x75, 0x05,                    //     Report Size (5)
    0x81, 0x03,                    //     Input (Cnst,Var,Abs)
    0x05, 0x01,                    //     Usage Page (Generic Desktop)
    0x09, 0x30,                    //     Usage (X)
    0x09, 0x31,                    //     Usage (Y)
    0x15, 0x81,                    //     Logical Minimum (-127)
    0x25, 0x7f,                    //     Logical Maximum (127)
    0x75, 0x08,                    //     Report Size (8)
    0x95, 0x02,                    //     Report Count (2)
    0x81, 0x06,                    //     Input (Data,Var,Rel)
    0xc0,                          //   End Collection
    0xc0                           // End Collection
};

// --- GATT Access Callbacks ---

static int ble_hid_gatt_svr_read_report_map(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    os_mbuf_append(ctxt->om, hid_report_map, sizeof(hid_report_map));
    return 0;
}

static int ble_hid_gatt_svr_read_report_reference(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (arg) {
        os_mbuf_append(ctxt->om, arg, 2);
    }
    return 0;
}

static int ble_hid_gatt_svr_access_dummy(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR && arg != NULL) {
        os_mbuf_append(ctxt->om, arg, 4); // Handle the 4-byte HID Info or similar
        return 0;
    }
    return 0;
}

static const struct ble_gatt_svc_def ble_hid_gatt_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x1812), // HID Service
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(0x2a4b), // Report Map
                .access_cb = ble_hid_gatt_svr_read_report_map,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = BLE_UUID16_DECLARE(0x2a4d), // Report (Keyboard)
                .val_handle = &keyboard_report_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .access_cb = ble_hid_gatt_svr_access_dummy,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = BLE_UUID16_DECLARE(0x2908), // Report Reference
                        .access_cb = ble_hid_gatt_svr_read_report_reference,
                        .arg = (uint8_t[]){0x01, 0x01}, // ID 1, Type Input
                    },
                    { 0 }
                }
            },
            {
                .uuid = BLE_UUID16_DECLARE(0x2a4d), // Report (Mouse)
                .val_handle = &mouse_report_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .access_cb = ble_hid_gatt_svr_access_dummy,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = BLE_UUID16_DECLARE(0x2908), // Report Reference
                        .access_cb = ble_hid_gatt_svr_read_report_reference,
                        .arg = (uint8_t[]){0x02, 0x01}, // ID 2, Type Input
                    },
                    { 0 }
                }
            },
            {
                .uuid = BLE_UUID16_DECLARE(0x2a4a), // HID Information
                .access_cb = ble_hid_gatt_svr_access_dummy,
                .flags = BLE_GATT_CHR_F_READ,
                .arg = (uint8_t[]){0x11, 0x01, 0x00, 0x01}, // ver 1.1, country 0, flags 1 (RemoteWake)
            },
            {
                .uuid = BLE_UUID16_DECLARE(0x2a4c), // HID Control Point
                .access_cb = ble_hid_gatt_svr_access_dummy,
                .flags = BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            { 0 }
        },
    },
    { 0 }
};

// --- GAP Handling ---

static void ble_hid_advertise(void) {
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));
    fields.name = (uint8_t *)"ESP32-HID";
    fields.name_len = strlen("ESP32-HID");
    fields.appearance = 0x03c1; // Keyboard
    fields.appearance_is_present = 1;
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    ble_gap_adv_set_fields(&fields);
    ble_gap_adv_start(hid_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_hid_gap_event, NULL);
}

static int ble_hid_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                hid_h_conn = event->connect.conn_handle;
                ESP_LOGI(TAG, "Connected");
            } else {
                ble_hid_advertise();
            }
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Disconnected");
            ble_hid_advertise();
            break;
    }
    return 0;
}

static void ble_hid_on_sync(void) {
    ble_hs_util_ensure_addr(0);
    ble_hs_id_infer_auto(0, &hid_addr_type);
    ble_hid_advertise();
}

static void ble_host_task(void *param) {
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// --- Public API ---

void hid_init(void) {
    ESP_LOGI(TAG, "hid_init: step 1 - nimble_port_init");
    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init failed: %d", ret);
        return;
    }

    ESP_LOGI(TAG, "hid_init: step 2 - svc inits");
    ble_svc_gap_init();
    ble_svc_gatt_init();
    
    ESP_LOGI(TAG, "hid_init: step 3 - config count & add svcs");
    ble_gatts_count_cfg(ble_hid_gatt_services);
    ble_gatts_add_svcs(ble_hid_gatt_services);

    ESP_LOGI(TAG, "hid_init: step 4 - setup gap name & sync_cb");
    ble_hs_cfg.sync_cb = ble_hid_on_sync;
    ble_svc_gap_device_name_set("ESP32-C6-HID");
    
    ESP_LOGI(TAG, "hid_init: step 5 - nimble_port_freertos_init");
    nimble_port_freertos_init(ble_host_task);
    ESP_LOGI(TAG, "hid_init: complete");
}

void send_keyboard_report(uint8_t modifier, uint8_t keycode[6]) {
    uint8_t report[8] = {modifier, 0, keycode[0], keycode[1], keycode[2], keycode[3], keycode[4], keycode[5]};
    struct os_mbuf *om = ble_hs_mbuf_from_flat(report, sizeof(report));
    ble_gatts_notify_custom(hid_h_conn, keyboard_report_handle, om);
}

void key_press(uint8_t key) {
    uint8_t keys[6] = {key, 0, 0, 0, 0, 0};
    send_keyboard_report(0, keys);
}

void key_release(void) {
    uint8_t keys[6] = {0, 0, 0, 0, 0, 0};
    send_keyboard_report(0, keys);
}

void send_mouse_report(uint8_t buttons, int8_t x, int8_t y, int8_t wheel) {
    uint8_t report[4] = {buttons, (uint8_t)x, (uint8_t)y, (uint8_t)wheel};
    struct os_mbuf *om = ble_hs_mbuf_from_flat(report, sizeof(report));
    ble_gatts_notify_custom(hid_h_conn, mouse_report_handle, om);
}
