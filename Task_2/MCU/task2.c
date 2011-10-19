/*
 * "Copyright (c) 2008 Robert B. Reese, Bryan A. Jones, J. W. Bruce ("AUTHORS")"
 * All rights reserved.
 * (R. Reese, reese_AT_ece.msstate.edu, Mississippi State University)
 * (B. A. Jones, bjones_AT_ece.msstate.edu, Mississippi State University)
 * (J. W. Bruce, jwbruce_AT_ece.msstate.edu, Mississippi State University)
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice, the following
 * two paragraphs and the authors appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE "AUTHORS" BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE "AUTHORS"
 * HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE "AUTHORS" SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE "AUTHORS" HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Please maintain this header in its entirety when copying/modifying
 * these files.
 *
 *
 */

// INCLUDEs go here  (First include the main esos.h file)
//      After that, the user can include what they need
#include    "esos.h"
#include    "pic24_adc.h"
#ifdef __linux
#include    "esos_pc.h"
#include    "esos_pc_stdio.h"

// INCLUDE these so that printf() and our PC hacks work
#include <stdio.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#else
#include    "esos_pic24.h"
#include    "esos_pic24_rs232.h"
#endif

// DEFINEs go here
#ifndef __linux
#define   CONFIG_LED1()   CONFIG_RB15_AS_DIG_OUTPUT()
#define   CONFIG_TEMP_SENSOR()  CONFIG_AN1_AS_ANALOG()
#define   AUTO_SAMPLE_TIME      31
#define   USE_12BIT_ENCODING    1
#define   LED1            _LATB15
#else
#define   CONFIG_LED1()   printf("called CONFIG_LED1()\n");
uint8_t     LED1 = TRUE;      // LED1 is initially "on"
#endif

// PROTOTYPEs go here
void bubbleSort(uint16 [], uint8);

// GLOBALs go here
//  Generally, the user-created semaphores will be defined/allocated here
static uint8_t psz_CRNL[3]= {0x0D, 0x0A, 0};
uint16 u16_tempExtMax = 174;			//For LM60 -- Maximum Default Exterior Temperature
uint16 u16_tempExtMin = 1205;			//For LM60 -- Maximum Default Exterior Temperature
uint16  au16_tempBuffer[7];		//For LM60 -- Exterior Temperature Buffer. Holds up to seven temp readings
static const uint16  u16_sizeOfArray = 7;
static const uint8   u8_middleOfArray = 3;

#ifdef __linux
/*
 * Simulate the timer ISR found on a MCU
 *   The PC doesn't have a timer ISR, so this task will periodically
 *   call the timer services callback instead.
 *   USED ONLY FOR DEVELOPMENT AND TESTING ON PC.
 *   Real MCU hardware doesn't need this task
 */
ESOS_USER_TASK( __simulated_isr ) {
  ESOS_TASK_BEGIN();
  while (TRUE) {
    // call the ESOS timer services callback just like a real H/W ISR would
    __esos_tmrSvcsExecute();
    ESOS_TASK_WAIT_TICKS( 1 );

  } // endof while(TRUE)
  ESOS_TASK_END();
} // end child_task
#endif

/************************************************************************
 * User supplied functions
 ************************************************************************
 */

/**
 * An ESOS task to mimic the heartbeat LED found
 * in the PIC24 support library code used in Chapters 8-13.
 *
 * Toggle LED1, wait 250ms, repeat forever.
 *
 * \note Since this heartbeat is performed in an ESOS task,
 * a flashing LED indicates that the ESOS scheduler is still
 * running properly.  If the LED quits flashing, the ESOS
 * scheduler is no longer rotating through the runnable task
 * list.  The most likely reason is that some task has ceased
 * "yielding" the processor, and is caught in some deadlock
 * or otherwise infinite loop.
 * \hideinitializer
 */
ESOS_USER_TASK(heartbeat_LED) {
  ESOS_TASK_BEGIN();
  while (TRUE) {
    LED1 = !LED1;
#ifdef __linux
    if (LED1) {
      printf("\a");
      fflush(stdout);
    }
#endif
    ESOS_TASK_WAIT_TICKS( 500 );
  } // endof while(TRUE)
  ESOS_TASK_END();
} // end upper_case()

/**
 * Respond to user request for temperature
 */
ESOS_USER_TASK(send_temperature) {
  static  uint8_t           u8_char;
  static  uint16_t           u16_temperature;

  ESOS_TASK_BEGIN();
  while (TRUE) {
    ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
    ESOS_TASK_WAIT_ON_GET_UINT8(u8_char);
    ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
    if (u8_char == '@') {
        /* If the user types '@' they want the minimum temp reading followed
         * by the max temp reading
        */
        ESOS_TASK_WAIT_ON_AVAILABLE_OUT_COMM();
        ESOS_TASK_WAIT_ON_SEND_UINT32_AS_HEX_STRING((uint32_t) u16_tempExtMin);
        ESOS_TASK_SIGNAL_AVAILABLE_OUT_COMM();
        ESOS_TASK_WAIT_ON_AVAILABLE_OUT_COMM();
        ESOS_TASK_WAIT_ON_SEND_UINT32_AS_HEX_STRING((uint32_t) u16_tempExtMax);
        ESOS_TASK_SIGNAL_AVAILABLE_OUT_COMM();
    }
    else if (u8_char == 'K')
    {
        /* If the user types K they want the current temperature median
        *  filtered. So we will use a bubble sort on the temp array
        *  and then return the middle element to the user.
        */
        bubbleSort(au16_tempBuffer, u16_sizeOfArray);
        u16_temperature = au16_tempBuffer[u8_middleOfArray];     //Store middle element
        ESOS_TASK_WAIT_ON_AVAILABLE_OUT_COMM();
        ESOS_TASK_WAIT_ON_SEND_UINT32_AS_HEX_STRING((uint32_t) u16_temperature);
        ESOS_TASK_SIGNAL_AVAILABLE_OUT_COMM();
    }
    else if (u8_char == '#')
    {
        //If the user types '#' return BCD VREF voltage 0x3000
        static uint32_t u32_bcdRefVoltage = 0x3000;
        ESOS_TASK_WAIT_ON_AVAILABLE_OUT_COMM();
        ESOS_TASK_WAIT_ON_SEND_UINT32_AS_HEX_STRING(u32_bcdRefVoltage);
        ESOS_TASK_SIGNAL_AVAILABLE_OUT_COMM();
    }
  } // endof while(TRUE)
  ESOS_TASK_END();
} // end upper_case()

ESOS_USER_TASK(read_external_temperature){
        static uint16 u16_temperature;
        static uint8 u8_position = 0;
	ESOS_TASK_BEGIN();
		while(1)
		{
			configADC1_ManualCH0( ADC_CH0_POS_SAMPLEA_AN1, AUTO_SAMPLE_TIME, 
                                USE_12BIT_ENCODING );
			u16_temperature = convertADC1();						//Read the voltage from the ADC
			if(u16_temperature>u16_tempExtMax)							//If value is higher than max then replace old max.
				u16_tempExtMax = u16_temperature;
			else if(u16_temperature<u16_tempExtMin)						//If value is lower than min then replace old min.
				u16_tempExtMin = u16_temperature;

			au16_tempBuffer[u8_position] = u16_temperature; 			//Add current reading to the buffer
			u8_position++;
			if(u8_position == 7)										// Reset count if it increments past the last index of the buffer
				u8_position = 0;
			ESOS_TASK_WAIT_TICKS(250);								//Give up time on scheduler for other tasks.
		}
	ESOS_TASK_END();
}

/****************************************************
 *  user_init()
 ****************************************************
 */
void user_init(void) {

  // Call the hardware-provided routines to print the
  // HELLO_MSG to the screen.  Must use this call because
  // the ESOS communications subsystems is not yet fully
  // initialized, since this call is in user_init()
  //
  // In general, users should call hardware-specific
  // function like this.
  __esos_unsafe_PutString( HELLO_MSG );

#ifdef __linux
  // register our little ESOS task to mimic MCU's TIMER T1 IRQ which kicks off
  // the ESOS S/W timers when they expire
  esos_RegisterTask( __simulated_isr );
#endif

  // configure our hardware to support to support our application
  CONFIG_LED1();

  // configure ADC params for temperature sensor LM-60 on AN1
  CONFIG_TEMP_SENSOR();

  // user_init() should register at least one user task
  esos_RegisterTask(heartbeat_LED);
  esos_RegisterTask(send_temperature);
  esos_RegisterTask(read_external_temperature);

} // end user_init()

/*  Bubble sort function used to sort temperature aray values
 *  when the user requests the current temperature. Complexity O(n^2)
 */
void bubbleSort(uint16 au16_tempArray[], uint8 u8_sizeOfArray) {
      bool_t swapped = TRUE;
      uint8 u8_j = 0;
      uint8 u8_i;
      uint16 u16_tmp;
      while (swapped) {
            swapped = FALSE;
            u8_j++;
            for (u8_i = 0; u8_i < (u8_sizeOfArray - u8_j); u8_i++)
            {
                  if (au16_tempArray[u8_i] > au16_tempArray[u8_i + 1])
                  {
                        u16_tmp = au16_tempArray[u8_i];
                        au16_tempArray[u8_i] = au16_tempArray[u8_i + 1];
                        au16_tempArray[u8_i + 1] = u16_tmp;
                        swapped = TRUE;
                  }
            }
      }
}
