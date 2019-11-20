/*
 * fs_linux.c
 *
 *  Created on: 14 Nov 2019
 *      Author: rgee
 */

#include "../lvgl/lvgl.h"
#include "fs_abs.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

#ifdef linux

#if LV_USE_FILESYSTEM
static lv_fs_res_t fs_open(lv_fs_drv_t * drv, void * file_p, const char * fn, lv_fs_mode_t mode);
static lv_fs_res_t fs_close(lv_fs_drv_t * drv, void * file_p);
static lv_fs_res_t fs_read(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br);
static lv_fs_res_t fs_seek(lv_fs_drv_t * drv, void * file_p, uint32_t pos);
static lv_fs_res_t fs_tell(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p);



void fs_abs_init(lv_fs_drv_t * pDrv)
{
   memset(pDrv, 0, sizeof(lv_fs_drv_t));

   pDrv->file_size = sizeof(FILE*);       /*Set up fields...*/
   pDrv->letter = 'P';
   pDrv->open_cb = fs_open;
   pDrv->close_cb = fs_close;
   pDrv->read_cb = fs_read;
   pDrv->seek_cb = fs_seek;
   pDrv->tell_cb = fs_tell;
   lv_fs_drv_register(pDrv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Open a file from the PC
 * @param drv pointer to the current driver
 * @param file_p pointer to a FILE* variable
 * @param fn name of the file.
 * @param mode element of 'fs_mode_t' enum or its 'OR' connection (e.g. FS_MODE_WR | FS_MODE_RD)
 * @return LV_FS_RES_OK: no error, the file is opened
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_open(lv_fs_drv_t * drv, void * file_p, const char * fn, lv_fs_mode_t mode)
{
   (void) drv; /*Unused*/

   errno = 0;

   const char * flags = "";

   if(mode == LV_FS_MODE_WR) flags = "wb";
   else if(mode == LV_FS_MODE_RD) flags = "rb";
   else if(mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) flags = "a+";

   /*Make the path relative to the current directory (the projects root folder)*/
   char buf[256];
   sprintf(buf, "./%s", fn);

   FILE* f = fopen(buf, flags);
   if(f == NULL) return LV_FS_RES_UNKNOWN;
   else {
      fseek(f, 0, SEEK_SET);

      /* 'file_p' is pointer to a file descriptor and
       * we need to store our file descriptor here*/
      FILE* * fp = file_p;        /*Just avoid the confusing casings*/
      *fp = f;
   }

   return LV_FS_RES_OK;
}


/**
 * Close an opened file
 * @param drv pointer to the current driver
 * @param file_p pointer to a FILE* variable. (opened with lv_ufs_open)
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv__fs_res_t enum
 */
static lv_fs_res_t fs_close(lv_fs_drv_t * drv, void * file_p)
{
   (void) drv; /*Unused*/

   FILE* * fp = file_p;        /*Just avoid the confusing casings*/
   fclose(*fp);
   return LV_FS_RES_OK;
}

/**
 * Read data from an opened file
 * @param drv pointer to the current driver
 * @param file_p pointer to a FILE variable.
 * @param buf pointer to a memory block where to store the read data
 * @param btr number of Bytes To Read
 * @param br the real number of read bytes (Byte Read)
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv__fs_res_t enum
 */
static lv_fs_res_t fs_read(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br)
{
   (void) drv; /*Unused*/

   FILE* * fp = file_p;        /*Just avoid the confusing casings*/
   *br = fread(buf, 1, btr, *fp);
   return LV_FS_RES_OK;
}

/**
 * Set the read write pointer. Also expand the file size if necessary.
 * @param drv pointer to the current driver
 * @param file_p pointer to a FILE* variable. (opened with lv_ufs_open )
 * @param pos the new position of read write pointer
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv__fs_res_t enum
 */
static lv_fs_res_t fs_seek(lv_fs_drv_t * drv, void * file_p, uint32_t pos)
{
   (void) drv; /*Unused*/

   FILE* * fp = file_p;        /*Just avoid the confusing casings*/
   fseek(*fp, pos, SEEK_SET);
   return LV_FS_RES_OK;
}

/**
 * Give the position of the read write pointer
 * @param drv pointer to the current driver
 * @param file_p pointer to a FILE* variable.
 * @param pos_p pointer to to store the result
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv__fs_res_t enum
 */
static lv_fs_res_t fs_tell(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p)
{
   (void) drv; /*Unused*/
   FILE* * fp = file_p;        /*Just avoid the confusing casings*/
   *pos_p = ftell(*fp);
   return LV_FS_RES_OK;
}


#endif

#endif
