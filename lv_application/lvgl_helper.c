/*
 * lvgl_helper.c
 *
 *  Created on: 20 Nov 2019
 *      Author: rgee
 */


#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lvgl.h"
#include "lv_app_conf.h"
#else
#include "../lvgl/lvgl.h"
#include "../lv_app_conf.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "lvgl_helper.h"

static lv_obj_t * mbox;

static void mbox_event_cb(lv_obj_t* obj, lv_event_t evt);


void lvh_mbox_create_modal(lv_obj_t * parent, const lv_obj_t * copy, const char* pMsg, const char** ppButtons)
{
   static lv_style_t modal_style;
   /* Create a full-screen background */
   lv_style_copy(&modal_style, &lv_style_plain_color);

   /* Set the background's style */
   modal_style.body.main_color = modal_style.body.grad_color = LV_COLOR_BLACK;
   modal_style.body.opa = LV_OPA_50;

   /* Create a base object for the modal background */
   lv_obj_t *obj = lv_obj_create(parent, NULL);
   lv_obj_set_style(obj, &modal_style);
   lv_obj_set_pos(obj, 0, 0);
   lv_obj_set_size(obj, LV_HOR_RES, LV_VER_RES);
   lv_obj_set_opa_scale_enable(obj, true); /* Enable opacity scaling for the animation */

   /* Create the message box as a child of the modal background */
   mbox = lv_mbox_create(obj, copy);
   lv_mbox_add_btns(mbox, ppButtons);
   lv_mbox_set_text(mbox, pMsg);
   lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
   lv_obj_set_event_cb(mbox, mbox_event_cb);

   /* Fade the message box in with an animation */
   lv_anim_t a;
   lv_anim_init(&a);
   lv_anim_set_time(&a, 500, 0);
   lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
   lv_anim_set_exec_cb(&a, obj, (lv_anim_exec_xcb_t)lv_obj_set_opa_scale);
   lv_anim_create(&a);
}

/**
 * @brief locates string in ddlist items
 */
int32_t lvh_ddlist_set_selected_str(lv_obj_t * ddlist, const char* item)
{
   char* opt_str = malloc(strlen(lv_ddlist_get_options(ddlist))+1);
   strcpy(opt_str, lv_ddlist_get_options(ddlist));
   const char delim[] = "\n";
   const char* ptok;
   int32_t index = 0;
   int32_t status = -1;

   ptok = strtok((char*)opt_str, delim);
   while((ptok != NULL) && strcmp(ptok, item))
   {
      ptok = strtok(NULL, delim);
      index++;
   }
   if(ptok != NULL)
   {
      lv_ddlist_set_selected(ddlist, index);
      status =  0;
   }
   free(opt_str);
   return status;
}



static void mbox_event_cb(lv_obj_t* obj, lv_event_t evt)
{
   if(evt == LV_EVENT_DELETE && obj == mbox)
   {
      /* Delete the parent modal background */
      lv_obj_del_async(lv_obj_get_parent(mbox));
      mbox = NULL; /* happens before object is actually deleted! */
   }
   else if(evt == LV_EVENT_VALUE_CHANGED)
   {
      /* A button was clicked */
      lv_mbox_start_auto_close(mbox, 0);
   }
}
