/*
 * buzzer.c
 *
 *  Created on: 7 Oct 2019
 *      Author: Rob
 */

#include <stdio.h>
#include <fcntl.h>

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>


#include "buzzer.h"

/****************************************************************************/
/* MACROS                                                                   */
/****************************************************************************/
#define PWM_IOCTL_SET_FREQ		1
#define PWM_IOCTL_STOP			0


/****************************************************************************/
/* Local prototypes                                                         */
/****************************************************************************/
static void control_buzzer(void);
static void open_buzzer(void);
static void close_buzzer(void);
static void stop_buzzer(void);
static void set_buzzer_freq(int freq);

/****************************************************************************/
/* Private storage                                                          */
/****************************************************************************/
static int fd = -1;
static volatile long buzzFreq;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_t ptBuzzer = (pthread_t)NULL;



/****************************************************************************/
/* Exported functions                                                       */
/****************************************************************************/
int buzzerInit(void)
{
   open_buzzer();
   pthread_mutex_init(&lock,NULL);
   int s = -1;

   // Create buzzer control thread
   if((s = pthread_create(&ptBuzzer, NULL, (void*)&control_buzzer, NULL)) != 0)
   {
      perror("pthread_create failure");
   }

   return s;
}


int doBuzz(int freq)
{
   if(ptBuzzer == 0)
   {
      errno = EPERM;
      perror("Buzzer not initialised");
      return -1;
   }

   if((freq < 10) || (freq > 10000))
   {
      errno = EPERM;
      perror("Invalid frequency");
      return -1;
   }

   errno = 0;
   buzzFreq = (long)freq;
   pthread_cond_signal(&cond);

   return 0;
}


int buzzerDeinit(void)
{
   int s = -1;
   void *res;

   if(ptBuzzer == 0)
   {
      errno = EPERM;
      perror("Buzzer not initialised");
      return -1;
   }

   buzzFreq = -1;
   pthread_cond_signal(&cond);

   if((s = pthread_join(ptBuzzer, &res)) != 0)
   {
      perror("pthread_join failure");
   }

   errno = 0;

   pthread_exit(NULL);

   close_buzzer();

   return s;
}


/****************************************************************************/
/* Private functions                                                        */
/****************************************************************************/
/* Buzzer thread */
static void control_buzzer(void)
{
   int Freq;
   pthread_mutex_lock(&lock);
   while(1)
   {
      pthread_cond_wait(&cond, &lock);
      if (buzzFreq > 0)
      {
         set_buzzer_freq(buzzFreq);
         usleep(BEEP_DURATION);
         stop_buzzer();
         buzzFreq = 0;
      }
      else if(buzzFreq < 0)
      {
         pthread_mutex_unlock(&lock);
         pthread_exit(NULL);
      }
   }

}

static void open_buzzer(void)
{
	fd = open("/dev/pwm", 0);
	if (fd < 0) {
		perror("open pwm_buzzer device");
		exit(1);
	}

	// any function exit call will stop the buzzer
	atexit(close_buzzer);
}


static void close_buzzer(void)
{
	if (fd >= 0) {
		ioctl(fd, PWM_IOCTL_STOP);
		close(fd);
		fd = -1;
	}
}

static void set_buzzer_freq(int freq)
{
	// this IOCTL command is the key to set frequency
	int ret = ioctl(fd, PWM_IOCTL_SET_FREQ, freq);
	if(ret < 0) {
		perror("set the frequency of the buzzer");
		exit(1);
	}
}
static void stop_buzzer(void)
{
	int ret = ioctl(fd, PWM_IOCTL_STOP);
	if(ret < 0) {
		perror("stop the buzzer");
		exit(1);
	}
}

