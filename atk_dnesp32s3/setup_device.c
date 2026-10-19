/*
 * SPDX-FileCopyrightText: 2026 ALIENTEK
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include "sdkconfig.h"
#if CONFIG_ESP_BOARD_DEV_GPIO_EXPANDER_SUPPORT && CONFIG_ESP_BOARD_DEV_BUTTON_SUB_CUSTOM_SUPPORT
#include "button_interface.h"
#include "dev_button.h"
#endif
#include "esp_board_device.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"

#include "atk_dnesp32s3_xl9555.h"

#if __has_include(<esp_lcd_panel_vendor.h>)
#define ATK_DNESP32S3_HAS_ST7789 \
    1
#include "esp_lcd_panel_vendor.h"
#endif

#if defined(ATK_DNESP32S3_HAS_ST7789)
__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(
    esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config,
    esp_lcd_panel_handle_t *ret_panel)
{
    return esp_lcd_new_panel_st7789(io, panel_dev_config, ret_panel);
}
#endif

#if CONFIG_ESP_BOARD_DEV_GPIO_EXPANDER_SUPPORT && CONFIG_ESP_BOARD_DEV_BUTTON_SUB_CUSTOM_SUPPORT
typedef struct {
    button_driver_t  base;
    atk_dnesp32s3_xl9555_handle_t *xl9555;
    atk_dnesp32s3_xl9555_pin_t  pin;
} atk_dnesp32s3_button_driver_t;

static const char *TAG = "ATK_DNESP32S3_SETUP";

static uint8_t xl9555_button_get_level(button_driver_t *driver)
{
    atk_dnesp32s3_button_driver_t *button = (atk_dnesp32s3_button_driver_t *)driver;
    uint8_t level = 1;
    if (button == NULL || button->xl9555 == NULL ||
        atk_dnesp32s3_xl9555_get_level(button->xl9555, button->pin, &level) != ESP_OK) {
        return 0;
    }
    return level == 0;
}

static esp_err_t xl9555_button_del(button_driver_t *driver)
{
    free(driver);
    return ESP_OK;
}

static button_driver_t *create_xl9555_button(atk_dnesp32s3_xl9555_pin_t pin)
{
    atk_dnesp32s3_button_driver_t *button = calloc(1, sizeof(*button));
    if (button == NULL) {
        return NULL;
    }
    if (atk_dnesp32s3_xl9555_get(&button->xl9555) != ESP_OK) {
        ESP_LOGE(TAG, "failed to get XL9555 device handle");
        free(button);
        return NULL;
    }
    button->pin = pin;
    button->base.get_key_level = xl9555_button_get_level;
    button->base.del = xl9555_button_del;
    button->base.enable_power_save = false;
    return &button->base;
}

static button_driver_t *key0_button_create(const dev_button_config_t *config)
{
    (void)config;
    return create_xl9555_button(ATK_DNESP32S3_XL9555_KEY0);
}

static button_driver_t *key1_button_create(const dev_button_config_t *config)
{
    (void)config;
    return create_xl9555_button(ATK_DNESP32S3_XL9555_KEY1);
}

static button_driver_t *key2_button_create(const dev_button_config_t *config)
{
    (void)config;
    return create_xl9555_button(ATK_DNESP32S3_XL9555_KEY2);
}

static button_driver_t *key3_button_create(const dev_button_config_t *config)
{
    (void)config;
    return create_xl9555_button(ATK_DNESP32S3_XL9555_KEY3);
}

DEVICE_EXTRA_FUNC_REGISTER(key0_button, key0_button_create);
DEVICE_EXTRA_FUNC_REGISTER(key1_button, key1_button_create);
DEVICE_EXTRA_FUNC_REGISTER(key2_button, key2_button_create);
DEVICE_EXTRA_FUNC_REGISTER(key3_button, key3_button_create);

#endif /* GPIO expander custom buttons */
