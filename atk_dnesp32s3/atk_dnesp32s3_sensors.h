/*
 * SPDX-FileCopyrightText: 2026 ALIENTEK
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#if __has_include("atk_ap3216c.h")
#include "atk_ap3216c.h"
#endif
#if __has_include("qma6100p.h")
#include "qma6100p.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if __has_include("atk_ap3216c.h")
/** @brief  BMGR handle returned for the custom device named ap3216c. */
typedef struct {
    atk_ap3216c_handle_t  sensor; /*!< Borrowed driver handle; do not delete */
    const char *peripheral_name;  /*!< Internal bus-reference bookkeeping */
} atk_dnesp32s3_ap3216c_handles_t;
#endif

#if __has_include("qma6100p.h")
/** @brief  BMGR handle returned for the custom device named qma6100p. */
typedef struct {
    qma6100p_handle_t  sensor;   /*!< Borrowed driver handle; do not delete */
    const char *peripheral_name; /*!< Internal bus-reference bookkeeping */
} atk_dnesp32s3_qma6100p_handles_t;
#endif

#ifdef __cplusplus
}
#endif
