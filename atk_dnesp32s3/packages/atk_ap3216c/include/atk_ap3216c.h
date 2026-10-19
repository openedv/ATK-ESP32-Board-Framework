/*
 * SPDX-FileCopyrightText: 2026 ALIENTEK
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief  AP3216C driver instance. The caller owns the parent I2C bus. */
typedef struct atk_ap3216c *atk_ap3216c_handle_t;

/** @brief  Raw measurements; proximity is not a distance measurement. */
typedef struct {
    uint16_t  als_raw;     /*!< 16-bit ambient light value, not lux */
    uint16_t  ir_raw;      /*!< 10-bit infrared intensity; check ir_valid */
    uint16_t  ps_raw;      /*!< 10-bit proximity value; check ps_valid */
    bool  ir_valid;        /*!< False when the IR overflow flag is set */
    bool  ps_valid;        /*!< False when the PS IR-overflow flag is set */
    bool  object_detected; /*!< PS object-detection status */
} atk_ap3216c_sample_t;

/**
 * @brief  Attach a sensor to an existing bus without changing its registers.
 *
 * @param[in]   bus           Shared I2C master bus.
 * @param[in]   address       Seven-bit sensor address (normally 0x1e).
 * @param[in]   frequency_hz  I2C clock frequency in Hz.
 * @param[out]  out_handle    Created instance; NULL on failure.
 *
 * @return
 *      - ESP_OK               Instance created.
 *      - ESP_ERR_INVALID_ARG  Invalid arguments.
 *      - ESP_ERR_NO_MEM       Allocation failed.
 *      - Other                I2C driver error.
 */
esp_err_t atk_ap3216c_create(i2c_master_bus_handle_t bus, uint8_t address,
    uint32_t frequency_hz, atk_ap3216c_handle_t *out_handle);

/**
 * @brief  Reset, enable continuous ALS/PS/IR and wait for the first conversion.
 *
 * @param[in]  handle  Sensor instance. Serialize access with read/stop/delete.
 *
 * @return  ESP_OK on success, ESP_ERR_INVALID_ARG for NULL, or a register/I2C error.
 */
esp_err_t atk_ap3216c_start(atk_ap3216c_handle_t handle);

/**
 * @brief  Read the latest sample, preserving the output on transfer failure.
 *
 * @note  ALS+PS+IR conversion takes 112.5 ms. Poll at intervals of at least 120 ms.
 *        Overflow is reported through validity flags rather than a bus error.
 *
 * @param[in]   handle  Sensor instance. Serialize calls with other driver APIs.
 * @param[out]  sample  Raw values and validity flags.
 *
 * @return  ESP_OK on success, ESP_ERR_INVALID_ARG for NULL, or an I2C error.
 */
esp_err_t atk_ap3216c_read(atk_ap3216c_handle_t handle, atk_ap3216c_sample_t *sample);

/**
 * @brief  Put the sensor in power-down mode without detaching it.
 *
 * @param[in]  handle  Sensor instance.
 *
 * @return  ESP_OK on success, ESP_ERR_INVALID_ARG for NULL, or an I2C error.
 */
esp_err_t atk_ap3216c_stop(atk_ap3216c_handle_t handle);

/**
 * @brief  Detach and free the instance without deleting its shared parent bus.
 *
 * @note  Call stop first when power-down is required. On error the handle remains
 *        valid for retry; on success it must no longer be used.
 *
 * @param[in]  handle  Sensor instance.
 *
 * @return  ESP_OK on success, ESP_ERR_INVALID_ARG for NULL, or an I2C error.
 */
esp_err_t atk_ap3216c_delete(atk_ap3216c_handle_t handle);

#ifdef __cplusplus
}
#endif
