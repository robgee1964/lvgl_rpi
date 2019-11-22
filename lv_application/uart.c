
#include "uart.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

/**
 * @struct Serial device structure.
 * Encapsulates a serial connection.
 */
struct serial_s {
   int32_t fd;                  //>! Connection file descriptor.
   int32_t state;               //>! Signifies connection state.
   int32_t running;             //>! Signifies thread state.
   void (*pRxCallback)(char* pMsg);
   volatile char rxbuff[BUFF_SIZE];  //>! Buffer for RX data.
   volatile bool bAvailable;
   pthread_t rx_thread;     //>! Listening thread.
};

// ---------------        Internal Functions        ---------------

static int32_t serial_resolve_baud(int32_t baud);

/**
 * @brief Start the serial threads.
 * This spawns the listening and transmitting threads
 * if they are not already running.
 * @param s - serial structure.
 * @return 0 on success, or -1 on error.
 */
static int32_t serial_start(serial_t* s);

/**
 * Recieve data.
 * Retrieves data from the serial device.
 * @param s - serial structure.
 * @param data - pointer to a buffer to read data into.
 * @param maxLength - size of input buffer.
 * @return amount of data recieved.
 */
static int32_t serial_recieve(serial_t* obj, uint8_t data[], int32_t maxLength);

static void serial_rx_callback(serial_t* s, char data[]);

/**
 * @brief Serial Listener Thread.
 * This blocks waiting for data to be recieved from the serial device,
 * and calls the serial_callback method with appropriate context when
 * data is recieved.
 * Exits when close method is called, or serial error occurs.
 * @param param - context passed from thread instantiation.
 */
static void *serial_data_listener(void *param);



/**
 * Stop serial listener thread.
 * @param s - serial structure.
 * @return 0;
 */
static int32_t serial_stop(serial_t* s);


// ---------------        External Functions        ---------------

//Create serial object.
serial_t* serial_create(void (*pCallback)(char* pMsg))
{
   //Allocate serial object.
   serial_t* s = malloc(sizeof(serial_t));
   //Reconfigure buffer object.
   s->bAvailable = false;
   s->fd = -1;
   if(pCallback != NULL)
   {
      s->pRxCallback = pCallback;
   }
   //Return pointer.
   return s;
}


void serial_destroy(serial_t* s)
{
   free(s);
}


//Connect to serial device.
int32_t serial_connect(serial_t* s, char device[], int32_t baud)
{
   struct termios oldtio;

   // Resolve baud.
   int32_t speed = serial_resolve_baud(baud);
   if (speed < 0) {
      printf("Error: Baud rate not recognized.\r\n");
      return -3;
   }

   //Open device.
   s->fd = open(device, O_RDWR);
   //Catch file open error.
   if (s->fd < 0) {
      perror(device);
      return -1;
   }
   //Retrieve settings.
   tcgetattr(s->fd, &oldtio);
   //Set baud rate.
   cfsetspeed(&oldtio, speed);
   //Flush cache.
   tcflush(s->fd, TCIFLUSH);
   // disable local echo
   oldtio.c_lflag &= ~ECHO;
   //Apply settings.
   tcsetattr(s->fd, TCSANOW, &oldtio);

   //Start listener thread.
   int32_t res = serial_start(s);
   //Catch error.
   if (res < 0) {
      printf("Error: serial thread could not be spawned\r\n");
      return -2;
   }

   //Indicate connection was successful.
   s->state = 1;
   return 0;
}


int32_t serial_set_params(serial_t* s, ser_param_t* pParam)
{
   if(s->fd < 0)
   {
      return -1;  // fail if port not open
   }
   struct termios tio;
   //Retrieve settings.
   tcgetattr(s->fd, &tio);

   int32_t speed = serial_resolve_baud(pParam->baud);

   if (speed < 0) {
      printf("Error: Baud rate not recognized\n");
      return -3;
   }

   if((pParam->databits < 5) || (pParam->databits > 8))
   {
      printf("Error: invalid data bits\n");
   }

   // setbaudrate
   cfsetspeed(&tio, speed);

   // set stop bits
   if(pParam->stopbits ==  STOPBITS_1)
   {
      tio.c_cflag &= ~CSTOPB;
   }
   else if(pParam->stopbits == STOPBITS_2)
   {
      tio.c_cflag |= CSTOPB;
   }

   // set data bits
   uint32_t maskc = ~(CS5 | CS6 | CS7 | CS8);
   uint32_t masks;
   switch(pParam->databits)
   {
   case 5:
      masks = CS5;
      break;
   case 6:
      masks = CS6;
      break;
   case 7:
      masks = CS7;
      break;
   case 8:
      masks = CS8;
      break;
   default:
      maskc = 0xffffffffUL;
      masks = 0UL;
      break;
   }
   tio.c_cflag &= maskc;
   tio.c_cflag |= masks;

   // set parity
   switch(pParam->parity)
   {
   case PARITY_NONE:
   case PARITY_SPACE:
      tio.c_cflag &= (PARENB | PARODD);
      break;

   case PARITY_EVEN:
      tio.c_cflag &= ~PARODD;
      tio.c_cflag |= PARENB;
      break;

   case PARITY_ODD:
      tio.c_cflag |= PARODD;
      tio.c_cflag |= PARENB;
      break;
   }

   tcsetattr(s->fd, TCSANOW, &tio);

   return 0;
}

//Send data.
int32_t serial_send(serial_t* s, const uint8_t data[], int32_t length)
{
   if(s->fd < 0)
   {
      return -1;
   }
   int32_t res = write(s->fd, data, length);
   return res;
}

void serial_put(serial_t* s, uint8_t data)
{
   char arr[1];
   arr[0] = data;
   write(s->fd, arr, 1);
}


//Fetch a message
int32_t serial_gets(serial_t* s, char* pBuf)
{
   int32_t count = -1;
   if(s->fd < 0)
   {
      return count;
   }
   if(s->bAvailable)
   {
      strcpy(pBuf, (void*)s->rxbuff);
      s->bAvailable = false;
      count = strlen(pBuf);
   }
   return count;
}

void serial_clear(serial_t* s)
{
   //Clear the buffer.
   s->bAvailable = false;
}

//Close serial port.
int32_t serial_close(serial_t* s)
{
   if(s->fd <0)
   {
      return -1;
   }
   //Stop thread.
   serial_stop(s);
   close(s->fd);
   return 0;
}

// ---------------        Internal Functions        --------------

//Stop serial listener thread.
static int32_t serial_stop(serial_t* s)
{
   s->running = 0;
   return 0;
}

// Resolves standard baud rates to linux constants.
static int32_t serial_resolve_baud(int32_t baud)
{
   int32_t speed;
   // Switch common baud rates to temios constants.
   switch (baud) {
   case 9600:
      speed = B9600;
      break;
   case 19200:
      speed = B19200;
      break;
   case 38400:
      speed = B38400;
      break;
   case 57600:
      speed = B57600;
      break;
   case 115200:
      speed = B115200;
      break;
   default:
      speed = -1;
      break;
   }
   // Return.
   return speed;
}

// Start serial listener.
static int32_t serial_start(serial_t* s)
{
   //Only start if it is not currently running.
   if (s->running != 1) {
      //Set running.
      s->running = 1;
      //Spawn thread.
      int32_t res;
      res = pthread_create(&s->rx_thread, NULL, serial_data_listener, (void*) s);
      if (res != 0) {
         return -2;
      }
      //Return result.
      return 0;
   } else {
      return -1;
   }
}



//Recieve data.
static int32_t serial_recieve(serial_t* s, uint8_t data[], int32_t maxLength)
{
   return read(s->fd, data, maxLength);
}

//Callback to store data in buffer.
static void serial_rx_callback(serial_t* s, char data[])
{
   //Put data into buffer.
   strcpy((void*)s->rxbuff, data);
   s->bAvailable = true;
   if(s->pRxCallback != NULL)
   {
      s->pRxCallback(data);
   }
}

//Serial data listener thread.
static void *serial_data_listener(void *param)
{
   int32_t res = 0;
   int32_t err = 0;
   struct pollfd ufds;
   uint8_t buff[BUFF_SIZE];

   //Retrieve paramaters and store locally.
   serial_t* serial = (serial_t*) param;

   //Run until ended.
   while (serial->running != 0) {
      int32_t count = serial_recieve(serial, buff, BUFF_SIZE - 1);
      //If data was recieved.
      if (count > 0) {
         //Pad end of buffer to ensure there is a termination symbol.
         buff[count-1] = '\0';
         // Call the serial callback.
         serial_rx_callback(serial, (char *)buff);
         //If an error occured.
      } else if (count < 0) {
         //Inform user and exit thread.
         printf("Error: Serial disconnect\r\n");
         err = 1;
         break;
      }
      //Otherwise, keep going around.
   }
   //If there was an error, close socket.
   if (err) {
      serial_close(serial);
      //raise(SIGLOST);
   }
   //Close file.
   res = close(serial->fd);

   return NULL;
}
