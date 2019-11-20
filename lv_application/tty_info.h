#ifndef TTY_INFO_H
#define TTY_INFO_H

/**
 * @brief Returns \n delimited string with all active serial (tty) devices
 * @return pointer to string
 * @warning calling function must free returned pointer to char
*/
char* ls_tty(void);

/**
 * @brief Returns string with USB device ID for string parameter \dev\tty
 * @param char* device name e.g. /dev/USB0
 * @return pointer to ID string, null if non USB device supplied as parameter
 * @warning calling function must free returned pointer to char
*/
char* find_tty_id(char* ptty);

#endif // TTY_INFO_H
