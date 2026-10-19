/*
 * SPDX-FileCopyrightText: 2026 ALIENTEK
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#if __has_include("esp_io_expander.h")
#include "esp_board_device.h"
#include "esp_io_expander.h"
#include "atk_dnesp32s3_xl9555.h"

#define ATK_DNESP32S3_XL9555_DIRECTION_MASK  0xF003U
#define ATK_DNESP32S3_XL9555_OUTPUT_MASK     0x0FFCU

typedef struct {
    esp_io_expander_t  base;
    i2c_master_dev_handle_t  device;
    uint16_t  output;
    uint16_t  direction;
} xl9555_t;

static const char *TAG = "ATK_DNESP32S3_XL9555";

static esp_err_t xl9555_write_register(xl9555_t *chip, uint8_t reg, uint16_t value)
{
    const uint8_t data[] = { reg, (uint8_t)value, (uint8_t)(value >> 8) };
    return i2c_master_transmit(chip->device, data, sizeof(data), 1000);
}

static esp_err_t xl9555_read_input(esp_io_expander_handle_t handle, uint32_t *value)
{
    xl9555_t *chip = (xl9555_t *)handle;
    const uint8_t reg = 0;
    uint8_t data[2];
    esp_err_t ret = i2c_master_transmit_receive(chip->device, &reg, sizeof(reg), data, sizeof(data), 1000);
    if (ret == ESP_OK) {
        *value = data[0] | ((uint32_t)data[1] << 8);
    }
    return ret;
}

static esp_err_t xl9555_write_output(esp_io_expander_handle_t handle, uint32_t value)
{
    xl9555_t *chip = (xl9555_t *)handle;
    esp_err_t ret = xl9555_write_register(chip, 2, (uint16_t)value);
    if (ret == ESP_OK) {
        chip->output = (uint16_t)value;
    }
    return ret;
}

static esp_err_t xl9555_read_output(esp_io_expander_handle_t handle, uint32_t *value)
{
    *value = ((xl9555_t *)handle)->output;
    return ESP_OK;
}

static esp_err_t xl9555_write_direction(esp_io_expander_handle_t handle, uint32_t value)
{
    xl9555_t *chip = (xl9555_t *)handle;
    esp_err_t ret = xl9555_write_register(chip, 6, (uint16_t)value);
    if (ret == ESP_OK) {
        chip->direction = (uint16_t)value;
    }
    return ret;
}

static esp_err_t xl9555_read_direction(esp_io_expander_handle_t handle, uint32_t *value)
{
    *value = ((xl9555_t *)handle)->direction;
    return ESP_OK;
}

static esp_err_t xl9555_reset(esp_io_expander_handle_t handle)
{
    ESP_RETURN_ON_ERROR(xl9555_write_direction(handle, UINT16_MAX), TAG, "Reset direction failed");
    return xl9555_write_output(handle, UINT16_MAX);
}

static esp_err_t xl9555_delete(esp_io_expander_handle_t handle)
{
    xl9555_t *chip = (xl9555_t *)handle;
    esp_err_t ret = i2c_master_bus_rm_device(chip->device);
    if (ret == ESP_OK) {
        free(chip);
    }
    return ret;
}

esp_err_t atk_dnesp32s3_xl9555_get(atk_dnesp32s3_xl9555_handle_t **out_handle)
{
    if (out_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_handle = NULL;
    esp_io_expander_handle_t *device = NULL;
    esp_err_t ret = esp_board_device_get_handle("xl9555", (void **)&device);
    if (ret == ESP_OK) {
        if (device == NULL || *device == NULL) {
            return ESP_ERR_INVALID_STATE;
        }
        *out_handle = *device;
    }
    return ret;
}

esp_err_t atk_dnesp32s3_xl9555_get_level(atk_dnesp32s3_xl9555_handle_t *handle,
    atk_dnesp32s3_xl9555_pin_t pin, uint8_t *level)
{
    if (handle == NULL || level == NULL || (unsigned)pin > ATK_DNESP32S3_XL9555_KEY0) {
        return ESP_ERR_INVALID_ARG;
    }
    uint32_t input = 0;
    esp_err_t ret = esp_io_expander_get_level(handle, 1U << pin, &input);
    if (ret == ESP_OK) {
        *level = (input >> pin) & 1U;
    }
    return ret;
}

esp_err_t atk_dnesp32s3_xl9555_set_level(atk_dnesp32s3_xl9555_handle_t *handle,
    atk_dnesp32s3_xl9555_pin_t pin, uint8_t level)
{
    if (handle == NULL || (unsigned)pin > ATK_DNESP32S3_XL9555_KEY0 || level > 1 ||
        (ATK_DNESP32S3_XL9555_DIRECTION_MASK & (1U << pin)) != 0) {
        return ESP_ERR_INVALID_ARG;
    }
    /* Use the upstream driver's synchronized output cache for all board controls. */
    return esp_io_expander_set_level(handle, 1U << pin, level);
}

esp_err_t atk_dnesp32s3_xl9555_set_beep(atk_dnesp32s3_xl9555_handle_t *handle, bool enabled)
{
    return atk_dnesp32s3_xl9555_set_level(handle, ATK_DNESP32S3_XL9555_BEEP, enabled ? 0 : 1);
}

esp_err_t atk_dnesp32s3_xl9555_set_speaker(atk_dnesp32s3_xl9555_handle_t *handle, bool enabled)
{
    return atk_dnesp32s3_xl9555_set_level(handle, ATK_DNESP32S3_XL9555_SPK_EN, enabled ? 0 : 1);
}

esp_err_t atk_dnesp32s3_xl9555_set_lcd_backlight(atk_dnesp32s3_xl9555_handle_t *handle, bool enabled)
{
    return atk_dnesp32s3_xl9555_set_level(handle, ATK_DNESP32S3_XL9555_LCD_BACKLIGHT, enabled ? 1 : 0);
}

/* BMGR owns the bus reference and driver deletion. */
__attribute__((weak)) esp_err_t io_expander_factory_entry_t(i2c_master_bus_handle_t bus, uint16_t address,
    esp_io_expander_handle_t *out_handle)
{
    if (out_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_handle = NULL;
    xl9555_t *chip = calloc(1, sizeof(*chip));
    if (chip == NULL) {
        return ESP_ERR_NO_MEM;
    }
    const i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = 400000,
    };
    esp_err_t ret = i2c_master_bus_add_device(bus, &device_config, &chip->device);
    if (ret != ESP_OK) {
        free(chip);
        return ret;
    }
    chip->base.config.io_count = 16;
    chip->base.config.flags.dir_out_bit_zero = 1;
    chip->base.read_input_reg = xl9555_read_input;
    chip->base.write_output_reg = xl9555_write_output;
    chip->base.read_output_reg = xl9555_read_output;
    chip->base.write_direction_reg = xl9555_write_direction;
    chip->base.read_direction_reg = xl9555_read_direction;
    chip->base.reset = xl9555_reset;
    chip->base.del = xl9555_delete;
    esp_io_expander_handle_t handle = &chip->base;
    ESP_GOTO_ON_ERROR(xl9555_reset(handle), cleanup, TAG, "Reset XL9555 registers failed");

    /* The driver's reset leaves all output latches high. Switch directions
     * before set_level(), which rejects input pins. PA and buzzer stay off. */
    ESP_GOTO_ON_ERROR(esp_io_expander_set_dir(handle, ATK_DNESP32S3_XL9555_OUTPUT_MASK, IO_EXPANDER_OUTPUT),
        cleanup, TAG, "Set output directions failed");
    ESP_GOTO_ON_ERROR(esp_io_expander_set_dir(handle, ATK_DNESP32S3_XL9555_DIRECTION_MASK, IO_EXPANDER_INPUT),
        cleanup, TAG, "Set input directions failed");
    ESP_GOTO_ON_ERROR(esp_io_expander_set_level(handle, ATK_DNESP32S3_XL9555_OUTPUT_MASK, 1),
        cleanup, TAG, "Set initial output levels failed");
    ESP_GOTO_ON_ERROR(atk_dnesp32s3_xl9555_set_level(handle, ATK_DNESP32S3_XL9555_CAMERA_PWDN, 0),
        cleanup, TAG, "Clear camera power-down failed");

    ESP_GOTO_ON_ERROR(atk_dnesp32s3_xl9555_set_level(handle, ATK_DNESP32S3_XL9555_LCD_RESET, 0),
        cleanup, TAG, "Assert LCD reset failed");
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_GOTO_ON_ERROR(atk_dnesp32s3_xl9555_set_level(handle, ATK_DNESP32S3_XL9555_LCD_RESET, 1),
        cleanup, TAG, "Release LCD reset failed");
    ESP_GOTO_ON_ERROR(atk_dnesp32s3_xl9555_set_level(handle, ATK_DNESP32S3_XL9555_CAMERA_RESET, 0),
        cleanup, TAG, "Assert camera reset failed");
    vTaskDelay(pdMS_TO_TICKS(20));
    ESP_GOTO_ON_ERROR(atk_dnesp32s3_xl9555_set_level(handle, ATK_DNESP32S3_XL9555_CAMERA_RESET, 1),
        cleanup, TAG, "Release camera reset failed");
    vTaskDelay(pdMS_TO_TICKS(20));

    *out_handle = handle;
    return ESP_OK;

cleanup:
{
    esp_err_t delete_ret = esp_io_expander_del(handle);
    if (delete_ret != ESP_OK) {
        ESP_LOGE(TAG, "Delete XL9555 after factory failure: %s", esp_err_to_name(delete_ret));
    }
}
    return ret;
}

/* The console passes BMGR's pointer-to-handle, as for other gpio_expander devices. */
static esp_err_t xl9555_io_get(void *device_handle, int pin, int *level)
{
    esp_io_expander_handle_t *device = device_handle;
    if (device == NULL || pin < 0 || pin > 15 || level == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t value = 0;
    esp_err_t ret = atk_dnesp32s3_xl9555_get_level(*device, (atk_dnesp32s3_xl9555_pin_t)pin, &value);
    if (ret == ESP_OK) {
        *level = value;
    }
    return ret;
}

static esp_err_t xl9555_io_set(void *device_handle, int pin, int level)
{
    esp_io_expander_handle_t *device = device_handle;
    if (device == NULL || pin < 0 || pin > 15 || level < 0 || level > 1) {
        return ESP_ERR_INVALID_ARG;
    }
    return atk_dnesp32s3_xl9555_set_level(*device, (atk_dnesp32s3_xl9555_pin_t)pin, (uint8_t)level);
}

DEVICE_EXTRA_FUNC_REGISTER(xl9555_io_get, xl9555_io_get);
DEVICE_EXTRA_FUNC_REGISTER(xl9555_io_set, xl9555_io_set);

#endif /* __has_include("esp_io_expander.h") */
