/**
 * @file lv_tutorial_objects.c
 *
 */

/*
 * ------------------------------------------------
 * Learn how to create GUI elements on the screen
 * ------------------------------------------------
 *
 * The basic building blocks (components or widgets) in LittlevGL are the graphical objects.
 * For example:
 *  - Buttons
 *  - Labels
 *  - Charts
 *  - Sliders etc
 *
 * In this part you can learn the basics of the objects like creating, positioning, sizing etc.
 * You will also meet some different object types and their attributes.
 *
 * Regardless to the object's type the 'lv_obj_t' variable type is used
 * and you can refer to an object with an lv_obj_t pointer (lv_obj_t *)
 *
 * PARENT-CHILD STRUCTURE
 * -------------------------
 * A parent can be considered as the container of its children.
 * Every object has exactly one parent object (except screens).
 * A parent can have unlimited number of children.
 * There is no limitation for the type of the parent.
 *
 * The children are visible only on their parent. The parts outside will be cropped (not displayed)
 *
 * If the parent is moved the children will be moved with it.
 *
 * The earlier created object (and its children) will drawn earlier.
 * Using this layers can be built.
 *
 * INHERITANCE
 * -------------
 * Similarly to object oriented languages some kind of inheritance is used
 * among the object types. Every object is derived from the 'Basic object'. (lv_obj)
 * The types are backward compatible therefore to set the basic parameters (size, position etc.)
 * you can use 'lv_obj_set/get_...()' function.

 * LEARN MORE
 * -------------
 * - General overview: http://www.gl.littlev.hu/objects
 * - Detailed description of types: http://www.gl.littlev.hu/object-types
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdlib.h>
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lvgl.h"
#include "lv_app_conf.h"
#else
#include "../lvgl/lvgl.h"
#include "../lv_app_conf.h"
#endif
#include "lvgl_helper.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "uart.h"
#include "fs_abs.h"
#include "uart.h"
#include "fontAwesomeExtra.h"
#if LV_USE_APPLICATION

LV_FONT_DECLARE(lv_font_roboto_28)
//LV_FONT_DECLARE(lv_font_roboto_22)
LV_FONT_DECLARE(fontAwesomeExtra)



/*********************
 *      DEFINES
 *********************/
#define BRIGHTNESS_FILE "/sys/class/backlight/rpi_backlight/brightness"
#define BRIGHTNESS_MIN   30
#define BRIGHTNESS_MAX   255
#define SCREEN_SLEEP	 30000

#define POWER_FILE "/sys/class/backlight/rpi_backlight/bl_power"
#define POWER_ON 	0
#define POWER_OFF 	1

#define NUM_SIDEBAR_BUTTONS	5

#define DEF_SERIAL_PORT		"/dev/ttyUSB0"
#define DEF_SERIAL_BAUD 	115200
#define DEF_STOPBITS       STOPBITS_1
#define DEF_PARITY         PARITY_NONE
#define DEF_DATABITS       8

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static bool openSerial(serial_t* pCtx);
static void createControlScreen(int32_t left, int32_t bottom);
static void createScopeScreen(int32_t left, int32_t bottom);
static void createSettingScreen(int32_t left, int32_t bottom);
static void btnSidebar_cb(lv_obj_t * btn, lv_event_t event);
static void btn1_event_cb(lv_obj_t * btn, lv_event_t event);
static void btn2_event_cb(lv_obj_t * btn, lv_event_t event);
static void btn_port_event_cb(lv_obj_t * btn, lv_event_t event);
static void btn_wake_cb(lv_obj_t * button, lv_event_t event);
static void ddlist_event_cb(lv_obj_t * ddlist, lv_event_t event);
static void ddl_port_event_cb(lv_obj_t * ddlist, lv_event_t event);
static void ddl_baud_event_cb(lv_obj_t * ddlist, lv_event_t event);
static void ddl_databits_event_cb(lv_obj_t * ddlist, lv_event_t event);
static void ddl_parity_event_cb(lv_obj_t * ddlist, lv_event_t event);
static void ddl_stopbits_event_cb(lv_obj_t * ddlist, lv_event_t event);
static void slider_event_cb(lv_obj_t * slider, lv_event_t event);
static void LCD_Off(void);
static void powerLCD(uint32_t power);
static void parseSerial(char* msg);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t * slider;
static lv_obj_t * btnWake;
static lv_obj_t * scr;
static lv_obj_t * contControl;
static lv_obj_t * contGraph;
static lv_obj_t * contSettings;
static lv_obj_t * btnSidebar[NUM_SIDEBAR_BUTTONS];
static lv_obj_t * lblMsg;
static lv_obj_t * lblStatus;

static lv_style_t titleStyle;
static lv_style_t lblOnBgStyle;
static lv_style_t titleStyle;

static lv_chart_series_t* dl1;

static char msg[30];
static bool bLCDcontrol = true;
static int16_t LCDlevel;
static int16_t LCDpower = POWER_ON;

static serial_t* pSerCtx;
static bool bSerialActive = false;
static ser_param_t ttyParams = {DEF_SERIAL_BAUD, DEF_STOPBITS, DEF_PARITY, DEF_DATABITS};
static char ttyName[128] = DEF_SERIAL_PORT;

static int32_t sbWidth;

static const char* sbLabels[NUM_SIDEBAR_BUTTONS] = {APP_HOME_SYMBOL, APP_CHART_SYMBOL, APP_SETTINGS_SYMBOL, NULL, NULL};
static const char* pBtnMB_OK[] = {"OK", ""};
static const char ddOptionsBaud[] = "9600\n19200\n38400\n57600\n115200";
static const char ddOptionsDatabits[] = "5\n6\n7\n8";
static const char ddOptionsParity[] = "none\nodd\neven\nspace";
static const char ddOptionsStopbits[] = "1\n2";

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void app_tick(void)
{
   char rxBuff[256];
#if 0
   if((lv_tick_elaps(lastTick) >= SCREEN_SLEEP) && (LCDpower == POWER_ON))
   {
      LCD_Off();
   }
#endif
#if 1
   if((lv_disp_get_inactive_time(NULL) >=  SCREEN_SLEEP) && (LCDpower == POWER_ON))
   {
      LCD_Off();
   }
#endif
   // Check for received messages from serial port
   if(serial_gets(pSerCtx, rxBuff) > 0)
   {
      lv_label_set_text(lblMsg, rxBuff);
      lv_obj_realign(lblMsg);
      parseSerial(rxBuff);
//		lv_obj_invalidate(lblMsg);
   }

}

/**
 * Create some objects
 */
void lv_application(void)
{
   FILE *Fbright, *Fpower;
   lv_fs_drv_t pcfs_drv;
   fs_abs_init(&pcfs_drv);

   if((Fbright = fopen(BRIGHTNESS_FILE, "r" )) == NULL)
   {
      sprintf(msg, "Cannot open %s", BRIGHTNESS_FILE);
      bLCDcontrol = false;
   }
   else if((Fpower = fopen(POWER_FILE, "r")) == NULL)
   {
      sprintf(msg, "Cannot open %s", POWER_FILE);
      fclose(Fbright);
   }
   else
   {
      sprintf(msg, "Machine controller");
      fscanf(Fbright, "%d", &LCDlevel);
      fclose(Fbright);
      fscanf(Fpower, "%d", &LCDpower);
   }


   lv_style_copy(&titleStyle, &lv_style_pretty_color);
   titleStyle.text.font = &lv_font_roboto_28;
   titleStyle.body.opa = LV_OPA_50;
   titleStyle.text.color = LV_COLOR_WHITE;


   // TODO close brightness file on exit

   /********************
    * CREATE A SCREEN
    *******************/
   /* Create a new screen and load it
    * Screen can be created from any type object type
    * Now a Page is used which is an objects with scrollable content*/
   scr = lv_disp_get_scr_act(NULL);

   lv_obj_t * wp = lv_img_create(scr, NULL);
   lv_img_set_src(wp, "P:/blue-background.bin");
   lv_obj_set_pos(wp, 0, 0);
   lv_obj_set_protect(wp, LV_PROTECT_POS);

//   return;


   /* Create container down left hand side */
   //lv_coord_t hres = lv_disp_get_hor_res(NULL);
   //lv_coord_t vres = lv_disp_get_ver_res(NULL);

   lv_obj_t * contSidebar = lv_cont_create(scr, NULL);
   lv_obj_set_style(contSidebar, &lv_style_transp);
   lv_obj_set_pos(contSidebar, 0 , 0);
//    lv_obj_set_width(contSidebar, 240);
   lv_cont_set_layout(contSidebar, LV_LAYOUT_COL_M);
   lv_cont_set_fit4(contSidebar, LV_FIT_NONE, LV_FIT_TIGHT, LV_FIT_NONE, LV_FIT_FLOOD);

   lv_style_t* contStyle = (lv_style_t*)lv_obj_get_style(contSidebar);
   contStyle->body.padding.inner = LV_DPI/5;
   contStyle->body.padding.top = LV_DPI/4;
   contStyle->body.padding.left = LV_DPI/8;
   contStyle->body.padding.right = LV_DPI/8;
   lv_obj_set_style(contSidebar, contStyle);

   // Create some buttons
   lv_obj_t * button, * lbl_btn;

   static lv_style_t  sbBtnStyle;
   lv_style_copy(&sbBtnStyle, &lv_style_plain_color);
   sbBtnStyle.text.font = &fontAwesomeExtra;

   for (uint32_t i = 0;  i < NUM_SIDEBAR_BUTTONS; i++)
   {
      btnSidebar[i] = lv_btn_create(contSidebar, NULL);
      lbl_btn = lv_label_create(btnSidebar[i], NULL);
      lv_obj_set_style(lbl_btn, &sbBtnStyle);
      lv_obj_set_size(btnSidebar[i], 3*LV_DPI/4, LV_DPI/2);
      if(sbLabels[i] != NULL)
      {
         lv_label_set_text(lbl_btn, sbLabels[i]);
      }
      lv_obj_set_event_cb(btnSidebar[i],btnSidebar_cb);
   }

   sbWidth = lv_obj_get_width(contSidebar);

   lv_obj_t * contStatus = lv_cont_create(scr, NULL);
   lv_obj_set_style(contStatus, &lv_style_transp);
   lv_obj_set_pos(contStatus, sbWidth, lv_disp_get_ver_res(NULL) - LV_DPI/3);
   lv_cont_set_fit4(contStatus, LV_FIT_NONE, LV_FIT_FLOOD, LV_FIT_NONE, LV_FIT_FLOOD);
   lv_cont_set_layout(contStatus, LV_LAYOUT_OFF);

   lblMsg = lv_label_create(contStatus, NULL);
   lv_style_copy(&lblOnBgStyle, &lv_style_transp);
   lblOnBgStyle.text.color = LV_COLOR_WHITE;
   lv_obj_set_style(lblMsg, &lblOnBgStyle);
   lv_obj_align(lblMsg, contStatus, LV_ALIGN_IN_LEFT_MID, LV_DPI/8, 0);
   lv_label_set_text(lblMsg, "Serial messages");

   lblStatus = lv_label_create(contStatus, NULL);
   lv_obj_set_style(lblStatus,  &lblOnBgStyle);
   lv_obj_align(lblStatus, contStatus, LV_ALIGN_IN_RIGHT_MID, -LV_DPI/8, 0);
   lv_label_set_text(lblStatus, "No comms");

   createControlScreen(sbWidth, lv_disp_get_ver_res(NULL) - LV_DPI/3);

   createScopeScreen(sbWidth, lv_disp_get_ver_res(NULL) - LV_DPI/3);
   lv_obj_set_hidden(contGraph, true);

   createSettingScreen(sbWidth, lv_disp_get_ver_res(NULL) - LV_DPI/3);
   lv_obj_set_hidden(contSettings, true);

   // Now attempt to open default serial port
   pSerCtx = serial_create(NULL);

   bSerialActive = openSerial(pSerCtx);

   const char msgWelcome[] = "Raspberry pi HMI\r\n";
   serial_send(pSerCtx, msgWelcome, strlen(msg));

   powerLCD(POWER_ON);
   lv_disp_trig_activity(NULL);

   return;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static bool openSerial(serial_t* pCtx)
{
   bool bSuccess = true;
   char msg[40];
   if(serial_connect(pCtx, ttyName, ttyParams.baud) < 0)
   {
      sprintf(msg, "Cannot open port:\n%s", ttyName);
      lvh_mbox_create_modal(lv_disp_get_scr_act(NULL), NULL, msg, pBtnMB_OK);
      bSuccess = false;
   }
   if(bSuccess)
   {
      sprintf(msg, "%s-%d", ttyName, ttyParams.baud);
      lv_label_set_text(lblStatus, msg);
   }
   else
   {
      lv_label_set_text(lblStatus, "Not connected");
   }
   lv_obj_realign(lblStatus);
   return bSuccess;
}


static void createControlScreen(int32_t left, int32_t bottom)
{
   // Now create container for our main screen
   contControl = lv_cont_create(lv_disp_get_scr_act(NULL), NULL);
   lv_obj_set_style(contControl, &lv_style_transp);
   lv_obj_set_height(contControl, bottom);
   lv_cont_set_layout(contControl, LV_LAYOUT_OFF);
   //    lv_obj_align(contControl, contSidebar, LV_ALIGN_OUT_RIGHT_TOP, 0, 0;)
   lv_obj_set_pos(contControl, left, 0);
   lv_cont_set_fit4(contControl, LV_FIT_NONE, LV_FIT_FLOOD, LV_FIT_NONE, LV_FIT_NONE);

   lv_style_t *contStyle = (lv_style_t*)lv_obj_get_style(contControl);
   contStyle->body.padding.inner = LV_DPI/4;
   lv_obj_set_style(contControl, contStyle);

   /****************
    * ADD A TITLE
    ****************/
   lv_obj_t * label1 = lv_label_create(contControl, NULL); /*First parameters (scr) is the parent*/
   lv_obj_set_style(label1, &titleStyle);
   lv_label_set_body_draw(label1, true);
   lv_label_set_text(label1, msg);  /*Set the text*/
   lv_obj_align(label1, NULL, LV_ALIGN_IN_TOP_MID, 0, 20);                        /*Set the x coordinate*/

   lv_obj_t * label = lv_label_create(contControl, NULL);
   lv_label_set_text(label, "Support: hacks@brooks.com");
   lv_obj_set_x(label, 50);                        /*Set the x coordinate*/
   lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 50);

   /*Create an animation to move the button continuously left to right*/
   lv_anim_t a;
   a.var = label;
   a.start = lv_obj_get_x(label);
   a.end = a.start + lv_obj_get_width(contControl) + lv_obj_get_x(contControl);
   a.exec_cb = (lv_anim_exec_xcb_t)lv_obj_set_x;
   a.path_cb = lv_anim_path_linear;
   a.ready_cb = NULL;
   a.act_time = -1000;                         /*Negative number to set a delay*/
   a.time = 4000;                               /*Animate in 400 ms*/
   a.playback = 1;                             /*Make the animation backward too when it's ready*/
   a.playback_pause = 0;                       /*Wait before playback*/
   a.repeat = 1;                               /*Repeat the animation*/
   a.repeat_pause = 500;                       /*Wait before repeat*/
   lv_anim_create(&a);


   /****************
    * ADD A SLIDER
    ****************/
   slider = lv_slider_create(contControl, NULL);                            /*Create a slider*/
   lv_obj_align(slider, label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
   lv_obj_set_size(slider, LV_DPI / 3, lv_obj_get_height(contControl) *2 / 3);            /*Set the size*/
   lv_slider_set_range(slider, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
   /*Set the current value*/
   lv_slider_set_value(slider, LCDlevel, false);
   lv_obj_set_event_cb(slider, slider_event_cb);


   /***********************
    * CREATE TWO BUTTONS
    ***********************/
   /*Create a button*/
   lv_obj_t * btn1 = lv_btn_create(contControl, NULL);         /*Create a button on the currently loaded screen*/
   lv_obj_set_event_cb(btn1, btn1_event_cb);                                  /*Set function to be called when the button is released*/
   lv_obj_align(btn1, slider, LV_ALIGN_OUT_RIGHT_TOP, 30, 0);               /*Align below the label*/
   lv_obj_set_width(btn1, (LV_DPI*3)/2);

   /*Create a label on the button (the 'label' variable can be reused)*/
   label = lv_label_create(btn1, NULL);
   static lv_style_t lblStyle;
   lv_style_copy(&lblStyle, (lv_style_t*)lv_obj_get_style(label));
   lblStyle.text.font = &lv_font_roboto_28;
   lv_obj_set_style(label, &lblStyle);

   //    lv_obj_set_style(label, &lblStyle);
   lv_label_set_text(label, "Left Button");
   lv_obj_align(label, btn1, LV_ALIGN_CENTER, 0, 0);

   /*Copy the previous button*/
   lv_obj_t * btn2 = lv_btn_create(contControl, NULL);                 /*Second parameter is an object to copy*/
   lv_obj_set_width(btn2, (LV_DPI*3)/2);
   lv_obj_align(btn2, slider, LV_ALIGN_OUT_RIGHT_BOTTOM, 30, 0);    /*Align next to the prev. button.*/
   lv_obj_set_event_cb(btn2, btn2_event_cb);                                  /*Set function to be called when the button is released*/

   /*Create a label on the button*/
   label = lv_label_create(btn2, label);
   lv_label_set_text(label, "Right Button");

   /* Label for the slider */
   label = lv_label_create(contControl, NULL);
   lv_label_set_text(label, "Brightness");
   lv_obj_align(label, slider, LV_ALIGN_OUT_RIGHT_MID, 30, 0);

   /***********************
    * ADD A DROP DOWN LIST
    ************************/
   lv_obj_t * ddlist = lv_ddlist_create(contControl, NULL);                     /*Create a drop down list*/
   lv_obj_align(ddlist, label, LV_ALIGN_OUT_RIGHT_TOP, 50, 0);         /*Align next to the slider*/
   lv_obj_set_top(ddlist, true);                                        /*Enable to be on the top when clicked*/
   lv_ddlist_set_options(ddlist, "None\nLittle\nHalf\nA lot\nAll");     /*Set the options*/
   lv_obj_set_event_cb(ddlist, ddlist_event_cb);                        /*Set function to call on new option is chosen*/

   return;
}

static void createScopeScreen(int32_t left, int32_t bottom)
{
   contGraph = lv_cont_create(lv_disp_get_scr_act(NULL), NULL);
   lv_obj_set_style(contGraph, &lv_style_transp);
   lv_obj_set_height(contGraph, bottom);
   lv_cont_set_layout(contGraph, LV_LAYOUT_OFF);
   lv_obj_set_pos(contGraph, left, 0);
   lv_cont_set_fit4(contGraph, LV_FIT_NONE, LV_FIT_FLOOD, LV_FIT_NONE, LV_FIT_NONE);

   lv_style_t *contStyle = (lv_style_t*)lv_obj_get_style(contGraph);
   contStyle->body.padding.inner = LV_DPI/4;
   lv_obj_set_style(contGraph, contStyle);
   /****************
    * CREATE A CHART
    ****************/
   lv_obj_t * chart = lv_chart_create(contGraph, NULL);                   /*Create the chart*/
   lv_obj_set_size(chart, lv_obj_get_width_fit(contGraph), lv_obj_get_height_fit(contGraph));   /*Set the size*/
   lv_obj_align(chart, contGraph, LV_ALIGN_CENTER, 0, 0);
   lv_chart_set_series_width(chart, 2);                                     /*Set the line width*/
   lv_chart_set_range(chart, -50, 50);
   lv_chart_set_point_count(chart, 100);
   lv_chart_set_div_line_count(chart, 10, 10);
   lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);

   /*Add a RED data series and set some points*/
   dl1 = lv_chart_add_series(chart, LV_COLOR_RED);

   return;
#if 0
   lv_chart_set_next(chart, dl1, 10);
   lv_chart_set_next(chart, dl1, 25);
   lv_chart_set_next(chart, dl1, 45);
   lv_chart_set_next(chart, dl1, 80);

   /*Add a BLUE data series and set some points*/
   lv_chart_series_t * dl2 = lv_chart_add_series(chart, lv_color_make(0x40, 0x70, 0xC0));
   lv_chart_set_next(chart, dl2, 10);
   lv_chart_set_next(chart, dl2, 25);
   lv_chart_set_next(chart, dl2, 45);
   lv_chart_set_next(chart, dl2, 80);
   lv_chart_set_next(chart, dl2, 75);
   lv_chart_set_next(chart, dl2, 90);
#endif
}

static void createSettingScreen(int32_t left, int32_t bottom)
{
   contSettings = lv_cont_create(scr, NULL);
   lv_obj_set_style(contSettings, &lv_style_transp);
   lv_obj_set_height(contSettings, bottom);
   lv_obj_set_pos(contSettings, left, 0);
   lv_cont_set_fit4(contSettings, LV_FIT_NONE, LV_FIT_FLOOD, LV_FIT_NONE, LV_FIT_NONE);
   lv_cont_set_layout(contSettings, LV_LAYOUT_OFF);

   lv_obj_t *label = lv_label_create(contSettings, NULL);
   lv_obj_set_style(label, &titleStyle);
   lv_label_set_body_draw(label, true);
   lv_label_set_text(label, "Settings");
   lv_obj_align(label, contSettings, LV_ALIGN_IN_TOP_MID, 0, 10);

   label = lv_label_create(contSettings, NULL);
   lv_obj_align(label, contSettings, LV_ALIGN_IN_TOP_LEFT, LV_DPI/8, LV_DPI/2);
   lv_obj_set_width(label, LV_DPI/2);
   lv_obj_set_style(label, &lblOnBgStyle);
   lv_label_set_text(label, "Serial");

   // create drop downs for serial port
   lv_obj_t *ddListPort = lv_ddlist_create(contSettings, NULL);
   lv_obj_align(ddListPort, label, LV_ALIGN_OUT_RIGHT_MID, LV_DPI/8, 0);
   lv_ddlist_set_fix_width(ddListPort, 250);
   lv_ddlist_set_draw_arrow(ddListPort, true);

   lv_obj_t *btnPort = lv_btn_create(contSettings, NULL);
   lv_obj_set_size(btnPort, 3*LV_DPI/4, LV_DPI/3);
   lv_obj_align(btnPort, ddListPort, LV_ALIGN_OUT_RIGHT_MID, LV_DPI/2, 0);
   lv_obj_t* lblBtn = lv_label_create(btnPort, NULL);
   lv_label_set_text(lblBtn, "Open");
   lv_obj_set_event_cb(btnPort, btn_port_event_cb);

   // Now populate list
   char *ptty = ls_tty();
   if(ptty != NULL)
   {
      lv_ddlist_set_options(ddListPort, ptty);
      free(ptty);
   }
   lvh_ddlist_set_selected_str(ddListPort, ttyName);
   lv_obj_set_event_cb(ddListPort, ddl_port_event_cb);

   // create drop down for baud rate
   lv_obj_t * label1 = lv_label_create(contSettings, NULL);
   lv_obj_align(label1, label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI/3);
   lv_obj_set_width(label1, LV_DPI/2);
   lv_obj_set_style(label1, &lblOnBgStyle);
   lv_label_set_text(label1, "Baud");

   lv_obj_t * ddListBaud = lv_ddlist_create(contSettings, NULL);
   lv_obj_align(ddListBaud, ddListPort, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI/3);

   lv_ddlist_set_options(ddListBaud, ddOptionsBaud);
   lv_ddlist_set_fix_width(ddListBaud, 150);
   lv_ddlist_set_draw_arrow(ddListBaud, true);
   char buff[20];
   sprintf(buff, "%d", ttyParams.baud);
   lvh_ddlist_set_selected_str(ddListBaud, buff);
   lv_obj_set_event_cb(ddListBaud, ddl_baud_event_cb);

   // create drop down list for parity
   lv_obj_t * ddListParity = lv_ddlist_create(contSettings, NULL);
   lv_obj_align(ddListParity, btnPort, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI/3);
   lv_ddlist_set_fix_width(ddListParity, 150);
   lv_ddlist_set_draw_arrow(ddListParity, true);
   lv_ddlist_set_options(ddListParity, ddOptionsParity);
   // Set default selection - this will break if enum values or sequence in ddOptionsParity change
   lv_ddlist_set_selected(ddListParity, (uint16_t)ttyParams.parity);
   lv_obj_set_event_cb(ddListParity, ddl_parity_event_cb);
   // label for parity
   label = lv_label_create(contSettings, label1);
   lv_obj_align(label, ddListBaud, LV_ALIGN_OUT_RIGHT_MID, LV_DPI/2, 0);
   lv_label_set_text(label, "Parity");

   // Create drop down list for data bits
   lv_obj_t * ddListDatabits = lv_ddlist_create(contSettings, NULL);
   lv_obj_align(ddListDatabits, ddListBaud, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI/3);
   lv_ddlist_set_fix_width(ddListDatabits, 150);
   lv_ddlist_set_draw_arrow(ddListDatabits, true);

   lv_ddlist_set_options(ddListDatabits, ddOptionsDatabits);
   // set default selection, this will break if enum values or sequence in ddOptionsDatabits change
   lv_ddlist_set_selected(ddListDatabits, ttyParams.databits-5);
   lv_obj_set_event_cb(ddListDatabits, ddl_databits_event_cb);
   // label for databits
   label = lv_label_create(contSettings, label1);
   lv_obj_align(label, label1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI/3);
   lv_label_set_text(label, "Data\nbits");

   // Create drop down list for stop bits
   lv_obj_t * ddListStopbits = lv_ddlist_create(contSettings, NULL);
   lv_obj_align(ddListStopbits, ddListParity, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI/3);
   lv_ddlist_set_fix_width(ddListStopbits, 150);
   lv_ddlist_set_draw_arrow(ddListStopbits, true);
   lv_ddlist_set_options(ddListStopbits, ddOptionsStopbits);
   // set default selection, this will break if enum changes or sequence in ddOptionsStopbits
   lv_ddlist_set_selected(ddListStopbits, ttyParams.stopbits);
   lv_obj_set_event_cb(ddListStopbits, ddl_stopbits_event_cb);
   // label for stopbits
   label = lv_label_create(contSettings, label1);
   lv_obj_align(label, ddListDatabits, LV_ALIGN_OUT_RIGHT_MID, LV_DPI/2, 0);
   lv_label_set_text(label, "Stop\nbits");

}


static void btnSidebar_cb(lv_obj_t * btn, lv_event_t event)
{
   // work out button index
   if(event == LV_EVENT_RELEASED)
   {
      if(btn == btnSidebar[0])
      {
         lv_obj_set_hidden(contGraph, true);
         lv_obj_set_hidden(contSettings, true);
         lv_obj_set_hidden(contControl, false);
      }
      else if(btn == btnSidebar[1])
      {
         lv_obj_set_hidden(contControl, true);
         lv_obj_set_hidden(contSettings, true);
         lv_obj_set_hidden(contGraph, false);
      }
      else if(btn == btnSidebar[2])
      {
         lv_obj_set_hidden(contGraph, true);
         lv_obj_set_hidden(contControl, true);
         lv_obj_set_hidden(contSettings, false);
      }
   }
}


/**
 * Called when a button is released
 * @param btn pointer to the released button
 * @param event the triggering event
 * @return LV_RES_OK because the object is not deleted in this function
 */
static void btn1_event_cb(lv_obj_t * btn, lv_event_t event)
{
   const char msg[] = "Button 1 pressed\r\n";
   if(event == LV_EVENT_RELEASED) {
      serial_send(pSerCtx, msg, strlen(msg));
   }
}


static void btn2_event_cb(lv_obj_t * btn, lv_event_t event)
{
   const char msg[] = "Button 2 pressed\r\n";
   if(event == LV_EVENT_RELEASED) {
      serial_send(pSerCtx, msg, strlen(msg));
   }
}

static void btn_port_event_cb(lv_obj_t * btn, lv_event_t event)
{
   if(event == LV_EVENT_RELEASED)
   {
      // TODO close, then open port with new parameters
      serial_close(pSerCtx);
      bSerialActive = false;
      usleep(50000);
      bSerialActive = openSerial(pSerCtx);
      if(bSerialActive)
      {
         serial_set_params(pSerCtx, &ttyParams);
      }
   }
}

static void btn_wake_cb(lv_obj_t * button, lv_event_t event)
{
   if(event == LV_EVENT_RELEASED)
   {
      lv_obj_del(btnWake);
      powerLCD(POWER_ON);
   }
}

/**
 * Called when a new option is chosen in the drop down list
 * @param ddlist pointer to the drop down list
 * @param event the triggering event
 * @return LV_RES_OK because the object is not deleted in this function
 */
static  void ddlist_event_cb(lv_obj_t * ddlist, lv_event_t event)
{
   if(event == LV_EVENT_VALUE_CHANGED) {
      uint16_t opt = lv_ddlist_get_selected(ddlist);            /*Get the id of selected option*/

      lv_slider_set_value(slider, (opt * 100) / 4, true);       /*Modify the slider value according to the selection*/
   }

}

static void ddl_port_event_cb(lv_obj_t * ddlist, lv_event_t event)
{
   if(event == LV_EVENT_PRESSED)
   {
      lv_obj_set_top(ddlist, true);
   }
   else if(event == LV_EVENT_VALUE_CHANGED)
   {
      lv_ddlist_get_selected_str(ddlist, ttyName, sizeof(ttyName));
   }
}

static void ddl_baud_event_cb(lv_obj_t * ddlist, lv_event_t event)
{
   if(event == LV_EVENT_PRESSED)
   {
      lv_obj_set_top(ddlist, true);
   }
   else if(event == LV_EVENT_VALUE_CHANGED)
   {
      char numStr[20];
      lv_ddlist_get_selected_str(ddlist, numStr, sizeof(numStr));
      ttyParams.baud = atol(numStr);
   }
}

static void ddl_databits_event_cb(lv_obj_t * ddlist, lv_event_t event)
{
   if(event == LV_EVENT_PRESSED)
   {
      lv_obj_set_top(ddlist, true);
   }
   else if(event == LV_EVENT_VALUE_CHANGED)
   {
      char str[20];
      lv_ddlist_get_selected_str(ddlist, str, sizeof(str));
      ttyParams.databits = (uint8_t)(str[0] == '0');
   }
}

static void ddl_parity_event_cb(lv_obj_t * ddlist, lv_event_t event)
{
   if(event == LV_EVENT_PRESSED)
   {
      lv_obj_set_top(ddlist, true);
   }
   else if(event == LV_EVENT_VALUE_CHANGED)
   {
      // NOTE this relies on order of tokens in ddOptionsParity
      uint16_t index = lv_ddlist_get_selected(ddlist);
      switch(index)
      {
      case 0:
         ttyParams.parity = PARITY_NONE;
         break;
      case 1:
         ttyParams.parity = PARITY_SPACE;
         break;
      case 2:
         ttyParams.parity = PARITY_EVEN;
         break;
      case 3:
         ttyParams.parity = PARITY_ODD;
         break;
      }
   }
}

static void ddl_stopbits_event_cb(lv_obj_t * ddlist, lv_event_t event)
{
   if(event == LV_EVENT_PRESSED)
   {
      lv_obj_set_top(ddlist, true);
   }
   else if(event == LV_EVENT_VALUE_CHANGED)
   {
      uint16_t index = lv_ddlist_get_selected(ddlist);
      if(index == 0)
      {
         ttyParams.stopbits = STOPBITS_1;
      }
      else
      {
         ttyParams.stopbits = STOPBITS_2;
      }
   }
}



static void slider_event_cb(lv_obj_t * slider, lv_event_t event)
{
   if(event == LV_EVENT_VALUE_CHANGED)
   {
      FILE *Fbright = fopen(BRIGHTNESS_FILE, "w" );
      LCDlevel = lv_slider_get_value(slider);
      fprintf(Fbright, "%d", LCDlevel);
      fclose(Fbright);
   }
}


static void LCD_Off(void)
{
   btnWake = lv_btn_create(scr, NULL);

   lv_obj_set_size(btnWake, LV_HOR_RES_MAX, LV_VER_RES_MAX);
   lv_btn_set_style(btnWake, LV_BTN_STYLE_REL, &lv_style_transp);
   lv_btn_set_style(btnWake, LV_BTN_STYLE_PR, &lv_style_transp);
   lv_btn_set_layout(btnWake, LV_LAYOUT_OFF);
   lv_obj_set_event_cb(btnWake, btn_wake_cb);

   powerLCD(POWER_OFF);
}

static void powerLCD(uint32_t power)
{
   FILE* Fpower = fopen(POWER_FILE, "w");
   if(power == 1)
      fprintf(Fpower, "1");
   else
      fprintf(Fpower, "0");
   fclose(Fpower);
   LCDpower = power;
}

static void parseSerial(char* msg)
{
   static char delims[] = " ,\t:";
   char * pTok;

   pTok = strtok(msg, delims);

   if(!strcmp(pTok, "slider"))
   {
      pTok = strtok(NULL, delims);

      int32_t val = strtol(pTok, NULL, 10);

      if(val <= lv_slider_get_max_value(slider))
      {
         lv_slider_set_value(slider, val, LV_ANIM_ON);
      }
   }
   else if(!strcmp(pTok, "wake"))
   {
      lv_obj_del(btnWake);
      powerLCD(POWER_ON);
      lv_disp_trig_activity(NULL);
   }

}

#endif // LV_USE_APPLICATION


