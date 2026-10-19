/*
 * SPDX-FileCopyrightText: 2026 ALIENTEK
 * SPDX-License-Identifier: Apache-2.0
 */

/* Keep the generated config and dependency under the same gen_skip guard. */
#if __has_include("qma6100p.h")
#include <stdlib.h>
#include "esp_log.h"
#include "dev_custom.h"
#include "esp_board_periph.h"
#include "gen_board_device_custom.h"
#include "atk_dnesp32s3_sensors.h"

static const char *TAG = "ATK_QMA6100P";

static esp_err_t qma6100p_deinit(void *device_handle)
{
    atk_dnesp32s3_qma6100p_handles_t *handles = device_handle;
    if (handles == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (handles->sensor != NULL) {
        /* The upstream delete API detaches the device, not the shared bus. */
        qma6100p_delete(handles->sensor);
        handles->sensor = NULL;
    }
    esp_err_t ret = esp_board_periph_unref_handle(handles->peripheral_name);
    if (ret == ESP_OK) {
        free(handles);
    }
    return ret;
}

static esp_err_t qma6100p_init(void *cfg, int cfg_size, void **device_handle)
{
    if (device_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *device_handle = NULL;
    const dev_custom_qma6100p_config_t *config = cfg;
    if (config == NULL || cfg_size != sizeof(*config) || config->peripheral_count != 1 ||
        config->peripheral_name == NULL || (config->i2c_addr != 0x12 && config->i2c_addr != 0x13)) {
        return ESP_ERR_INVALID_ARG;
    }
    qma6100p_acce_fs_t range;
    switch (config->range_g) {
        case 2:
            range = ACCE_FS_2G;
            break;
        case 4:
            range = ACCE_FS_4G;
            break;
        case 8:
            range = ACCE_FS_8G;
            break;
        case 16:
            range = ACCE_FS_16G;
            break;
        case 32:
            range = ACCE_FS_32G;
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }
    atk_dnesp32s3_qma6100p_handles_t *handles = calloc(1, sizeof(*handles));
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
    ret = qma6100p_create(bus, config->i2c_addr, &handles->sensor);
    uint8_t id = 0;
    if (ret == ESP_OK) {
        ret = qma6100p_get_deviceid(handles->sensor, &id);
    }
    if (ret == ESP_OK && id != QMA6100P_WHO_AM_I_VAL) {
        ESP_LOGE(TAG, "Unexpected chip ID: 0x%02x", id);
        ret = ESP_ERR_INVALID_RESPONSE;
    }
    if (ret == ESP_OK) {
        ret = qma6100p_nvm_load(handles->sensor);
    }
    if (ret == ESP_OK) {
        ret = qma6100p_wake_up(handles->sensor);
    }
    if (ret == ESP_OK) {
        ret = qma6100p_config(handles->sensor, range);
    }
    if (ret != ESP_OK) {
        esp_err_t cleanup_ret = qma6100p_deinit(handles);
        if (cleanup_ret != ESP_OK) {
            ESP_LOGE(TAG, "Cleanup after init failure: %s", esp_err_to_name(cleanup_ret));
        }
        return ret;
    }
    ESP_LOGI(TAG, "Chip ID 0x%02x, range +/- %d g", id, config->range_g);
    *device_handle = handles;
    return ESP_OK;
}

CUSTOM_DEVICE_IMPLEMENT(qma6100p, qma6100p_init, qma6100p_deinit);
#endif
