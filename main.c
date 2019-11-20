#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/touch.h"
#include "lv_examples/lv_apps/demo/demo.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <lv_application/lv_application.h>
#include "lv_examples/lv_apps/tpcal/tpcal.h"
#include <lv_examples/lv_tutorial/4_themes/lv_tutorial_themes.h>
#include <lv_examples/lv_tutorial/6_images/lv_tutorial_images.h>
#include <stdlib.h>
#include <stdio.h>
#include "buzzer.h"

#define DISP_BUF_SIZE (80*LV_HOR_RES_MAX)

void lv_ticker(void);
void feedback_cb(struct _lv_indev_drv_t *, uint8_t);

extern void app_tick(void);


static enum {APP_DEMO, APP_TP_CAL} eAppState;
lv_indev_drv_t  indev_drv;              // input device driver
lv_indev_t *  indev;
pthread_t lv_tick_thread;

volatile bool bTick = false;


int main(void)
{
   int tick_count = 0;
   int seconds = 0;
   printf("LVGL demo\r\n");
    /*LittlevGL init*/
    lv_init();

    /*Linux frame buffer device init*/
    fbdev_init();

//    buzzerInit();

#if 1

    /* Touchscreen driver device init */
    int32_t drvStatus = touchInit();
    if(drvStatus == TOUCH_DRV_FAIL)
    {
       perror("Cannot initialise touch screen driver\r\n");
       exit(1);
    }
#endif

    /*A small buffer for LittlevGL to draw the screen's content*/
    static lv_color_t buf[DISP_BUF_SIZE];

    /*Initialize a descriptor for the buffer*/
    static lv_disp_buf_t disp_buf;
    lv_disp_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    /*Initialize and register a display driver*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.buffer = &disp_buf;
    disp_drv.flush_cb = fbdev_flush;
    lv_disp_drv_register(&disp_drv);

#if 1
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchRead;
//    indev_drv.feedback_cb = feedback_cb;
    //indev_drv.user_data = (void*)&touchCalFunc;
    indev = lv_indev_drv_register(&indev_drv);
#endif
    int s;

    // Start up thread for lv tick
    if((s = pthread_create(&lv_tick_thread, NULL, (void*)&lv_ticker, NULL)) != 0)
    {
       printf("Cannot create tick thread:%d\r\n", s);
    }
    else
    {
       printf("tick thread created\r\n");
    }


    lv_application();
//    lv_tutorial_themes();
//    lv_tutorial_image();


#if EVDEV_CALIBRATE
    if(drvStatus == TOUCH_CAL_REQ)
       tpcal_create(indev);
#endif


    /*Handle LitlevGL tasks (tickless mode)*/
    while(1) {
        lv_task_handler();
        if(bTick)
        {
           tick_count += 5;
           if(tick_count >= 1000)
           {
              tick_count = 0;
//              printf("%d, lv_tick:%d\r\n", ++seconds, lv_tick_get());
           }
           bTick = false;
        }
#if LV_USE_APPLICATION
        app_tick();
#endif
        usleep(2000);
    }

    return 0;
}

/* measurents have shown that usleep(5000) actually delays for more like 10000 */
void lv_ticker(void)
{
   while(1)
   {
      lv_tick_inc(5);
      bTick = true;
      usleep(5000);
   }
}

void feedback_cb(struct _lv_indev_drv_t * p_drv, uint8_t event)
{
   if(event == LV_EVENT_CLICKED)
   {
      doBuzz(1000);
   }
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}
