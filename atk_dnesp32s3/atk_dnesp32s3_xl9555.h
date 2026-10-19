/*
 * SPDX-FileCopyrightText: 2026 ALIENTEK
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct esp_io_expander_s atk_dnesp32s3_xl9555_handle_t;

typedef enum {
    ATK_DNESP32S3_XL9555_SPK_EN = 2,
    ATK_DNESP32S3_XL9555_BEEP = 3,
    ATK_DNESP32S3_XL9555_CAMERA_PWDN = 4,
    ATK_DNESP32S3_XL9555_CAMERA_RESET = 5,
    ATK_DNESP32S3_XL9555_LCD_BACKLIGHT = 8,
    ATK_DNESP32S3_XL9555_TOUCH_RESET = 9,
    ATK_DNESP32S3_XL9555_LCD_RESET = 10,
    ATK_DNESP32S3_XL9555_LCD_POWER = 11,
    ATK_DNESP32S3_XL9555_KEY3 = 12,
    ATK_DNESP32S3_XL9555_KEY2 = 13,
    ATK_DNESP32S3_XL9555_KEY1 = 14,
    ATK_DNESP32S3_XL9555_KEY0 = 15,
} atk_dnesp32s3_xl9555_pin_t;

/**
 * @brief  Get the initialized board XL9555 handle.
 *
 * @param[out]  out_handle  Underlying handle of the Board Manager GPIO expander device.
 *
 * @return
 *      - ESP_OK               Handle obtained.
 *      - ESP_ERR_INVALID_ARG  out_handle is NULL.
 *      - Others               Board Manager lookup failed.
 */
esp_err_t atk_dnesp32s3_xl9555_get(atk_dnesp32s3_xl9555_handle_t **out_handle);

/**
 * @brief  Read one XL9555 input or output pin.
 */
esp_err_t atk_dnesp32s3_xl9555_get_level(atk_dnesp32s3_xl9555_handle_t *handle, atk_dnesp32s3_xl9555_pin_t pin,
    uint8_t *level);

/**
 * @brief  Set one XL9555 output pin level.
 */
esp_err_t atk_dnesp32s3_xl9555_set_level(atk_dnesp32s3_xl9555_handle_t *handle, atk_dnesp32s3_xl9555_pin_t pin,
    uint8_t level);

/**
 * @brief  Enable or disable the low-active board buzzer.
 */
esp_err_t atk_dnesp32s3_xl9555_set_beep(atk_dnesp32s3_xl9555_handle_t *handle, bool enabled);

/**
 * @brief  Enable or disable the low-active speaker power amplifier.
 */
esp_err_t atk_dnesp32s3_xl9555_set_speaker(atk_dnesp32s3_xl9555_handle_t *handle, bool enabled);

/**
 * @brief  Enable or disable the LCD backlight.
 */
esp_err_t atk_dnesp32s3_xl9555_set_lcd_backlight(atk_dnesp32s3_xl9555_handle_t *handle, bool enabled);

#ifdef __cplusplus
}
#endif
