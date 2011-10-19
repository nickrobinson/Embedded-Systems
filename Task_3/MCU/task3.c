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

/** \file
 * ESOS application program to demonstrate I2C mastering in ESOS.
 * Application recreates code in \ref ds1631_i2c.c in Figure 10.52.
 * (See Figure 10.49 in the text for circuit.)
 *
 * Application also has a flashing LED on RB15.  Flashing LED is generated
 * by an <em>software timer</em> calling a user-provided callback function.
 *
 *  \note Demonstrates child tasks, ESOS software timers, and I2C service
 */


// INCLUDEs go here  (First include the main esos.h file)
//      After that, the user can include what they need
#include    "esos.h"
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
#include    "esos_comm.h"
#include    "esos_pic24_rs232.h"
#include    "esos_pic24_i2c.h"
#include    <stdio.h>
#endif

// DEFINEs go here
#define DS1631ADDR 0x9F   //DS1631 address with all pins tied high
#define ACCESS_CONFIG 0xAC
#define CONFIG_COMMAND 0x0C //continuous conversion, 12-bit mode
#define START_CONVERT 0x51
#define READ_TEMP 0xAA

#ifndef __linux
#define   CONFIG_LED1()   CONFIG_RB15_AS_DIG_OUTPUT()
#define   LED1            _LATB15
#else
#define   CONFIG_LED1()   printf("called CONFIG_LED1()\n");
uint8_t     LED1 = TRUE;      // LED1 is initially "on"
#endif

// PROTOTYPEs go here
uint8_t hexToUnsigned(uint8_t u8_upperChar, uint8_t u8_lowerChar);

// GLOBALs go here
//  Generally, the user-created semaphores will be defined/allocated here
char psz_CRNL[3]= {0x0D, 0x0A, 0};
char psz_prompt[] = "Temp is  ";
char psz_done[9]= {' ','D','O','N','E','!',0x0D, 0x0A, 0};
int16_t i16_temp;
UINT16 U16_raw;

ESOS_SEMAPHORE(sem_dataReady);
ESOS_SEMAPHORE(sem_dataPrinted);
ESOS_SEMAPHORE(sem_ds1631Ready);

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

/*
 * An ESOS software timer callback function strobe the heartbeat LED.
 *
 * Toggles LED1 everytime the callback is called.  Exact period is
 * determined by application when this timer callback function is
 * registered with ESOS.  See \ref esos_RegisterTimer
 * Application can change timer period on-the-fly with \ref esos_ChangeTimerPeriod
 *
 * \note Since this heartbeat is performed in an ESOS software
 * timer callabck, a flashing LED indicates that the ESOS system
 * tick ISR is being called properly.  If the LED quits flashing,
 * then the ESOS system tick has ceased working.  This probably indicates
 * some catastrophic failure of the system.  However, the cause could
 * be poorly-behaved user code that is manipulating the hardware registers
 * with the timer or interrupt enables directly.  ESOS provides functions
 * to change state of interrupts and user code should never modify the
 * hardware used by ESOS to implement the system tick.
 * \hideinitializer
 */

// user-created timer callback
ESOS_USER_TIMER( swTimerLED ) {
  LED1 = !LED1;
#ifdef __linux
  if (LED1) {
    printf("\a");
    fflush(stdout);
  }
#endif
} //endof swTimerLED

/*
 * user task to setup DS1631 for continuous temperature
 * conversion.  Will signal when DS1631 is ready to be
 * used.
 */
ESOS_USER_TASK(start_ds1631) {
  ESOS_TASK_BEGIN();
  ESOS_TASK_WAIT_TICKS(500);
  ESOS_TASK_WAIT_ON_WRITE2I2C1(DS1631ADDR, ACCESS_CONFIG, CONFIG_COMMAND);
  ESOS_TASK_WAIT_ON_WRITE1I2C1(DS1631ADDR, START_CONVERT);
  ESOS_TASK_WAIT_TICKS(500);
  ESOS_SIGNAL_SEMAPHORE(sem_ds1631Ready, 1);
  ESOS_TASK_END();
} //end task start_ds1631

ESOS_CHILD_TASK(fast_read_child, uint8 u8_i2cAddress)
{
      static uint8_t u8_upperChar;
      static uint8_t u8_lowerChar;
      static uint8_t u8_bytesToRead;
      static uint8_t pau8_i2cData[8];
      //Increment write address by 1 so we have read address
      static uint8_t u8_i2cReadAddress;


      ESOS_TASK_BEGIN();
      //Read Address
      u8_i2cReadAddress = u8_i2cAddress + 1;
      //Get number of bytes to read
      ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
      ESOS_TASK_WAIT_ON_GET_UINT8(u8_upperChar);
      ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
      ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
      ESOS_TASK_WAIT_ON_GET_UINT8(u8_lowerChar);
      ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();

      u8_bytesToRead = hexToUnsigned(u8_upperChar, u8_lowerChar);
      ESOS_TASK_WAIT_ON_READNI2C1(u8_i2cReadAddress, pau8_i2cData, u8_bytesToRead);
      ESOS_TASK_WAIT_ON_AVAILABLE_OUT_COMM();
      ESOS_TASK_WAIT_ON_SEND_U8BUFFER(pau8_i2cData, u8_bytesToRead);
      ESOS_TASK_SIGNAL_AVAILABLE_OUT_COMM();

      ESOS_TASK_END();
}

ESOS_CHILD_TASK(fast_write_child, uint8 u8_i2cAddress)
{
    static uint8_t u8_upperChar;
    static uint8_t u8_lowerChar;
    static uint8_t u8_i2cData;

    ESOS_TASK_BEGIN();
    ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
    ESOS_TASK_WAIT_ON_GET_UINT8(u8_upperChar);
    ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
    ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
    ESOS_TASK_WAIT_ON_GET_UINT8(u8_lowerChar);
    ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();

    u8_i2cData = hexToUnsigned(u8_upperChar, u8_lowerChar);
    ESOS_TASK_WAIT_ON_WRITE1I2C1(u8_i2cAddress, u8_i2cData);
    ESOS_TASK_END();
}

ESOS_CHILD_TASK(wait_child)
{
    static uint16_t u16_delayTime;

    ESOS_TASK_BEGIN();
    ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
    ESOS_TASK_WAIT_ON_GET_UINT16(u16_delayTime);
    ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
    ESOS_TASK_WAIT_TICKS((uint32_t) u16_delayTime);
    ESOS_TASK_END();
}

ESOS_CHILD_TASK(echo_child)
{
    static char pau8_in[7];
    static uint8 i;

    ESOS_TASK_BEGIN();
    i = 0;
    ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
    ESOS_TASK_WAIT_ON_GET_STRING(pau8_in);
    ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();

    while(pau8_in[i])
    {
        ESOS_TASK_WAIT_ON_AVAILABLE_OUT_COMM();
        ESOS_TASK_WAIT_ON_SEND_UINT8(pau8_in[i]);
        ESOS_TASK_SIGNAL_AVAILABLE_OUT_COMM();
        i = i + 1;
    }
    
    ESOS_TASK_END();
}

/**
 * Respond to user command
 */
ESOS_USER_TASK(listenForCommand) {
  static uint8_t u8_command;
  static uint8_t u8_upperChar;
  static uint8_t u8_lowerChar;
  static uint8_t u8_bytesToRead;
  static uint8_t u8_i2cAddress = 0;
  static uint8_t u8_i2cData;
  static uint8_t i = 0;
  static ESOS_TASK_HANDLE th_child;

  ESOS_TASK_BEGIN();
  while (TRUE) {
      ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
      ESOS_TASK_WAIT_ON_GET_UINT8(u8_command);
      ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();

      if(u8_command == 'R')
      {
          ESOS_ALLOCATE_CHILD_TASK(th_child);
          ESOS_TASK_SPAWN_AND_WAIT(th_child, fast_read_child, u8_i2cAddress);
      }
      else if(u8_command == 'W')
      {
          ESOS_ALLOCATE_CHILD_TASK(th_child);
          ESOS_TASK_SPAWN_AND_WAIT(th_child, fast_write_child, u8_i2cAddress);
      }
      else if(u8_command == 'L')
      {
          ESOS_ALLOCATE_CHILD_TASK(th_child);
          ESOS_TASK_SPAWN_AND_WAIT(th_child, wait_child);
      }
      else if(u8_command == 'T')
      {
          ESOS_ALLOCATE_CHILD_TASK(th_child);
          ESOS_TASK_SPAWN_AND_WAIT(th_child, echo_child);
      }
      else if(u8_command == 'S')
      {
          //We have recieved the start command lets get the addres
          ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
          ESOS_TASK_WAIT_ON_GET_UINT8(u8_upperChar);
          ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
          ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
          ESOS_TASK_WAIT_ON_GET_UINT8(u8_lowerChar);
          ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();

          u8_i2cAddress = hexToUnsigned(u8_upperChar, u8_lowerChar);
          static uint8_t pau8_i2cData[8];

          for(i=0; i<=8; i++)
          {
            pau8_i2cData[i] = NULL;
          }

          //Check if address is Read(1) or Write(0)
          if((u8_i2cAddress & 0x01) == 1)
          {
              //Read Address
              //Get number of bytes to read
              ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
              ESOS_TASK_WAIT_ON_GET_UINT8(u8_upperChar);
              ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
              ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
              ESOS_TASK_WAIT_ON_GET_UINT8(u8_lowerChar);
              ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();

              u8_bytesToRead = hexToUnsigned(u8_upperChar, u8_lowerChar);
              ESOS_TASK_WAIT_ON_READNI2C1(u8_i2cAddress, pau8_i2cData, u8_bytesToRead);
              ESOS_TASK_WAIT_ON_AVAILABLE_OUT_COMM();
              ESOS_TASK_WAIT_ON_SEND_U8BUFFER(pau8_i2cData, u8_bytesToRead);
              ESOS_TASK_SIGNAL_AVAILABLE_OUT_COMM();
          } //if((u8_i2cAddress & 0x01) == 1)
          else
          {
              //Write Address
              //Since this is the write address we need to get data from the user
              i = 0;

              ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
              ESOS_TASK_WAIT_ON_GET_UINT8(u8_upperChar);
              ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();

              while(u8_upperChar != 'P' && u8_upperChar != 'S' && u8_upperChar != 'R'
                      && u8_upperChar != 'W' && u8_upperChar != 'L' && u8_upperChar != 'T')
              {
                  ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
                  ESOS_TASK_WAIT_ON_GET_UINT8(u8_lowerChar);
                  ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
                  
                  pau8_i2cData[i] = hexToUnsigned(u8_upperChar, u8_lowerChar);
                  i = i + 1;

                  //ESOS_TASK_WAIT_ON_WRITE1I2C1(u8_i2cAddress, u8_i2cData);

                  ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
                  ESOS_TASK_WAIT_ON_GET_UINT8(u8_upperChar);
                  ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
              }

              ESOS_TASK_WAIT_ON_WRITENI2C1(u8_i2cAddress, pau8_i2cData, i);

              if(u8_upperChar == 'R')
              {
                  ESOS_ALLOCATE_CHILD_TASK(th_child);
                  ESOS_TASK_SPAWN_AND_WAIT(th_child, fast_read_child, u8_i2cAddress);
              }
              else if(u8_upperChar == 'W')
              {
                  ESOS_ALLOCATE_CHILD_TASK(th_child);
                  ESOS_TASK_SPAWN_AND_WAIT(th_child, fast_write_child, u8_i2cAddress);
              }
              else if(u8_command == 'L')
              {
                  ESOS_ALLOCATE_CHILD_TASK(th_child);
                  ESOS_TASK_SPAWN_AND_WAIT(th_child, wait_child);
              }
              else if(u8_command == 'T')
              {
                  ESOS_ALLOCATE_CHILD_TASK(th_child);
                  ESOS_TASK_SPAWN_AND_WAIT(th_child, echo_child);
              }
          }//if((u8_i2cAddress & 0x01) != 1)
      } //else if(u8_command == 'S')
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
  esos_pic24_configI2C1(400);            //configure I2C for 400 KHz

  ESOS_INIT_SEMAPHORE(sem_ds1631Ready, 0);
  ESOS_INIT_SEMAPHORE(sem_dataReady, 0);
  ESOS_INIT_SEMAPHORE(sem_dataPrinted, 0);

  // user_init() should register at least one user task
  esos_RegisterTask(start_ds1631);
  esos_RegisterTask(listenForCommand);

  // register our callback function with ESOS to create a software timer
  esos_RegisterTimer( swTimerLED, 250);

} // end user_init()

uint8_t hexToUnsigned(uint8_t u8_upperChar, uint8_t u8_lowerChar)
{
     static uint8_t u8_converted;
     u8_upperChar = toUnsignedInt(u8_upperChar);
     u8_lowerChar = toUnsignedInt(u8_lowerChar);
     u8_converted = (16 * (uint8_t) u8_upperChar);
     u8_converted = u8_converted + (uint8_t) u8_lowerChar;
     return u8_converted;
}

void toUnsignedInt(uint8_t u8_character)
{
    if(u8_character == 'A')
        u8_character = 10;
    else if(u8_character == 'B')
        u8_character = 11;
    else if(u8_character == 'C')
        u8_character = 12;
    else if(u8_character == 'D')
        u8_character = 13;
    else if(u8_character == 'E')
        u8_character = 14;
    else if(u8_character == 'F')
        u8_character = 15;
    else if(u8_character <= '9' || u8_character >= '0')
        u8_character = u8_character - '0';
}