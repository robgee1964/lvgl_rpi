/**
 * @file touch.h
 *
 */

#ifndef TOUCH_H
#define TOUCH_H


#include "lv_drv_conf.h"
#include <stdbool.h>
#include <stdint.h>
#include "../../lvgl/lvgl.h"
#include "evdev.h"

#define TOUCH_INIT_OK   0
#define TOUCH_CAL_REQ   1
#define TOUCH_DRV_FAIL  2


int32_t touchInit(void);

bool touchRead(lv_indev_drv_t * drv, lv_indev_data_t * data);

bool touchReadRaw(lv_indev_drv_t * drv, lv_indev_data_t * data);

bool touchDoCalibration(lv_point_t* pPoints, uint16_t ofst);

#endif //TOUCH_H
