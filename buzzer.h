/*
 * buzzer.h
 *
 *  Created on: 7 Oct 2019
 *      Author: Rob
 */

#ifndef BUZZER_H_
#define BUZZER_H_

#define BEEP_DURATION   10000L
#define BEEP_FREQ		1000


int buzzerInit(void);
int doBuzz(int freq);
int buzzerDeinit(void);

#endif /* BUZZER_H_ */
