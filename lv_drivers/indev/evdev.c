/**
 * @file evdev.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "evdev.h"
#if USE_EVDEV != 0

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

/*********************
 *      DEFINES
 *********************/
//#define EV_DEBUG

/**********************
 *      TYPEDEFS
 **********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/
int map(int x, int in_min, int in_max, int out_min, int out_max);

/**********************
 *  STATIC VARIABLES
 **********************/
int evdev_fd;
int evdev_root_x;
int evdev_root_y;
int evdev_button;

int evdev_key_val;



/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Initialize the evdev interface
 */
bool evdev_init(void)
{
    evdev_fd = open(EVDEV_NAME, O_RDWR | O_NOCTTY | O_NDELAY);
    char name[256] = "Unknown";

    if(evdev_fd == -1) {
        perror("unable open evdev interface:");
        return false;
    }

    fcntl(evdev_fd, F_SETFL, O_ASYNC | O_NONBLOCK);

    /* Print Device Name */
    ioctl(evdev_fd, EVIOCGNAME(sizeof(name)), name);
#if defined EV_DEBUG && defined EV_ALL
    printf("Reading from:\n");
    printf("device file = %s\n", EVDEV_NAME);
    printf("device name = %s\n", name);
#endif


    evdev_root_x = 0;
    evdev_root_y = 0;
    evdev_key_val = 0;
    evdev_button = LV_INDEV_STATE_REL;

    return true;
}


/**
 * reconfigure the device file for evdev
 * @param dev_name set the evdev device filename
 * @return true: the device file set complete
 *         false: the device file doesn't exist current system
 */
bool evdev_set_file(char* dev_name)
{ 
     if(evdev_fd != -1) {
        close(evdev_fd);
     }
     evdev_fd = open(dev_name, O_RDWR | O_NOCTTY | O_NDELAY);

     if(evdev_fd == -1) {
        perror("unable open evdev interface:");
        return false;
     }

     fcntl(evdev_fd, F_SETFL, O_ASYNC | O_NONBLOCK);

     evdev_root_x = 0;
     evdev_root_y = 0;
     evdev_key_val = 0;
     evdev_button = LV_INDEV_STATE_REL;

     return true;
}
/**
 * Get the current position and state of the evdev
 * @param data store the evdev data here
 * @return false: because the points are not buffered, so no more data to be read
 */
bool evdev_read(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
   struct input_event in;

   evdev_button = LV_INDEV_STATE_REL;

   while(read(evdev_fd, &in, sizeof(struct input_event)) > 0) {
#if defined EV_DEBUG && defined EV_ALL
      printf("type: %d, code: %d\r\n", in.type, in.code);
#endif
      if(in.type == EV_REL) {
         if(in.code == REL_X) {
#if EVDEV_SWAP_AXES
            evdev_root_y += in.value;
#else
         evdev_root_x += in.value;
#endif
         evdev_button = LV_INDEV_STATE_PR;
         }
         else if(in.code == REL_Y) {
#if EVDEV_SWAP_AXES
            evdev_root_x += in.value;
#else
         evdev_root_y += in.value;
#endif
         evdev_button = LV_INDEV_STATE_PR;
         }
      }
      else if(in.type == EV_ABS) {
         if(in.code == ABS_X) {
#if EVDEV_SWAP_AXES
            evdev_root_y = in.value;
#else
         evdev_root_x = in.value;
#endif
         evdev_button = LV_INDEV_STATE_PR;
         }
         else if(in.code == ABS_Y) {
#if EVDEV_SWAP_AXES
            evdev_root_x = in.value;
#else
         evdev_root_y = in.value;
#endif
         evdev_button = LV_INDEV_STATE_PR;
         }
         else if(in.code == ABS_MT_POSITION_X)
#if EVDEV_SWAP_AXES
            evdev_root_y = in.value;
#else
         evdev_root_x = in.value;
#endif
         else if(in.code == ABS_MT_POSITION_Y)
#if EVDEV_SWAP_AXES
            evdev_root_x = in.value;
#else
         evdev_root_y = in.value;
#endif
      } else if(in.type == EV_KEY) {
         if(in.code == BTN_MOUSE || in.code == BTN_TOUCH) {
            if(in.value == 0)
               evdev_button = LV_INDEV_STATE_REL;
            else if(in.value == 1)
               evdev_button = LV_INDEV_STATE_PR;
         } else if(drv->type == LV_INDEV_TYPE_KEYPAD) {
            data->state = (in.value) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
            switch(in.code) {
            case KEY_BACKSPACE:
               data->key = LV_KEY_BACKSPACE;
               break;
            case KEY_ENTER:
               data->key = LV_KEY_ENTER;
               break;
            case KEY_UP:
               data->key = LV_KEY_UP;
               break;
            case KEY_LEFT:
               data->key = LV_KEY_PREV;
               break;
            case KEY_RIGHT:
               data->key = LV_KEY_NEXT;
               break;
            case KEY_DOWN:
               data->key = LV_KEY_DOWN;
               break;
            default:
               data->key = 0;
               break;
            }
            evdev_key_val = data->key;
            evdev_button = data->state;
            return false;
         }
      }
   }

   if(drv->type == LV_INDEV_TYPE_KEYPAD) {
      /* No data retrieved */
      data->key = evdev_key_val;
      data->state = evdev_button;
      return false;
   }
   if(drv->type != LV_INDEV_TYPE_POINTER)
      return false;

#ifdef EV_DEBUG
   if(evdev_button == LV_INDEV_STATE_PR)
   {
      //printf("Pressed\r\n");
      printf("x:%d, y:%d\n", evdev_root_x, evdev_root_y);
   }
#endif
   /*Store the collected data*/
   data->state = evdev_button;


   data->point.x = evdev_root_x;
   data->point.y = evdev_root_y;


   return false;

}

/**********************
 *   STATIC FUNCTIONS
 **********************/
int map(int x, int in_min, int in_max, int out_min, int out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}




#endif
