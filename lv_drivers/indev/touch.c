/*
 * touch.c
 *
 *  Created on: 29 Sep 2019
 *      Author: Rob
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../lvgl/lvgl.h"
// TODO change the following #include so that we can work with any touchscreen device
#include "evdev.h"
#include "touch.h"


/*********************
 *      DEFINES
 *********************/
#define SAMPLE_POINTS      4

#define FILT_SHIFT         2


#define TOUCH_CAL_FILE     "touchcal.dat"


// Default calibration points
#define TOUCHCAL_ULX       149
#define TOUCHCAL_ULY       825
#define TOUCHCAL_URX       898
#define TOUCHCAL_URY       852
#define TOUCHCAL_LRX       898
#define TOUCHCAL_LRY       210
#define TOUCHCAL_LLX       144
#define TOUCHCAL_LLY       193
#define TOUCHCAL_DEF_OFST  30

/**********************
 *      TYPEDEFS
 **********************/
typedef struct __attribute__ ((packed))
{
   lv_point_t  points[SAMPLE_POINTS];
   uint16_t        scn_ofst;      // location of calibration circles from corner of screen
   uint16_t        crc;
} t_Tpcal;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void touchCalculateCalpoints(t_Tpcal* pCal);
static bool  touchStoreCalibration(t_Tpcal* pCal);
static bool  touchCheckForCalibration(void);
static bool  touchLoadCalibration(void);


/**********************
 *  STATIC VARIABLES
 **********************/

static int32_t ymt, xmt;
static int32_t xmd, ymd;
static int32_t xc, yc;

static const t_Tpcal defCal = {
{
   {TOUCHCAL_ULX, TOUCHCAL_ULY},
   {TOUCHCAL_URX, TOUCHCAL_URY},
   {TOUCHCAL_LRX, TOUCHCAL_LRY},
   {TOUCHCAL_LLX, TOUCHCAL_LLY},
}, TOUCHCAL_DEF_OFST, 0};

static t_Tpcal tpCal = {0};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int32_t touchInit(void)
{
   bool calRequired = false;

   if(!evdev_init())
      return TOUCH_DRV_FAIL;

#if EVDEV_CALIBRATE

   if(!touchLoadCalibration())
   {
      calRequired = true;
   }
   else if(touchCheckForCalibration())
   {
      calRequired = true;
   }

   if(calRequired)
      return TOUCH_CAL_REQ;
   else
      return TOUCH_INIT_OK;
#else
   return TOUCH_INIT_OK;
#endif

}


bool touchRead(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
   /* This is called every LV_INDEV_DEF_READ_PERIOD milliseconds */
   static lv_point_t pt_prev = {0, 0};
   bool bStatus = evdev_read(drv, data);

#if EVDEV_CALIBRATE
   static lv_indev_state_t prev_state = LV_INDEV_STATE_REL;


   if(data->state == LV_INDEV_STATE_PR)
   {
   //   printf("raw x:%d. raw y: %d\n", data->point.x, data->point.y);
   }

   // scaling
   int32_t x = xc + (((int32_t)data->point.x * xmt)/xmd);

   int32_t y = yc + (((int32_t)data->point.y * ymt)/ymd);

   if(x < 0)
     x = 0;
   if(y < 0)
     y = 0;
   if(x >= lv_disp_get_hor_res(drv->disp))
     x = lv_disp_get_hor_res(drv->disp) - 1;
   if(y >= lv_disp_get_ver_res(drv->disp))
     y = lv_disp_get_ver_res(drv->disp) - 1;

#ifdef TOUCH_FILT

   int32_t x_filt, y_filt;

   if(data->state == LV_INDEV_STATE_PR)
   {
      if(prev_state == LV_INDEV_STATE_REL)
      {
         printf("Touch pressed\r\n");
         pt_prev.x = x;     // reset previous point to current x,y
         pt_prev.y = y;
      }
      /* do filter out = kx + (1-k)xprev, make k a power of 2 */
      x_filt = (x >> FILT_SHIFT) + (pt_prev.x - (pt_prev.x >> FILT_SHIFT));
      y_filt = (y >> FILT_SHIFT) + (pt_prev.y - (pt_prev.y >> FILT_SHIFT));
      pt_prev.x = x_filt;
      pt_prev.y = y_filt;

      printf("xraw:%d, xfilt:%d, yraw:%d, yfilt:%d\n", x, x_filt, y, y_filt);

   }
   else if(data->state == LV_INDEV_STATE_REL)
   {
      if(prev_state == LV_INDEV_STATE_PR)
      {
         printf("Touch released\r\n");
      }
   }
   prev_state = data->state;

   data->point.x = (lv_coord_t)x_filt;
   data->point.y = (lv_coord_t)y_filt;
#else
   data->point.x = x;
   data->point.y = y;
#endif

   /* Tests have shown there is quite a lot of noise on the touch screen analog channels, especially when dragging
    * therefore, when state is LV_INDEV_STATE_PR, implement a filter
    */
#endif
   return bStatus;
}


bool touchReadRaw(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
   bool bStatus = evdev_read(drv, data);

   return bStatus;
}



bool touchDoCalibration(lv_point_t* pPoints, uint16_t ofst)
{
   bool bSuccess = false;
   t_Tpcal cal;

   for(uint32_t i = 0; i < SAMPLE_POINTS; i++)
   {
      printf("x: %d, y: %d\r\n", pPoints[i].x, pPoints[i].y);
   }

   memcpy(cal.points, pPoints, SAMPLE_POINTS*sizeof(lv_point_t));
   cal.scn_ofst = ofst;

   printf("ofst:%d\n", ofst);
   printf("cal.scn_ofst:%d\n", cal.scn_ofst);

   touchCalculateCalpoints(&cal);

   if(touchStoreCalibration(&cal))
      bSuccess = true;

   return bSuccess;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/


static void touchCalculateCalpoints(t_Tpcal* pCal)  //lv_point_t* pPoints, WORD ofst)
{
   // ofst is the location of the calibation circle from screen edge
   int32_t trA, trB, trC, trD;                    // variables for the coefficients
   int32_t trAhold, trBhold, trChold, trDhold;
   int32_t test1, test2;                          // temp variables (must be signed type)

   printf("scn_ofst:%d\n", pCal->scn_ofst);


   lv_point_t scrPoints[SAMPLE_POINTS] =
   {  {pCal->scn_ofst, pCal->scn_ofst},                             // Top left
      {LV_HOR_RES_MAX - 1 - pCal->scn_ofst, pCal->scn_ofst},       // Top right
      {LV_HOR_RES_MAX - 1 - pCal->scn_ofst, LV_VER_RES_MAX  - 1 - pCal->scn_ofst},  // Bottom right
      {pCal->scn_ofst, LV_VER_RES_MAX  - 1 - pCal->scn_ofst},       // bottom left
   };

   // xslope1
   int32_t xmd_1 = pCal->points[1].x - pCal->points[0].x;
   int32_t xmd_2 = pCal->points[2].x - pCal->points[3].x;

   printf("xmd1:%d, xmd2:%d\n", xmd_1, xmd_2);

   xmd = (xmd_1 + xmd_2)/2;
   xmt = LV_HOR_RES_MAX - (2 * pCal->scn_ofst);

   // yslope
   int32_t ymd_1 = pCal->points[3].y - pCal->points[0].y;
   int32_t ymd_2 = pCal->points[2].y - pCal->points[1].y;

   printf("ymd1:%d, ymd2:%d\n", ymd_1, ymd_2);

   ymd = (ymd_1 + ymd_2)/2;
   ymt = LV_VER_RES_MAX - (2 * pCal->scn_ofst);

   // calculate transfer function for x upper
   // x = (xmt * raw)/xmd + xc
   // rearranging we get
   int32_t xc1 = LV_HOR_RES_MAX;
   xc1 -= ((xmt * (pCal->points[0].x + pCal->points[1].x))/xmd_1);
   xc1 /= 2;

   // Now do x-lower
   int32_t xc2 = LV_HOR_RES_MAX;
   xc2 -= ((xmt * (pCal->points[2].x + pCal->points[3].x))/xmd_2);
   xc2 /= 2;

   printf("xc1:%d, xc2:%d\n", xc1, xc2);

   // y right
   int32_t yc1 = LV_VER_RES_MAX;
   yc1 -= ((ymt * (pCal->points[3].y + pCal->points[0].y))/ymd_1);
   yc1 /= 2;

   // y left
   int32_t yc2 = LV_VER_RES_MAX;
   yc2 -= ((ymt * (pCal->points[2].y + pCal->points[1].y))/ymd_2);
   yc2 /= 2;

   printf("yc1:%d, yc2:%d\n", yc1, yc2);

   // take averages
   yc = (yc1 + yc2)/2;
   xc = (xc1 + xc2)/2;

   printf("xmt:%d ymt:%d, xmd: %d, ymd:%d, xc:%d, yc:%d\n", xmt, ymt, xmd, ymd, xc, yc);

}

static bool  touchStoreCalibration(t_Tpcal* pCal)
{
   FILE* fCal;

   if((fCal = fopen(TOUCH_CAL_FILE, "w")) == NULL)
   {
      return false;
   }

   // Write out calibration readings
   for(uint32_t i = 0; i < SAMPLE_POINTS; i++)
   {
      fprintf(fCal, "%d %d\n", pCal->points[i].x, pCal->points[i].y);
   }
   // Write out the screen offset location for cal point
   fprintf(fCal, "%d\r\n", pCal->scn_ofst);

   fclose(fCal);

   // Copy into active calibration points
   memcpy(tpCal.points, pCal->points, sizeof(tpCal.points));
   tpCal.scn_ofst = pCal->scn_ofst;

   return true;
}

static bool  touchCheckForCalibration(void)
{
   return false;
}

static bool  touchLoadCalibration(void)
{
   FILE* fCal;
   int16_t x,y,ofst;
   char line[80];
   char* tok;
   const char delim[] = "\r\n ";

   if((fCal = fopen(TOUCH_CAL_FILE, "r")) == NULL)
   {
      printf("Loading default calibration\r\n");
      memcpy(&tpCal, &defCal, sizeof(defCal));
      return false;
   }
   else
   {
      for(uint32_t i = 0; i < SAMPLE_POINTS; i++)
      {
         fgets(line, 80, fCal);
         tok = strtok(line, delim);
         x = atoi(tok);
         tok = strtok(NULL, delim);
         y = atoi(tok);
         tpCal.points[i].x = (lv_coord_t)x;
         tpCal.points[i].y = (lv_coord_t)y;
      }
      fgets(line, 80, fCal);
      tok = strtok(line, delim);
      ofst = atoi(tok);
      tpCal.scn_ofst = ofst;

      for(uint32_t i = 0; i < SAMPLE_POINTS; i++)
      {
         printf("%d, %d\n", tpCal.points[i].x, tpCal.points[i].y);
      }

      touchCalculateCalpoints(&tpCal);

      fclose(fCal);

      return true;
   }


}

