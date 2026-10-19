# SPDX-FileCopyrightText: 2026 ALIENTEK
# SPDX-License-Identifier: Apache-2.0

idf_component_register(SRCS "atk_ap3216c.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_i2c)
