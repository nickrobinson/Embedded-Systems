#include 	"esos.h"
#include    "esos_pic24.h"
#include 	"pic24_all.h"
#include	"pic24_adc.h"
#include    "esos_pic24_rs232.h"
#include	"lcd_control.h"
#include    "esos_pic24_i2c.h"

// DEFINEs go here
#define DS1631ADDR 0x9F   //DS1631 address with all pins tied high
#define ACCESS_CONFIG 0xAC
#define CONFIG_COMMAND 0x0C //continuous conversion, 12-bit mode
#define START_CONVERT 0x51
#define READ_TEMP 0xAA

#define   CONFIG_TEMP_SENSOR()  CONFIG_AN1_AS_ANALOG()
#define   AUTO_SAMPLE_TIME      31
#define   USE_12BIT_ENCODING    1

uint16 u16_tempExtMax = 174;                    //For LM60 -- Maximum Default Exterior Temperature
uint16 u16_tempExtMin = 1205;                   //For LM60 -- Maximum Default Exterior Temperature
uint16  au16_tempBuffer[7];             //For LM60 -- Exterior Temperature Buffer. Holds up to seven temp readings
static const uint16  u16_sizeOfArray = 7;
static const uint8   u8_middleOfArray = 3;

static struct lcdStruct* pst_lcdStruct = &my_lcdStruct;

ESOS_SEMAPHORE(sem_ds1631Ready);

// PROTOTYPEs go here
void bubbleSort(uint16 [], uint8);


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
    static uint8 u8_upperByte;
    static uint8 u8_lowerByte;
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
        ESOS_TASK_WAIT_TICKS(100);

    }
    ESOS_TASK_END();
}
void user_init(void) {
	__esos_unsafe_PutString(HELLO_MSG);

        esos_pic24_configI2C1(400);            //configure I2C for 400 KHz
        esos_pic24_config_char_lcd(pst_lcdStruct);

        esos_lcd_configDisplay(pst_lcdStruct);
        esos_lcd_init(pst_lcdStruct);

        esos_RegisterTask(lcdServiceTask);
	esos_RegisterTask(start_ds1631);
        esos_RegisterTask(read_ds1631);
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

