/**
 * @file lv_tutorial_objects.h
 *
 */

#ifndef LV_APPLICATION_H
#define LV_APPLICATION_H

#ifdef __cplusplus
extern "C" {
#endif

   /*********************
    *      INCLUDES
    *********************/
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lvgl.h"
#include "lv_app_conf.h"
#else
#include "../lvgl/lvgl.h"
#include "../lv_app_conf.h"
#endif

#if LV_USE_APPLICATION

   /*********************
    *      DEFINES
    *********************/

   /**********************
    *      TYPEDEFS
    **********************/

   /**********************
    * GLOBAL PROTOTYPES
    **********************/
   void lv_application(void);

   /**********************
    *      MACROS
    **********************/

#endif /*LV_USE_TUTORIALS*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_TUTORIAL_OBJECTS_H*/
