/*
 * SPDX-FileCopyrightText: 2026 ALIENTEK
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "atk_ap3216c.h"

#define AP3216C_TIMEOUT_MS  (1000)

struct atk_ap3216c {
    i2c_master_dev_handle_t  device;
};

static esp_err_t write_mode(atk_ap3216c_handle_t handle, uint8_t mode)
{
    const uint8_t command[] = { 0x00, mode };
    return i2c_master_transmit(handle->device, command, sizeof(command), AP3216C_TIMEOUT_MS);
}

static esp_err_t read_register(atk_ap3216c_handle_t handle, uint8_t reg, uint8_t *value)
{
    return i2c_master_transmit_receive(handle->device, &reg, 1, value, 1, AP3216C_TIMEOUT_MS);
}

esp_err_t atk_ap3216c_create(i2c_master_bus_handle_t bus, uint8_t address,
    uint32_t frequency_hz, atk_ap3216c_handle_t *out_handle)
{
    if (out_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_handle = NULL;
    if (bus == NULL || address > 0x7f || frequency_hz == 0 || frequency_hz > 400000) {
        return ESP_ERR_INVALID_ARG;
    }
    atk_ap3216c_handle_t handle = calloc(1, sizeof(*handle));
    if (handle == NULL) {
        return ESP_ERR_NO_MEM;
    }
    const i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = frequency_hz,
    };
    esp_err_t ret = i2c_master_bus_add_device(bus, &config, &handle->device);
    if (ret != ESP_OK) {
        free(handle);
        return ret;
    }
    *out_handle = handle;
    return ESP_OK;
}

esp_err_t atk_ap3216c_start(atk_ap3216c_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = write_mode(handle, 0x04);
    if (ret != ESP_OK) {
        return ret;
    }
    /* The datasheet requires no commands for at least 10 ms after reset. */
    vTaskDelay(pdMS_TO_TICKS(50) + 1);
    ret = write_mode(handle, 0x03);
    if (ret != ESP_OK) {
        return ret;
    }
    uint8_t mode = 0;
    ret = read_register(handle, 0x00, &mode);
    if (ret != ESP_OK) {
        return ret;
    }
    if (mode != 0x03) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    /* Add a tick so a delay starting just before a tick still covers 120 ms. */
    vTaskDelay(pdMS_TO_TICKS(120) + 1);
    return ESP_OK;
}

esp_err_t atk_ap3216c_read(atk_ap3216c_handle_t handle, atk_ap3216c_sample_t *sample)
{
    if (handle == NULL || sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t data[6];
    /* Reading each low byte latches its high byte. Do not assume auto-increment. */
    for (uint8_t i = 0; i < sizeof(data); ++i) {
        esp_err_t ret = read_register(handle, 0x0a + i, &data[i]);
        if (ret != ESP_OK) {
            return ret;
        }
    }
    *sample = (atk_ap3216c_sample_t) {
        .als_raw = ((uint16_t)data[3] << 8) | data[2],
        .ir_raw = ((uint16_t)data[1] << 2) | (data[0] & 0x03),
        .ps_raw = ((uint16_t)(data[5] & 0x3f) << 4) | (data[4] & 0x0f),
        .ir_valid = !(data[0] & 0x80),
        .ps_valid = !(data[4] & 0x40),
        .object_detected = !!(data[4] & 0x80),
    };
    return ESP_OK;
}

esp_err_t atk_ap3216c_stop(atk_ap3216c_handle_t handle)
{
    return handle == NULL ? ESP_ERR_INVALID_ARG : write_mode(handle, 0x00);
}

esp_err_t atk_ap3216c_delete(atk_ap3216c_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = i2c_master_bus_rm_device(handle->device);
    if (ret == ESP_OK) {
        free(handle);
    }
    return ret;
}
