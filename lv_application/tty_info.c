/*
 * test.c
 *
 *  Created on: 18 Nov 2019
 *      Author: rob
 */


#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "tty_info.h"

#define SERIAL_BY_DEV	"/dev/serial/by-id"
#define SYS_TTY			"/sys/class/tty"

#if 0
int main(void)
{
   char* pDev;

   if((pDev = ls_tty()) == NULL)
   {
      printf("No serial devices found\n");
      exit(1);
   }
   else
   {
      printf("%s\n", pDev);
      // iterate through result and if there are any USB ports, look up the ID
      const char delim[] = "\n";
      char *pTok;
      pTok = strtok(pDev, delim);
      while(pTok != NULL)
      {
         char *pId = find_tty_id(pTok);
         if(pId != NULL)
         {
            printf("%s:\t%s\n", pTok, pId);
            free(pId);
         }
         pTok = strtok(NULL, delim);
      }
      free(pDev);
   }
   exit(0);
}

#endif

/**
 * @brief Returns \n delimited string with all active serial (tty) devices
 * @return pointer to string
 * @warning calling function must free returned pointer to char
*/
char* ls_tty(void)
{
   DIR* pDir;
   struct dirent* pEnt;
   char* pPath = NULL;
   char* pResult = NULL;
   char ttyItem[261];

   // Now open /sys/class/tty
   if((pDir = opendir(SYS_TTY)) == NULL)
   {
      perror("Cannot open "SYS_TTY);
   }
   else
   {
      pPath = malloc(PATH_MAX);
      while((pEnt = readdir(pDir)) != NULL)
      {
         if((pEnt->d_name[0] != '.') && !strncmp(pEnt->d_name, "tty", 3))
         {
#if 0
            // Exclude ttyUSB as these have already been counted
            if(strstr(pEnt->d_name, "USB") != NULL)
            {
               continue;
            }
#endif
            // 1st case, if ./device/driver directory is present
            strcpy(pPath, SYS_TTY);
            strcat(pPath, "/");
            strcat(pPath, pEnt->d_name);
            strcat(pPath, "/device/driver");
            if(access(pPath, F_OK) == 0)
            {
               // if driver is serial8250 then chances are device isn't
               // physically present
               strcat(pPath, "/serial8250");
               if(access(pPath, F_OK) != 0)
               {
                  if(pResult == NULL)
                  {
                     pResult = malloc(1);
                     pResult[0] = '\x0';
                  }
                  sprintf(ttyItem, "/dev/%s\n", pEnt->d_name);
                  pResult = realloc(pResult, strlen(ttyItem) + strlen(pResult) + 1);
                  strcat(pResult, ttyItem);
               }
            }
         }
      }
      if(pResult != NULL)
      {
         // get rid of last newline
         int last_newline = strlen(pResult) - 1;
         pResult[last_newline] = '\x0';
      }
      closedir(pDir);
      free(pPath);
   }

   return pResult;
}


/**
 * @brief Returns string with USB device ID for string parameter \dev\tty
 * @param char* device name e.g. /dev/USB0
 * @return pointer to ID string, null if non USB device supplied as parameter
 * @warning calling function must free returned pointer to char
*/
char* find_tty_id(char* ptty)
{
   DIR* pDir;
   struct dirent* pEnt;
   struct stat info;
   ssize_t nbytes, bufsiz;
   char* pPath;
   char* buf;
   char* pResult = NULL;

   if(strstr(ptty, "USB") == NULL)
   {
      return pResult;
   }

   if((pDir = opendir(SERIAL_BY_DEV)) == NULL)
   {
      printf("No USB serial devices detected\n");
   }
   else
   {
      pPath = malloc(PATH_MAX);
      while((pEnt = readdir(pDir)) != NULL)
      {
         if(pEnt->d_name[0] != '.')
         {
            strcpy(pPath, SERIAL_BY_DEV"/");
            strcat(pPath, pEnt->d_name);
            if(lstat(pPath, &info) < 0)
            {
               perror("lstat error");
               continue;
            }
            if(S_ISLNK(info.st_mode))
            {
               /* Add one to the link size, so that we can determine whether
               		  the buffer returned by readlink() was truncated. */
               bufsiz = info.st_size + 1;
               buf = malloc(bufsiz);
               nbytes = readlink(pPath, buf, bufsiz);


               if(nbytes < 0)
               {
                  perror("readlink failure");
                  continue;
               }

               if(nbytes == bufsiz)
               {
                  perror("WARNING: returned buffer may have been truncated");
               }
               else
               {
                  buf[nbytes] = '\x0';
                  char *pSep = strrchr(pPath, '/');
                  *(pSep+1) = '\x0';
                  strcat(pPath, buf);
                  char* rbuf = malloc(PATH_MAX);
                  realpath(pPath, rbuf);

                  // Look for parameter tty name
                  if(strstr(rbuf, ptty) != NULL)
                  {
                     // remove usb prefix from id string
                     char* plch = strchr(pEnt->d_name, '-')+1;

                     char* prchr = strchr(plch, '-');
                     *prchr = '\x0';

                     pResult = malloc(strlen(plch)+1);
                     strcpy(pResult, plch);
                  }

                  free(rbuf);
               }

               free(buf);
            }
         }
      }
      free(pPath);
      closedir(pDir);
   }

#if 0
   if(pResult != NULL)
   {
      printf("%s\n", pResult);
   }
#endif
   return pResult;
}



