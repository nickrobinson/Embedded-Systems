

// Several bit mask need to be defined for use in u16_pwmFlags
#define PWM_PORT_OPENDRAIN 0x0001
#define PWM_PORT_CMOS 0x0002
#define PWM_PORT_CONT_PULSE 0x0004
#define PWM_PORT_DISTRIBUTED_PULSE 0x0100
#define PWM_OUTPUT_ON 0x8000
#define PWM_OUTPUT_OFF 0x0000


//**********************************************************
// pin configuration
//**********************************************************


//#define macros
//Macros


struct pwmStruct {
  uint16 *pu16_port;    //a pointer to the single MCU port that contains all of the HW interface pins
  uint16 u16_dataMask; //a mask denoting the LSb of the data pins (all data pins are contiguous)
  uint16 u16_pwmPeriod; //a mask denoting the location of the RS pin
  uint16 u16_pwmPulsewidth; //a mask denoting the location of the E pin
  uint8 u8_pwmDutycycle; //a mask denoting the location of the RW pin
  uint16 u16_pwmFlags; //the number of LCD row to display
} my_pwmStruct;

//**********************************************************
// setup only the hardware specific aspects of the PIC24 interface of the LCD
//**********************************************************
void esos_pwm_config( struct pwmStruct* pst_pwmStruct )
{
  pst_pwmStruct->pu16_port = &LATB;
  pst_pwmStruct->u16_dataMask = 0x0008;
  pst_pwmStruct->u16_pwmPeriod = 1000;
  pst_pwmStruct->u16_pwmPulsewidth = 10;
  pst_pwmStruct->u8_pwmDutycycle = 0.5;
  pst_pwmStruct->u16_pwmFlags = PWM_OUTPUT_ON | PWM_PORT_CONT_PULSE;
}

uint16 esos_pwm_getPeriod( struct pwmStruct* pst_pwmStruct )
{
    return pst_pwmStruct->u16_pwmPeriod;
}

uint16 esos_pwm_getPulsewidth( struct pwmStruct* pst_pwmStruct )
{
    return pst_pwmStruct->u16_pwmPulsewidth;
}

uint8 esos_pwm_getDutycyclceh( struct pwmStruct* pst_pwmStruct )
{
    return pst_pwmStruct->u8_pwmDutycycle;
}

void esos_pwm_setPeriod( struct pwmStruct* pst_pwmStruct , uint16 u16_period )
{
    pst_pwmStruct->u16_pwmPeriod = u16_period;
    pst_pwmStruct->u8_pwmDutycycle = (pst_pwmStruct->u16_pwmPulsewidth)/(pst_pwmStruct->u16_pwmPeriod);
}

void esos_pwm_setPulsewidth( struct pwmStruct* pst_pwmStruct, uint16 u16_pulsewidth )
{
    pst_pwmStruct->u16_pwmPulsewidth = u16_pulsewidth;
    pst_pwmStruct->u8_pwmDutycycle = (pst_pwmStruct->u16_pwmPulsewidth)/(pst_pwmStruct->u16_pwmPeriod);
}

void esos_pwm_setDutycycle( struct pwmStruct* pst_pwmStruct, uint8 u8_dutycycle )
{
    pst_pwmStruct->u8_pwmDutycycle = u8_dutycycle;
    pst_pwmStruct->u16_pwmPulsewidth = (pst_pwmStruct->u8_pwmDutycycle) * (pst_pwmStruct->u16_pwmPeriod);
}

void esos_pwm_setOn( struct pwmStruct* pst_pwmStruct )
{
    pst_pwmStruct->u16_pwmFlags = pst_pwmStruct->u16_pwmFlags | PWM_OUTPUT_ON;
}

void esos_pwm_setOff( struct pwmStruct* pst_pwmStruct )
{
    pst_pwmStruct->u16_pwmFlags = pst_pwmStruct->u16_pwmFlags & (~PWM_OUTPUT_ON);
}

void esos_pwm_setOpendrain( struct pwmStruct* pst_pwmStruct )
{
    pst_pwmStruct->u16_pwmFlags = pst_pwmStruct->u16_pwmFlags | PWM_PORT_OPENDRAIN;
    ODCB = ODCB | pst_pwmStruct->u16_pwmFlags;
}

void esos_pwm_setCmos( struct pwmStruct* pst_pwmStruct )
{
    pst_pwmStruct->u16_pwmFlags = pst_pwmStruct->u16_pwmFlags | PWM_PORT_OPENDRAIN;
}

void esos_pwm_setContinuousOutput( struct pwmStruct* pst_pwmStruct )
{
    pst_pwmStruct->u16_pwmFlags = pst_pwmStruct->u16_pwmFlags | PWM_PORT_CONT_PULSE;
}

void esos_pwm_setDistributedOutput( struct pwmStruct* pst_pwmStruct )
{
    static uint16 u16_timerPeriod;
    static uint16 u16_timerWidth;
    pst_pwmStruct->u16_pwmFlags = pst_pwmStruct->u16_pwmFlags & (~PWM_PORT_CONT_PULSE);
    u16_timerWidth = pst_pwmStruct->u16_pwmPulsewidth;
    u16_timerPeriod = pst_pwmStruct->u16_pwmPeriod;
}