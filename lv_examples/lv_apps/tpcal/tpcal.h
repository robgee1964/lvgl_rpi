/**
 * @file tpcal.h
 *
 */

#ifndef TPCAL_H
#define TPCAL_H

#ifdef __cplusplus
extern "C" {
#endif

   /*********************
    *      INCLUDES
    *********************/
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lvgl.h"
#include "lv_ex_conf.h"
#else
#include "../../../lvgl/lvgl.h"
#include "../../../lv_ex_conf.h"
#endif

/**
 * Create a touch pad calibration screen
 */

void tpcal_create(lv_indev_t * indev);

#if LV_USE_DEMO

   /*********************
    *      DEFINES
    *********************/

   /**********************
    *      TYPEDEFS
    **********************/

   /**********************
    * GLOBAL PROTOTYPES
    **********************/


   /**********************
    *      MACROS
    **********************/

#endif /*LV_USE_TPCAL*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*TP_CAL_H*/
