
#ifndef SERIAL_H
#define SERIAL_H

#define BUFF_SIZE 512
#define POLL_TIMEOUT 2000

#include <stdint.h>
#include "tty_info.h"

#ifdef __cplusplus
extern "C" {
#endif


   typedef struct serial_s serial_t;

   typedef enum {STOPBITS_1, STOPBITS_2} stopbits_t;

   typedef enum {PARITY_NONE, PARITY_SPACE, PARITY_EVEN, PARITY_ODD} parity_t;

   typedef struct
   {
      int32_t baud;
      stopbits_t stopbits;
      parity_t parity;
      uint8_t databits;
   }ser_param_t ;

   /**
    * Create the serial structure.
    * Convenience method to allocate memory
    * and instantiate objects.
    * @return serial structure.
    */
   serial_t* serial_create(void (*pCallback)(char* pMsg));

   /**
    * Destroy the serial structure
    */
   void serial_destroy(serial_t* s);

   /**
    * Connect to a serial device.
    * @param s - serial structure.
    * @param device - serial device name.
    * @param baud - baud rate for connection.
    * @return -ve on error, 0 on success.
    */
   int32_t serial_connect(serial_t* s, char device[], int32_t baud);

   /**
    * Set port parameters.
    * @param s - serial structure.
    * @param pParam - pointer to struct containing baud, data len, parity etc.
    * @param length - size of the data array.
    */
   int32_t serial_set_params(serial_t* s, ser_param_t* pParam);
   /**
    * Send data.
    * @param s - serial structure.
    * @param data - character array to transmit.
    * @param length - size of the data array.
    */

   int32_t serial_send(serial_t* s, const uint8_t data[], int32_t length);

   /**
    * Send a single character.
    * @param s - serial structure.
    * @param data - single character to be sent.
    */
   void serial_put(serial_t* s, uint8_t data);

   /**
    * Fetch message from the serial buffer.
    * @param s - serial structure.
    * @return character. Null if empty.
    */
   int32_t serial_gets(serial_t* s, char* pBuf);

   /**
    * Clear the serial buffer.
    * @param s - serial structure.
    */
   void serial_clear(serial_t* s);

   /**
    * Close the serial port.
    * @param s - serial structure.
    * @return value of close().
    */
   int32_t serial_close(serial_t* s);

#ifdef __cplusplus
}
#endif

#endif
