/*
 * SPDX-FileCopyrightText: 2026 ALIENTEK
 * SPDX-License-Identifier: Apache-2.0
 */

/* gen_skip removes the device dependency as well as its generated config. */
#if __has_include("atk_ap3216c.h")
#include <stdlib.h>
#include "esp_log.h"
#include "dev_custom.h"
#include "esp_board_periph.h"
#include "gen_board_device_custom.h"
#include "atk_dnesp32s3_sensors.h"

static const char *TAG = "ATK_AP3216C";

static esp_err_t ap3216c_deinit(void *device_handle)
{
    atk_dnesp32s3_ap3216c_handles_t *handles = device_handle;
    if (handles == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (handles->sensor != NULL) {
        esp_err_t ret = atk_ap3216c_stop(handles->sensor);
        if (ret != ESP_OK) {
            return ret;
        }
        ret = atk_ap3216c_delete(handles->sensor);
        if (ret != ESP_OK) {
            return ret;
        }
        handles->sensor = NULL;
    }
    esp_err_t ret = esp_board_periph_unref_handle(handles->peripheral_name);
    if (ret == ESP_OK) {
        free(handles);
    }
    return ret;
}

static esp_err_t ap3216c_init(void *cfg, int cfg_size, void **device_handle)
{
    if (device_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *device_handle = NULL;
    const dev_custom_ap3216c_config_t *config = cfg;
    if (config == NULL || cfg_size != sizeof(*config) || config->peripheral_count != 1 ||
        config->peripheral_name == NULL || config->i2c_addr != 0x1e ||
        config->frequency_hz <= 0 || config->frequency_hz > 400000) {
        return ESP_ERR_INVALID_ARG;
    }
    atk_dnesp32s3_ap3216c_handles_t *handles = calloc(1, sizeof(*handles));
    if (handles == NULL) {
        return ESP_ERR_NO_MEM;
    }
    i2c_master_bus_handle_t bus = NULL;
    esp_err_t ret = esp_board_periph_ref_handle(config->peripheral_name, (void **)&bus);
    if (ret != ESP_OK) {
        free(handles);
        return ret;
    }
    handles->peripheral_name = config->peripheral_name;
    ret = atk_ap3216c_create(bus, config->i2c_addr, config->frequency_hz, &handles->sensor);
    if (ret == ESP_OK) {
        ret = atk_ap3216c_start(handles->sensor);
    }
    if (ret != ESP_OK) {
        if (handles->sensor != NULL) {
            esp_err_t stop_ret = atk_ap3216c_stop(handles->sensor);
            if (stop_ret != ESP_OK) {
                ESP_LOGW(TAG, "Power-down after init failure: %s", esp_err_to_name(stop_ret));
            }
            esp_err_t delete_ret = atk_ap3216c_delete(handles->sensor);
            if (delete_ret != ESP_OK) {
                ESP_LOGE(TAG, "Detach after init failure: %s", esp_err_to_name(delete_ret));
                /* Keep the parent bus referenced while the I2C device exists. */
                free(handles);
                return ret;
            }
        }
        esp_err_t unref_ret = esp_board_periph_unref_handle(handles->peripheral_name);
        if (unref_ret != ESP_OK) {
            ESP_LOGE(TAG, "Bus release after init failure: %s", esp_err_to_name(unref_ret));
        }
        free(handles);
        return ret;
    }
    ESP_LOGI(TAG, "Continuous ALS/PS/IR enabled at 7-bit address 0x%02x", config->i2c_addr);
    *device_handle = handles;
    return ESP_OK;
}

CUSTOM_DEVICE_IMPLEMENT(ap3216c, ap3216c_init, ap3216c_deinit);
#endif
