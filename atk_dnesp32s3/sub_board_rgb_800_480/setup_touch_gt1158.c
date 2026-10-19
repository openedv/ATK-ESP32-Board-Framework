/*
 * SPDX-FileCopyrightText: 2026 ALIENTEK
 * SPDX-License-Identifier: Apache-2.0
 */

#if __has_include("esp_lcd_touch_gt1151.h")
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_lcd_touch_gt1151.h"
#include "atk_dnesp32s3_xl9555.h"

/**
 * @brief  Create the GT1158 touch controller using the GT115x driver.
 *
 * @param[in]   io                Board Manager-owned I2C panel IO.
 * @param[in]   touch_dev_config  Touch geometry and reset/interrupt pins.
 * @param[out]  ret_touch         Created touch handle, owned by Board Manager.
 *
 * @note  Reset is held low and released for 200 ms each. XL9555 owns the pin
 *        for the lifetime of the board; the touch driver does not reset it.
 *
 * @return
 *      - ESP_OK                 Controller created.
 *      - ESP_ERR_INVALID_ARG    Invalid configuration.
 *      - ESP_ERR_INVALID_STATE  XL9555 is unavailable.
 *      - Others                 Error returned by XL9555 or the touch driver.
 */
__attribute__((weak)) esp_err_t lcd_touch_factory_entry_t(
    esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *touch_dev_config,
    esp_lcd_touch_handle_t *ret_touch)
{
    if (io == NULL || touch_dev_config == NULL || ret_touch == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *ret_touch = NULL;
    atk_dnesp32s3_xl9555_handle_t *expander = NULL;
    esp_err_t ret = atk_dnesp32s3_xl9555_get(&expander);
    if (ret != ESP_OK) {
        return ret;
    }
    esp_lcd_touch_config_t config = *touch_dev_config;
    ret = atk_dnesp32s3_xl9555_set_level(expander, ATK_DNESP32S3_XL9555_TOUCH_RESET,
        config.levels.reset);
    if (ret != ESP_OK) {
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(200));
    ret = atk_dnesp32s3_xl9555_set_level(expander, ATK_DNESP32S3_XL9555_TOUCH_RESET,
        !config.levels.reset);
    if (ret != ESP_OK) {
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(200));
    /* This module needs longer reset/startup delays than the driver's 10 ms. */
    config.rst_gpio_num = GPIO_NUM_NC;
    return esp_lcd_touch_new_i2c_gt1151(io, &config, ret_touch);
}
#endif
