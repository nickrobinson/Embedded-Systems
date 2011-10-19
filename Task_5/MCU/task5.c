#include 	"esos.h"
#include    "esos_pic24.h"
#include 	"pic24_all.h"
#include	"pic24_adc.h"
#include    "esos_pic24_rs232.h"
#include    "lcd_control.h"
#include    "pwm_control.h"
#include    "esos_pic24_i2c.h"

// DEFINEs go here
#define DS1631ADDR 0x9F   //DS1631 address with all pins tied high
#define ACCESS_CONFIG 0xAC
#define CONFIG_COMMAND 0x0C //continuous conversion, 12-bit mode
#define START_CONVERT 0x51
#define READ_TEMP 0xAA

#define PRESCALER 256
#define LED_HIGH()         _LATB3 = 1
#define LED_LOW()          _LATB3 = 0

#define   CONFIG_TEMP_SENSOR()  CONFIG_AN1_AS_ANALOG()
#define   AUTO_SAMPLE_TIME      31
#define   USE_12BIT_ENCODING    1

// PROTOTYPEs go here
uint8_t hexToUnsigned(uint8_t u8_upperChar, uint8_t u8_lowerChar);
uint16_t hexToUnsigned32(uint16_t u16_upperBytes, uint16_t u16_lowerBytes);
void bubbleSort(uint16 [], uint8);
uint16 getPort(uint16_t u16_character);

uint16 u16_tempExtMax = 174;                    //For LM60 -- Maximum Default Exterior Temperature
uint16 u16_tempExtMin = 1205;                   //For LM60 -- Maximum Default Exterior Temperature
uint16  au16_tempBuffer[7];             //For LM60 -- Exterior Temperature Buffer. Holds up to seven temp readings
static const uint16  u16_sizeOfArray = 7;
static const uint8   u8_middleOfArray = 3;
static uint8 u8_ledFlag = 1;
static uint8 u8_thermometerFunctionFlag = 0;

static struct lcdStruct* pst_lcdStruct = &my_lcdStruct;
static struct pwmStruct* pst_pwmStruct = &my_pwmStruct;

ESOS_SEMAPHORE(sem_ds1631Ready);


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

void configTimer2(void) {
  T2CON = T2_OFF | T2_IDLE_CON | T2_GATE_OFF
          | T2_SOURCE_INT
          | T2_PS_1_256 ;  //1 tick = 1.6 us at FCY=40 MHz
  PR2 = MS_TO_TICKS(100, PRESCALER) - 1;
  TMR2  = 0;       //clear timer3 value
  ESOS_MARK_PIC24_USER_INTERRUPT_SERVICED(ESOS_IRQ_PIC24_T2);
  //Note: PID will be performed at the interrupt level. Thus, it is given the lowest interrupt priority so that
  //other interrupt tasks that need to be executed will still get priority over the PID function.
  ESOS_REGISTER_PIC24_USER_INTERRUPT( ESOS_IRQ_PIC24_T2, ESOS_USER_IRQ_LEVEL4, _T2Interrupt);
  ESOS_ENABLE_PIC24_USER_INTERRUPT(ESOS_IRQ_PIC24_T2);
  T2CONbits.TON = 1;    //Turn timer on
}

ESOS_USER_TASK(thermometerTask)
{
    static uint16 u16_lm60Temp;

    ESOS_TASK_BEGIN();

    ESOS_TASK_WAIT_UNTIL(u8_thermometerFunctionFlag == 1);

    while(1)
    {
        if(u8_thermometerFunctionFlag == 1)
        {
            u16_lm60Temp = au16_tempBuffer[u8_middleOfArray];

            u16_lm60Temp = u16_lm60Temp - 0x0350;

            if(u16_lm60Temp < 0x0005)
            {
                _LATB1 = 0;
                _LATB2 = 0;
                _LATB3 = 0;
                pst_pwmStruct->u16_dataMask = 0x0001;
                esos_pwm_setDutycycle(pst_pwmStruct, 25);
            }
            else if(u16_lm60Temp >= 0x0005 && u16_lm60Temp < 0x000F)
            {
                _LATB1 = 0;
                _LATB2 = 0;
                _LATB3 = 0;
                pst_pwmStruct->u16_dataMask = 0x0001;
                esos_pwm_setDutycycle(pst_pwmStruct, 50);
            }
            else if(u16_lm60Temp >= 0x000F && u16_lm60Temp < 0x001F)
            {
                _LATB1 = 0;
                _LATB2 = 0;
                _LATB3 = 0;
                pst_pwmStruct->u16_dataMask = 0x0001;
                esos_pwm_setDutycycle(pst_pwmStruct, 75);
            }
            else if(u16_lm60Temp >= 0x001F && u16_lm60Temp < 0x002F)
            {
                _LATB1 = 0;
                _LATB2 = 0;
                _LATB3 = 0;
                pst_pwmStruct->u16_dataMask = 0x0001;
                esos_pwm_setDutycycle(pst_pwmStruct, 98);
            }
            else if(u16_lm60Temp >= 0x002F && u16_lm60Temp < 0x003F)
            {
                pst_pwmStruct->u16_dataMask = 0x0002;
                _LATB0 = 1;
                _LATB2 = 0;
                _LATB3 = 0;
                esos_pwm_setDutycycle(pst_pwmStruct, 25);
            }
            else if(u16_lm60Temp >= 0x003F && u16_lm60Temp < 0x004F)
            {
                pst_pwmStruct->u16_dataMask = 0x0002;
                _LATB0 = 1;
                _LATB2 = 0;
                _LATB3 = 0;
                esos_pwm_setDutycycle(pst_pwmStruct, 50);
            }
            else if(u16_lm60Temp >= 0x004F && u16_lm60Temp < 0x005F)
            {
                pst_pwmStruct->u16_dataMask = 0x0002;
                _LATB0 = 1;
                _LATB2 = 0;
                _LATB3 = 0;
                esos_pwm_setDutycycle(pst_pwmStruct, 75);
            }
            else if(u16_lm60Temp >= 0x005F && u16_lm60Temp < 0x006F)
            {
                pst_pwmStruct->u16_dataMask = 0x0002;
                _LATB0 = 1;
                _LATB2 = 0;
                _LATB3 = 0;
                esos_pwm_setDutycycle(pst_pwmStruct, 98);
            }
            else if(u16_lm60Temp >= 0x006F && u16_lm60Temp < 0x007F)
            {
                pst_pwmStruct->u16_dataMask = 0x0004;
                _LATB0 = 1;
                _LATB1 = 1;
                _LATB3 = 0;
                esos_pwm_setDutycycle(pst_pwmStruct, 25);
            }
            else if(u16_lm60Temp >= 0x007F && u16_lm60Temp < 0x008F)
            {
                pst_pwmStruct->u16_dataMask = 0x0004;
                _LATB0 = 1;
                _LATB1 = 1;
                _LATB3 = 0;
                esos_pwm_setDutycycle(pst_pwmStruct, 50);
            }
            else if(u16_lm60Temp >= 0x008F && u16_lm60Temp < 0x009F)
            {
                pst_pwmStruct->u16_dataMask = 0x0004;
                _LATB0 = 1;
                _LATB1 = 1;
                _LATB3 = 0;
                esos_pwm_setDutycycle(pst_pwmStruct, 75);
            }
            else if(u16_lm60Temp >= 0x009F && u16_lm60Temp < 0x00AF)
            {
                pst_pwmStruct->u16_dataMask = 0x0004;
                _LATB0 = 1;
                _LATB1 = 1;
                _LATB3 = 0;
                esos_pwm_setDutycycle(pst_pwmStruct, 98);
            }
            else if(u16_lm60Temp >= 0x00AF && u16_lm60Temp < 0x00BF)
            {
                pst_pwmStruct->u16_dataMask = 0x0008;
                _LATB0 = 1;
                _LATB1 = 1;
                _LATB2 = 1;
                esos_pwm_setDutycycle(pst_pwmStruct, 25);
            }
            else if(u16_lm60Temp >= 0x00BF && u16_lm60Temp < 0x00CF)
            {
                pst_pwmStruct->u16_dataMask = 0x0008;
                _LATB0 = 1;
                _LATB1 = 1;
                _LATB2 = 1;
                esos_pwm_setDutycycle(pst_pwmStruct, 50);
            }
            else if(u16_lm60Temp >= 0x00CF && u16_lm60Temp < 0x00DF)
            {
                pst_pwmStruct->u16_dataMask = 0x0008;
                _LATB0 = 1;
                _LATB1 = 1;
                _LATB2 = 1;
                esos_pwm_setDutycycle(pst_pwmStruct, 75);
            }
            else if(u16_lm60Temp >= 0x00DF)
            {
                pst_pwmStruct->u16_dataMask = 0x0008;
                _LATB0 = 1;
                _LATB1 = 1;
                _LATB2 = 1;
                esos_pwm_setDutycycle(pst_pwmStruct, 98);
            }

            ESOS_TASK_YIELD();
        }
        else
        {
            ESOS_TASK_WAIT_UNTIL(u8_thermometerFunctionFlag == 1);
        }

       // if(!(pst_pwmStruct->u16_pwmFlags & PWM_PORT_CONT_PULSE)) //If distributed pulse call setDistPulse to reset timer
       // {
        //    esos_pwm_setDistributedOutput(pst_pwmStruct);
       // }
        
    }

    ESOS_TASK_END();
}

ESOS_USER_TASK(pwmServiceRoutine)
{
    static uint8 u8_command;
    static uint8 u8_dataRecieved;
    static uint32 u32_pulse;
    static uint8 u8_upperChar;
    static uint8 u8_lowerChar;

    ESOS_TASK_BEGIN();

    while(1)
    {
        ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
        ESOS_TASK_WAIT_ON_GET_UINT8(u8_command);
        ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();

        if(u8_command == 'P')
        {
            ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
            ESOS_TASK_WAIT_ON_GET_UINT32(u32_pulse);
            ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
            esos_pwm_setPeriod(pst_pwmStruct, hexToUnsigned32(u32_pulse , (u32_pulse >> 16)));
        }
        else if(u8_command == 'Q')
        {
            ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
            ESOS_TASK_WAIT_ON_GET_UINT32(u32_pulse);
            ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
            esos_pwm_setPulsewidth(pst_pwmStruct, hexToUnsigned32(u32_pulse , (u32_pulse >> 16)));
        }
        else if(u8_command == 'R')
        {
            ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
            ESOS_TASK_WAIT_ON_GET_UINT8(u8_upperChar);
            ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
            ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
            ESOS_TASK_WAIT_ON_GET_UINT8(u8_lowerChar);
            ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
            esos_pwm_setDutycycle(pst_pwmStruct, (uint8) hexToUnsigned(u8_upperChar, u8_lowerChar));
        }
        else if(u8_command == 'S')
        {
            ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
            ESOS_TASK_WAIT_ON_GET_UINT8(u8_dataRecieved);
            ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
            if(u8_dataRecieved)
                esos_pwm_setCmos(pst_pwmStruct);
            else
                esos_pwm_setOpendrain(pst_pwmStruct);
        }
        else if(u8_command == 'T')
        {
            ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
            ESOS_TASK_WAIT_ON_GET_UINT8(u8_dataRecieved);
            ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
            if(u8_dataRecieved != '0')
                esos_pwm_setOn(pst_pwmStruct);
            else
                esos_pwm_setOff(pst_pwmStruct);
        }
        else if(u8_command == 'M')
        {
            static uint16 u16_portMaskRecieved;
            ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
            ESOS_TASK_WAIT_ON_GET_UINT8(u8_dataRecieved);
            ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
            u8_dataRecieved = toUnsignedInt(u8_dataRecieved);
            u16_portMaskRecieved = getPort(u8_dataRecieved);
            pst_pwmStruct->u16_dataMask = pst_pwmStruct->u16_dataMask | u16_portMaskRecieved;
        }
        else if(u8_command == 'U')
        {
            ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
            ESOS_TASK_WAIT_ON_GET_UINT8(u8_dataRecieved);
            ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
            if(u8_dataRecieved != '0')
                esos_pwm_setContinuousOutput(pst_pwmStruct);
            else
                esos_pwm_setDistributedOutput(pst_pwmStruct);
        }
        else if(u8_command == 'Z')
        {
            ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
            ESOS_TASK_WAIT_ON_GET_UINT8(u8_dataRecieved);
            ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
            if(u8_dataRecieved == '0'){
                u8_thermometerFunctionFlag = 1;
            }
            else
            {
                u8_thermometerFunctionFlag = 0;
            }
        }
    }
    ESOS_TASK_END();
}


ESOS_USER_TASK(lcdServiceTask)
{
    static uint8 u8_numOfRows = 0;
    static uint8 u8_numOfCols = 0;
    static uint8 u8_i = 0;
    static uint8 u8_j = 0;
    static char u8_testData = 0;
    static ESOS_TASK_HANDLE th_child;

    ESOS_TASK_BEGIN();

    u8_numOfRows = pst_lcdStruct->u8_lcdRows;
    u8_numOfCols = pst_lcdStruct->u8_lcdColumns;

    esos_lcd_clearScreen(pst_lcdStruct);

    while(1){
        for(u8_i = 0; u8_i < u8_numOfRows; u8_i++){
            for(u8_j = 0; u8_j < u8_numOfCols; u8_j++){
                ESOS_TASK_WAIT_LCD_WRITE_DATA(pst_lcdStruct, pst_lcdStruct->__pu8_lcdBuffer[u8_i][u8_j]);
            }

            esos_lcd_setCursor(pst_lcdStruct, u8_i + 2, 0);
        }
        esos_lcd_setCursorHome(pst_lcdStruct);
        ESOS_TASK_YIELD();
    }

    ESOS_TASK_END();
}

ESOS_USER_TASK(read_external_temperature){
    static uint16 u16_temperature;
    static uint8 u8_position = 0;
    static uint8 u8_upperBit = 0;
    static uint8 u8_lowerBit = 0;
    static char* u8_outString[6];
    static ESOS_TASK_HANDLE th_child;

    ESOS_TASK_BEGIN();
    while(1)
    {
            configADC1_ManualCH0( ADC_CH0_POS_SAMPLEA_AN1, AUTO_SAMPLE_TIME,
                    USE_12BIT_ENCODING );
            u16_temperature = convertADC1();                                                //Read the voltage from the ADC
            if(u16_temperature>u16_tempExtMax)                                                      //If value is higher than max then replace old max.
                    u16_tempExtMax = u16_temperature;
            else if(u16_temperature<u16_tempExtMin)                                         //If value is lower than min then replace old min.
                    u16_tempExtMin = u16_temperature;

            au16_tempBuffer[u8_position] = u16_temperature;                         //Add current reading to the buffer
            u8_position++;
            if(u8_position == 7)                                                                            // Reset count if it increments past the last index of the buffer
                    u8_position = 0;
            ESOS_TASK_WAIT_TICKS(250);                                                              //Give up time on scheduler for other tasks.
            bubbleSort(au16_tempBuffer, u16_sizeOfArray);
            u16_temperature = au16_tempBuffer[u8_middleOfArray];     //Store middle element
            u8_upperBit = u16_temperature >> 8;
            u8_lowerBit = u16_temperature;
            esos_lcd_writeBuffer(pst_lcdStruct, 1, 1, "LM60 Temp: ", 11);

            //Format output temp string
            u8_outString[0] = '0';
            u8_outString[1] = 'x';
            u8_outString[2] = __esos_u8_GetMSBHexCharFromUint8(u8_upperBit);
            u8_outString[3] = __esos_u8_GetLSBHexCharFromUint8(u8_upperBit);
            u8_outString[4] = __esos_u8_GetMSBHexCharFromUint8(u8_lowerBit);
            u8_outString[5] = __esos_u8_GetLSBHexCharFromUint8(u8_lowerBit);

            //Print output string to buffer
            esos_lcd_writeChar(pst_lcdStruct, 1, 12, u8_outString[0]);
            esos_lcd_writeChar(pst_lcdStruct, 1, 13, u8_outString[1]);
            esos_lcd_writeChar(pst_lcdStruct, 1, 14, u8_outString[2]);
            esos_lcd_writeChar(pst_lcdStruct, 1, 15, u8_outString[3]);
            esos_lcd_writeChar(pst_lcdStruct, 1, 16, u8_outString[4]);
            esos_lcd_writeChar(pst_lcdStruct, 1, 17, u8_outString[5]);
            ESOS_TASK_WAIT_TICKS(250);
    }
    ESOS_TASK_END();
}

ESOS_USER_TASK(read_ds1631)
{
    static uint8 u8_bytesToRead = 2;
    static uint8_t u8_upperByte;
    static uint8_t u8_lowerByte;
    static char* u8_outString[6];

    ESOS_TASK_BEGIN();

    ESOS_TASK_WAIT_SEMAPHORE(sem_ds1631Ready, 1);

    //Loop to get temp from DS1631 and print temp to buffer
    while(1){

        esos_lcd_writeBuffer(pst_lcdStruct, 2, 1, "DS1631 Temp: ", 13);
        
        ESOS_TASK_WAIT_ON_WRITE1I2C1(DS1631ADDR, READ_TEMP);
        ESOS_TASK_WAIT_ON_READ2I2C1(DS1631ADDR, u8_upperByte, u8_lowerByte);


        //Format output temp string
        u8_outString[0] = '0';
        u8_outString[1] = 'x';
        u8_outString[2] = __esos_u8_GetMSBHexCharFromUint8(u8_upperByte);
        u8_outString[3] = __esos_u8_GetLSBHexCharFromUint8(u8_upperByte);
        u8_outString[4] = __esos_u8_GetMSBHexCharFromUint8(u8_lowerByte);
        u8_outString[5] = __esos_u8_GetLSBHexCharFromUint8(u8_lowerByte);

        //Print output string to buffer
        esos_lcd_writeChar(pst_lcdStruct, 2, 14, u8_outString[0]);
        esos_lcd_writeChar(pst_lcdStruct, 2, 15, u8_outString[1]);
        esos_lcd_writeChar(pst_lcdStruct, 2, 16, u8_outString[2]);
        esos_lcd_writeChar(pst_lcdStruct, 2, 17, u8_outString[3]);
        esos_lcd_writeChar(pst_lcdStruct, 2, 18, u8_outString[4]);
        esos_lcd_writeChar(pst_lcdStruct, 2, 19, u8_outString[5]);

    }
    ESOS_TASK_END();
}

//In order for PID (and accurate speed tracking on GUI), the encoders need to be sampled at a regular interval. Achieved
//with timer interrupt every 100 ms.
ESOS_USER_INTERRUPT( ESOS_IRQ_PIC24_T2 ) {

        ESOS_MARK_PIC24_USER_INTERRUPT_SERVICED(ESOS_IRQ_PIC24_T2);

        if(pst_pwmStruct->u16_pwmFlags & PWM_OUTPUT_ON)
        {
            if(pst_pwmStruct->u16_pwmFlags & PWM_PORT_CONT_PULSE)
            {
                if(u8_ledFlag == 1)
                {
                    PR2 = (pst_pwmStruct->u16_pwmPulsewidth/.0016);
                    u8_ledFlag = 0;
                    *pst_pwmStruct->pu16_port = *pst_pwmStruct->pu16_port | pst_pwmStruct->u16_dataMask;
                }
                else
                {
                    PR2 = (((pst_pwmStruct->u16_pwmPeriod) - (pst_pwmStruct->u16_pwmPulsewidth))/.0016);
                    u8_ledFlag = 1;
                    *pst_pwmStruct->pu16_port = *pst_pwmStruct->pu16_port & (~pst_pwmStruct->u16_dataMask);
                }
            }
            else //Distributed Pulse
            {
                if(u8_ledFlag == 1)
                {
                    u8_ledFlag = 0;
                    //Set led high for 1ms
                    PR2 = (1/.0016);
                    *pst_pwmStruct->pu16_port = *pst_pwmStruct->pu16_port | pst_pwmStruct->u16_dataMask;
                }
                else
                {
                    u8_ledFlag = 1;
                    //Set LED's low for (period/width)-1
                     PR2 = ((((float)(pst_pwmStruct->u16_pwmPeriod) / (float)(pst_pwmStruct->u16_pwmPulsewidth)) - 1)/.0016);
                    *pst_pwmStruct->pu16_port = *pst_pwmStruct->pu16_port & (~pst_pwmStruct->u16_dataMask);
                }
            }
        }
}

void user_init(void) {
	__esos_unsafe_PutString(HELLO_MSG);

        esos_pwm_config(pst_pwmStruct);
        esos_pwm_setContinuousOutput(pst_pwmStruct);
        esos_thermometer_config(pst_pwmStruct);
        configTimer2();

        CONFIG_RB0_AS_DIG_OUTPUT();
        CONFIG_RB1_AS_DIG_OUTPUT();
        CONFIG_RB2_AS_DIG_OUTPUT();
        CONFIG_RB3_AS_DIG_OUTPUT();

        esos_pic24_configI2C1(400);            //configure I2C for 400 KHz
        esos_pic24_config_char_lcd(pst_lcdStruct);

        esos_lcd_configDisplay(pst_lcdStruct);
        esos_lcd_init(pst_lcdStruct);

        esos_RegisterTask(lcdServiceTask);
	esos_RegisterTask(start_ds1631);
        esos_RegisterTask(read_ds1631);
        esos_RegisterTask(pwmServiceRoutine);
        esos_RegisterTask(thermometerTask);
        esos_RegisterTask(read_external_temperature);
}

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

uint8_t hexToUnsigned(uint8_t u8_upperChar, uint8_t u8_lowerChar)
{
     static uint8_t u8_converted;
     u8_upperChar = toUnsignedInt(u8_upperChar);
     u8_lowerChar = toUnsignedInt(u8_lowerChar);
     u8_converted = (16 * (uint8_t) u8_upperChar);
     u8_converted = u8_converted + (uint8_t) u8_lowerChar;
     return u8_converted;
}

uint16_t hexToUnsigned32(uint16_t u16_upperBytes, uint16_t u16_lowerBytes)
{
     static uint16_t u16_converted;
     static uint8_t u8_upperChar1;
     static uint8_t u8_upperChar2;
     static uint8_t u8_lowerChar1;
     static uint8_t u8_lowerChar2;

     u8_upperChar1 = u16_upperBytes;
     u8_lowerChar1 = u16_upperBytes >> 8;
     u8_upperChar2 = u16_lowerBytes;
     u8_lowerChar2 = u16_lowerBytes >> 8;

     u8_upperChar1 = toUnsignedInt(u8_upperChar1);
     u8_lowerChar1 = toUnsignedInt(u8_lowerChar1);
     u8_upperChar2 = toUnsignedInt(u8_upperChar2);
     u8_lowerChar2 = toUnsignedInt(u8_lowerChar2);
     u16_converted = (4096 * (uint16_t) u8_upperChar1);
     u16_converted = u16_converted + (256 * (uint16_t) u8_lowerChar1);
     u16_converted = u16_converted + (16 * (uint16_t) u8_upperChar2);
     u16_converted = u16_converted + (uint16_t) u8_lowerChar2;
     return u16_converted;
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

uint16  getPort(uint16_t u16_character)
{
    if(u16_character == 0xF)
        u16_character = 0x8000;
    else if(u16_character == 0xE)
        u16_character = 0x4000;
    else if(u16_character == 0xD)
        u16_character = 0x2000;
    else if(u16_character == 0xC)
        u16_character = 0x1000;
    else if(u16_character == 0xB)
        u16_character = 0x0800;
    else if(u16_character == 0xA)
        u16_character = 0x0400;
    else if(u16_character == 0x9)
        u16_character = 0x0200;
    else if(u16_character == 0x8)
        u16_character = 0x0100;
    else if(u16_character == 0x7)
        u16_character = 0x0080;
    else if(u16_character == 0x6)
        u16_character = 0x0040;
    else if(u16_character == 0x5)
        u16_character = 0x0020;
    else if(u16_character == 0x4)
        u16_character = 0x0010;
    else if(u16_character == 0x3)
        u16_character = 0x0008;
    else if(u16_character == 0x2)
        u16_character = 0x0004;
    else if(u16_character == 0x1)
        u16_character = 0x0002;
    else if(u16_character == 0x0)
        u16_character = 0x0001;

    return u16_character;
}