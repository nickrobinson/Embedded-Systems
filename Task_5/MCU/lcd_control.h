

// Several bit mask need to be de?ned for use in u16_lcdFlags
#define LCD_DATA_FOURBIT 0x0000
#define LCD_DATA_EIGHTBIT 0x8000
#define LCD_CHARSET_5X8 0x0000
#define LCD_CHARSET_5X11 0x0001
#define LCD_BLINK_ON 0x0004
#define LCD_BLINK_OFF 0x0000
#define LCD_DISPLAY_ON 0x0008
#define LCD_DISPLAY_OFF 0x0000
#define LCD_CURSOR_ON 0x0010
#define LCD_CURSOR_OFF 0x0000
// RESERVED bit masks 0x0020 through 0x400





//**********************************************************
// pin configuration
//**********************************************************
#define RS_HIGH()        _LATB12 = 1
#define RS_LOW()         _LATB12 = 0
#define CONFIG_RS()      CONFIG_RB12_AS_DIG_OUTPUT()

#define RW_HIGH()        _LATB13 = 1
#define RW_LOW()         _LATB13 = 0
#define CONFIG_RW()      CONFIG_RB13_AS_DIG_OUTPUT()

#define E_HIGH()         _LATB14 = 1
#define E_LOW()          _LATB14 = 0
#define CONFIG_E()       CONFIG_RB14_AS_DIG_OUTPUT()

#define LCD4O          _LATB5
#define LCD4I          _RB5
#define LCD5O          _LATB6
#define LCD5I          _RB6
#define LCD6O          _LATB7
#define LCD6I          _RB7
#define LCD7O          _LATB4
#define LCD7I          _RB4

#define CONFIG_LCD4_AS_INPUT() CONFIG_RB5_AS_DIG_INPUT()
#define CONFIG_LCD5_AS_INPUT() CONFIG_RB6_AS_DIG_INPUT()
#define CONFIG_LCD6_AS_INPUT() CONFIG_RB7_AS_DIG_INPUT()
#define CONFIG_LCD7_AS_INPUT() CONFIG_RB4_AS_DIG_INPUT()

#define CONFIG_LCD4_AS_OUTPUT() CONFIG_RB5_AS_DIG_OUTPUT()
#define CONFIG_LCD5_AS_OUTPUT() CONFIG_RB6_AS_DIG_OUTPUT()
#define CONFIG_LCD6_AS_OUTPUT() CONFIG_RB7_AS_DIG_OUTPUT()
#define CONFIG_LCD7_AS_OUTPUT() CONFIG_RB4_AS_DIG_OUTPUT()

#define GET_BUSY_FLAG()  LCD7I

//#define macros
//Macros

#define ESOS_TASK_WAIT_ON_LCD_WRITE(lcdStruct, u8_rs, u8_data)  \
    if((lcdStruct->u16_lcdFlags & LCD_DATA_EIGHTBIT) == 0x8000)   \
        ESOS_TASK_SPAWN_AND_WAIT((ESOS_TASK_HANDLE)&th_child, __esos_pic24_lcd_writeByte, (lcdStruct), (u8_rs), (u8_data))

#define ESOS_TASK_WAIT_ON_LCD_READ(lcdStruct, u8_rs, u8_data) \
    if((lcdStruct->u16_lcdFlags & LCD_DATA_EIGHTBIT) == 0x8000)   \
        ESOS_TASK_SPAWN_AND_WAIT((ESOS_TASK_HANDLE)&th_child, __esos_pic24_lcd_readByte, (lcdStruct), (u8_rs), (u8_data))

#define ESOS_TASK_WAIT_LCD_WRITE_COMMAND(lcdStruct, u8_cmd) \
    if((lcdStruct->u16_lcdFlags & LCD_DATA_EIGHTBIT) == 0x8000)   \
        ESOS_TASK_SPAWN_AND_WAIT((ESOS_TASK_HANDLE)&th_child, __esos_pic24_lcd_writeByte, (lcdStruct), 0, (u8_cmd))

#define ESOS_TASK_WAIT_LCD_SET_CG_ADDRESS(lcdStruct, u8_addr) \
	ESOS_TASK_SPAWN_AND_WAIT((ESOS_TASK_HANDLE)&th_child, __esos_lcd_setCharAddress, (lcdStruct), (u8_addr))

#define ESOS_TASK_WAIT_LCD_SET_DATA_ADDRESS(pst_lcdStruct, u8_addr) \
	ESOS_TASK_SPAWN_AND_WAIT((ESOS_TASK_HANDLE)&th_child, __esos_lcd_setDataAddress, (lcdStruct), (u8_addr))

#define ESOS_TASK_WAIT_LCD_WHILE_BUSY(lcdStruct) \
do{ \
    ESOS_TASK_SPAWN_AND_WAIT((ESOS_TASK_HANDLE)&th_child, __esos_lcd_readBusyFlagAndAddres, lcdStruct, pu8_bf, u8_dataRead); \
}while(*pu8_bf == 1) \

#define ESOS_TASK_WAIT_LCD_READ_ADDRESS(lcdStruct, u8_addr) \
        ESOS_TASK_SPAWN_AND_WAIT((ESOS_TASK_HANDLE)&th_child, __esos_lcd_readBusyFlagAndAddres, lcdStruct, pu8_bf, u8_addr); \

#define ESOS_TASK_WAIT_LCD_WRITE_DATA(lcdStruct, u8_data)   \
        ESOS_TASK_SPAWN_AND_WAIT((ESOS_TASK_HANDLE)&th_child, __esos_pic24_lcd_writeByte, (lcdStruct), 1, (u8_data))

#define ESOS_TASK_WAIT_LCD_READ_DATA(lcdStruct, u8_addr)    \
        ESOS_TASK_SPAWN_AND_WAIT((ESOS_TASK_HANDLE)&th_child, __esos_pic24_lcd_readByte, (lcdStruct), 1, (u8_data)


//Configure 4-bit data bus for output
void configBusAsOutLCD(void) {
  RW_LOW();                  //RW=0 to stop LCD from driving pins
  CONFIG_LCD4_AS_OUTPUT();   //D4
  CONFIG_LCD5_AS_OUTPUT();   //D5
  CONFIG_LCD6_AS_OUTPUT();   //D6
  CONFIG_LCD7_AS_OUTPUT();   //D7
}

//Configure 4-bit data bus for input
void configBusAsInLCD(void) {
  CONFIG_LCD4_AS_INPUT();   //D4
  CONFIG_LCD5_AS_INPUT();   //D5
  CONFIG_LCD6_AS_INPUT();   //D6
  CONFIG_LCD7_AS_INPUT();   //D7
  RW_HIGH();                   // R/W = 1, for read
}

//Configure the control lines for the LCD
void configControlLCD(void) {
  CONFIG_RS();     //RS
  CONFIG_RW();     //RW
  CONFIG_E();      //E
  RW_LOW();
  E_LOW();
  RS_LOW();
}

//Pulse the E clock, 1 us delay around edges for
//setup/hold times
void pulseE(void) {
  DELAY_MS(1);
  E_HIGH();
  DELAY_MS(1);
  E_LOW();
  DELAY_MS(1);
}

//Output lower 4-bits of u8_c to LCD data lines
void outputTo4BusLCD(uint8_t u8_c) {
  LCD4O = u8_c & 0x01;          //D4
  LCD5O = (u8_c >> 1)& 0x01;    //D5
  LCD6O = (u8_c >> 2)& 0x01;    //D6
  LCD7O = (u8_c >> 3)& 0x01;    //D7
}


/* Write a byte (u8_Cmd) to the LCD.
u8_DataFlag is '1' if data byte, '0' if command byte
u8_CheckBusy is '1' if must poll busy bit before write, else simply delay before write
u8_Send8Bits is '1' if must send all 8 bits, else send only upper 4-bits
*/
void writeLCD(uint8_t u8_Cmd, uint8_t u8_DataFlag,
              uint8_t u8_CheckBusy, uint8_t u8_Send8Bits) {

  uint8_t u8_BusyFlag;
  if (u8_CheckBusy) {
    RS_LOW();            //RS = 0 to check busy
    // check busy
    configBusAsInLCD();  //set data pins all inputs
    do {
      E_HIGH();
      DELAY_US(1);  // read upper 4 bits
      u8_BusyFlag = GET_BUSY_FLAG();
      E_LOW();
      DELAY_US(1);
      pulseE();              //pulse again for lower 4-bits
    } while (u8_BusyFlag);
  } else {
    DELAY_MS(10); // don't use busy, just delay
  }
  configBusAsOutLCD();
  if (u8_DataFlag)
  {
      RS_HIGH();   // RS=1, data byte
  }
  else    RS_LOW();             // RS=0, command byte
  outputTo4BusLCD(u8_Cmd >> 4);  // send upper 4 bits
  pulseE();
  if (u8_Send8Bits) {
    outputTo4BusLCD(u8_Cmd);     // send lower 4 bits
    pulseE();
  }
}

struct lcdStruct {
  uint16 *pu16_port;    //a pointer to the single MCU port that contains all of the HW interface pins
  uint16 u16_dataMask; //a mask denoting the LSb of the data pins (all data pins are contiguous)
  uint16 u16_rsMask; //a mask denoting the location of the RS pin
  uint16 u16_eMask; //a mask denoting the location of the E pin
  uint16 u16_rwMask; //a mask denoting the location of the RW pin
  uint8 u8_lcdRows; //the number of LCD row to display
  uint8 u8_lcdColumns; //the number of LCD characters per line
  uint16 u16_lcdFlags; //flags to denote the state of the LCD character module
  uint8 **__pu8_lcdBuffer; //private pointer to MCU memory to represent LCD frame buffer
} my_lcdStruct;

//**********************************************************
// setup only the hardware specific aspects of the PIC24 interface of the LCD
//**********************************************************
void esos_pic24_config_char_lcd( struct lcdStruct* pst_lcdStruct )
{
  pst_lcdStruct->pu16_port = &PORTB;
  pst_lcdStruct->u16_dataMask = 0x0002;
  pst_lcdStruct->u16_rsMask = 0x0200;
  pst_lcdStruct->u16_eMask = 0x4000;
  pst_lcdStruct->u16_rwMask = 0x2000;
  pst_lcdStruct->u8_lcdRows = 2;
  pst_lcdStruct->u8_lcdColumns = 24;
  pst_lcdStruct->u16_lcdFlags = LCD_DISPLAY_ON | LCD_DATA_EIGHTBIT;

  configControlLCD();
}

// Initialize the LCD, modify to suit your application and LCD
void esos_lcd_init( struct lcdStruct* pst_lcdStruct ) {
  DELAY_MS(50);          //wait for device to settle
  writeLCD(0x30,0,0,0); // 4 bit interface
  DELAY_MS(1);
  writeLCD(0x30,0,0,0); // 4 bit interface
  DELAY_MS(1);
  writeLCD(0x20,0,0,0); // 4 bit interface
  writeLCD(0x28,0,1,1); // 2 line display, 5x7 font
  writeLCD(0x08,0,1,1); // 2 line display, 5x7 font
  writeLCD(0x01,0,1,1); // clear display, move cursor to home
  writeLCD(0x06,0,1,1); // enable display
  writeLCD(0x0C,0,1,1); // turn display on; cursor, blink is off
  DELAY_MS(3);
}

void esos_lcd_configDisplay(struct lcdStruct* pst_lcdStruct)
{
    uint8 u8_i = 0;
    uint8 u8_numOfRows = pst_lcdStruct->u8_lcdRows;

    pst_lcdStruct->__pu8_lcdBuffer = (uint8**) malloc(u8_numOfRows*sizeof(uint8*));

    for(u8_i = 0; u8_i < u8_numOfRows; u8_i++)
        pst_lcdStruct->__pu8_lcdBuffer[u8_i] = (uint8 *) malloc((pst_lcdStruct->u8_lcdColumns) * sizeof(uint8));
}

//Output a string to the LCD
void outStringLCD(char *psz_s) {
  while (*psz_s) {
    writeLCD(*psz_s, 1, 1, 1);
    psz_s++;
  }
}

void esos_lcd_clearScreen(struct lcdStruct* pst_lcdStruct)
{
    static uint8 u8_i;
    static uint8 u8_j;
    static uint8 u8_numOfRows;
    static uint8 u8_numOfCols;

    //Move cursor to home
    writeLCD(0x02,0,1,1);

    u8_numOfRows = pst_lcdStruct->u8_lcdRows;
    u8_numOfCols = pst_lcdStruct->u8_lcdColumns;

    for(u8_i = 0; u8_i < u8_numOfRows; u8_i++){
        for(u8_j = 0; u8_j < u8_numOfCols; u8_j++){
            pst_lcdStruct->__pu8_lcdBuffer[u8_i][u8_j] = ' ';
        }
    }

}

esos_lcd_setCursorHome(struct lcdStruct* pst_lcdStruct) {
	writeLCD(0x02,0,1,1); //set cursor to home without clearing display
}

esos_lcd_setCursor(struct lcdStruct* pst_lcdStruct, uint8 u8_row, uint8 u8_col) {

	static uint8 u8_DDRAM_Address;

	if(u8_row == 1)
	{
		u8_DDRAM_Address = u8_col | 0x80;   //D7 must be 1
		writeLCD(u8_DDRAM_Address,0,1,1);
	}
	else if (u8_row == 2)
	{
		u8_DDRAM_Address = u8_col | 0x80;   //D7 must be 1
		writeLCD((u8_DDRAM_Address+0x40),0,1,1);
	}
	else if (pst_lcdStruct->u8_lcdRows == 4 & pst_lcdStruct->u8_lcdColumns == 16)
	{
		if(u8_row == 3)
		{
			u8_DDRAM_Address = u8_col | 0x80;   //D7 must be 1
			writeLCD((u8_DDRAM_Address+0x10),0,1,1);
		}
		else if(u8_row == 4)
		{
			u8_DDRAM_Address = u8_col | 0x80;   //D7 must be 1
			writeLCD((u8_DDRAM_Address+0x50),0,1,1);
		}
	}
	else if (pst_lcdStruct->u8_lcdRows == 4 & pst_lcdStruct->u8_lcdColumns == 20)
	{
		if(u8_row == 3)
		{
			u8_DDRAM_Address = u8_col | 0x80;   //D7 must be 1
			writeLCD((u8_DDRAM_Address+0x14),0,1,1);
		}
		else if(u8_row == 4)
		{
			u8_DDRAM_Address = u8_col | 0x80;   //D7 must be 1
			writeLCD((u8_DDRAM_Address+0x54),0,1,1);
		}
	}
}

esos_lcd_setCursorDisplay(struct lcdStruct* pst_lcdStruct, uint8 u8_state) {
	if(u8_state == TRUE)
	{
		pst_lcdStruct->u16_lcdFlags = pst_lcdStruct->u16_lcdFlags | LCD_CURSOR_ON;
		writeLCD(0x0E,0,1,1); //turn on cursor
	}
	else
	{
		pst_lcdStruct->u16_lcdFlags = pst_lcdStruct->u16_lcdFlags & !LCD_CURSOR_ON;
		writeLCD(0x0B,0,1,1); //turn off cursor
	}
}

esos_lcd_getCursorDisplay(struct lcdStruct* pst_lcdStruct, uint8 u8_state) {
	if(pst_lcdStruct->u16_lcdFlags & LCD_CURSOR_ON)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

esos_lcd_setCursorBlink(struct lcdStruct* pst_lcdStruct, uint8 u8_state) {
	if(u8_state == TRUE)
	{
		pst_lcdStruct->u16_lcdFlags = pst_lcdStruct->u16_lcdFlags | LCD_BLINK_ON;
		writeLCD(0x0F,0,1,1); //turn on cursor blink
	}
	else
	{
		pst_lcdStruct->u16_lcdFlags = pst_lcdStruct->u16_lcdFlags & !LCD_BLINK_ON;
		writeLCD(0x0E,0,1,1); //turn off cursor blink
	}
}

esos_lcd_getCursorBlink(struct lcdStruct* pst_lcdStruct, uint8 u8_state) {
	if(pst_lcdStruct->u16_lcdFlags & LCD_BLINK_ON)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void esos_lcd_writeChar(struct lcdStruct* pst_lcdStruct,uint8 u8_row,uint8 u8_column,uint8 u8_data) {

	// Write data to LCD
	pst_lcdStruct->__pu8_lcdBuffer[(u8_row - 1)][(u8_column - 1)] = u8_data;

}

void esos_lcd_writeBuffer(struct lcdStruct* pst_lcdStruct,uint8 u8_row,uint8 u8_column,char *psz_s,uint8 u8_bufflen ) {

	static uint8 u8_numOfRows = 0;
	static uint8 u8_numOfCols = 0;
	static uint8 u8_bufferPosition = 0;
	static uint8 u8_currentRow = 0;
	static uint8 u8_currentCol = 0;

	u8_numOfRows = pst_lcdStruct->u8_lcdRows;
        u8_numOfCols = pst_lcdStruct->u8_lcdColumns;
	u8_currentRow = u8_row;
	u8_currentCol = u8_column;
        u8_bufferPosition = 0;

	while(u8_bufferPosition <  u8_bufflen) {
		if (u8_currentCol > u8_numOfCols) {
			u8_currentCol = 1;
			u8_currentRow++;
		}
		if (u8_currentRow > u8_numOfRows) {
			u8_currentRow = 1;
		}

		pst_lcdStruct->__pu8_lcdBuffer[u8_currentRow - 1][u8_currentCol - 1] = *psz_s;

		u8_currentCol++;
		u8_bufferPosition++;
		psz_s++;

	}
}


uint8 esos_lcd_getChar( struct lcdStruct* pst_lcdStruct, uint8 u8_row, uint8 u8_column)
{
    return pst_lcdStruct->__pu8_lcdBuffer[u8_row-1][u8_column-1];
}


void esos_lcd_getBuffer( struct lcdStruct* pst_lcdStruct, uint8 u8_row, uint8 u8_column, uint8* pu8_data, uint8 u8_bufflen)
{
    static uint8 u8_i = 0;
    static uint8 u8_Col = 0;

    u8_Col = u8_column;

    while((u8_i < u8_bufflen) && (u8_Col < pst_lcdStruct->u8_lcdColumns))
    {
        *pu8_data = pst_lcdStruct->__pu8_lcdBuffer[u8_row][u8_Col];
        u8_Col++;
        pu8_data++;
    }
}


//**********************************************************
// The two write functions will write the data in u8_data to the LCD character module hardware
// described by pst_lcdStruct using the RS pin state given in u8_rs (viewed as a boolean
//**********************************************************
//NOTE: make sure the data in u8_data is in upper 4 bits
ESOS_CHILD_TASK( __esos_pic24_lcd_writeNibble, struct lcdStruct* pst_lcdStruct, uint8 u8_rs, uint8 u8_data ) {


  static uint8 u8_BusyFlag;

  ESOS_TASK_BEGIN();

  RS_LOW();            //RS = 0 to check busy
  configBusAsInLCD();  //set data pins all inputs

  do {
    E_HIGH();
    DELAY_US(1);  // read upper 4 bits
    u8_BusyFlag = GET_BUSY_FLAG();
    E_LOW();
    DELAY_US(1);
    pulseE();              //pulse again for lower 4-bits
  } while (u8_BusyFlag);

  configBusAsOutLCD();
  if (u8_rs) RS_HIGH();   // RS=1, data byte
  else    RS_LOW();             // RS=0, command byte
  outputTo4BusLCD(u8_data >> 4);  // send upper 4 bits
  pulseE();

  ESOS_TASK_END();
}

ESOS_CHILD_TASK( __esos_pic24_lcd_writeByte, struct lcdStruct* pst_lcdStruct, uint8 u8_rs, uint8 u8_data ) {

  static uint8 u8_BusyFlag;

  ESOS_TASK_BEGIN();

  RS_LOW();            //RS = 0 to check busy
  configBusAsInLCD();  //set data pins all inputs

  do {
    E_HIGH();
    DELAY_US(1);  // read upper 4 bits
    u8_BusyFlag = GET_BUSY_FLAG();
    E_LOW();
    DELAY_US(1);
    pulseE();              //pulse again for lower 4-bits
  } while (u8_BusyFlag);

  configBusAsOutLCD();
  if (u8_rs) RS_HIGH();   // RS=1, data byte
  else    RS_LOW();             // RS=0, command byte
  outputTo4BusLCD(u8_data >> 4);  // send upper 4 bits
  pulseE();
  outputTo4BusLCD(u8_data);  // send lower 4 bits
  pulseE();

  ESOS_TASK_END();
}


//**********************************************************
// The two read functions will read from the LCD character module hardware described by pst_lcdStruct
// using the RS pin state given in u8_rs (viewed as a boolean) and place the result into the location
// u8_data
//**********************************************************
ESOS_CHILD_TASK( __esos_pic24_lcd_readNibble, struct lcdStruct* pst_lcdStruct, uint8 u8_rs, uint8 u8_data ) {
  static uint8 u8_BusyFlag;

  ESOS_TASK_BEGIN();

  RS_LOW();            //RS = 0 to check busy
  configBusAsInLCD();  //set data pins all inputs

  do {
    E_HIGH();
    DELAY_US(1);  // read upper 4 bits
    u8_BusyFlag = GET_BUSY_FLAG();
    E_LOW();
    DELAY_US(1);
    pulseE();              //pulse again for lower 4-bits
  } while (u8_BusyFlag);

  RS_HIGH();

  E_HIGH();
  DELAY_US(1);  // read upper 4 bits
  u8_data = (LCD7I << 7) + (LCD6I << 6) + (LCD5I << 5) + (LCD4I << 4);
  E_LOW();
  DELAY_US(1);
  pulseE();

  RS_LOW();

  ESOS_TASK_END();
}

ESOS_CHILD_TASK( __esos_pic24_lcd_readByte, struct lcdStruct* pst_lcdStruct, uint8 u8_rs, uint8* pu8_data ) {
  static uint8 u8_BusyFlag;
  static uint8 u8_upperNibble;
  static uint8 u8_lowerNibble;

  ESOS_TASK_BEGIN();

  RS_LOW();            //RS = 0 to check busy
  configBusAsInLCD();  //set data pins all inputs

  do {
    E_HIGH();
    DELAY_US(1);  // read upper 4 bits
    u8_BusyFlag = GET_BUSY_FLAG();
    E_LOW();
    DELAY_US(1);
    pulseE();              //pulse again for lower 4-bits
  } while (u8_BusyFlag);

  RS_HIGH();

  E_HIGH();
  DELAY_US(1);  // read upper 4 bits
  u8_upperNibble = (LCD7I << 7) + (LCD6I << 6) + (LCD5I << 5) + (LCD4I << 4);
  E_LOW();
  DELAY_US(1);
  pulseE();

  u8_lowerNibble = (LCD7I << 7) + (LCD6I << 6) + (LCD5I << 5) + (LCD4I << 4);

  *pu8_data = (u8_upperNibble << 4) + u8_lowerNibble;

  RS_LOW();

  ESOS_TASK_END();
}

ESOS_CHILD_TASK(__esos_lcd_readBusyFlagAndAddres, struct lcdStruct* pst_lcdStruct, uint8* pu8_bf, uint8* pu8_address )
{
    static uint8 u8_upperNibble;
    static uint8 u8_lowerNibble;

    ESOS_TASK_BEGIN();

    RS_LOW();            //RS = 0 to check busy
    configBusAsInLCD();  //set data pins all inputs

    E_HIGH();
    DELAY_US(1);  // read upper 4 bits
    *pu8_bf = GET_BUSY_FLAG();
    u8_upperNibble = ((LCD6I << 6) + (LCD5I << 5) + (LCD4I << 4));
    E_LOW();
    DELAY_US(1);
    pulseE();              //pulse again for lower 4-bits
    u8_lowerNibble = (LCD7I << 3) + (LCD6I << 2) + (LCD5I << 1) + LCD4I;

    *pu8_address = (u8_upperNibble << 4) + u8_lowerNibble;

    ESOS_TASK_END();
}


//**********************************************************
// These routines will set the data and character addresses in the desired LCD, respectively.
//**********************************************************
ESOS_CHILD_TASK(__esos_lcd_setDataAddress, struct lcdStruct* pst_lcdStruct, uint8 u8_loc)
{
    static uint8 u8_DDRAM_Address;

    ESOS_TASK_BEGIN();

    u8_DDRAM_Address = u8_loc | 0x80;   //D7 must be 1

    writeLCD(u8_DDRAM_Address,0,1,1);

    ESOS_TASK_END();
}

ESOS_CHILD_TASK(__esos_lcd_setCharAddress, struct lcdStruct* pst_lcdStruct, uint8 u8_loc)
{
    static uint8 u8_CGRAM_Address;

    ESOS_TASK_BEGIN();

    u8_CGRAM_Address = u8_loc & 0x7F;   //D7 must be 0
    u8_CGRAM_Address = u8_CGRAM_Address | 0x40;     //D6 must be 1

    writeLCD(u8_CGRAM_Address,0,1,1);

    ESOS_TASK_END();
}

