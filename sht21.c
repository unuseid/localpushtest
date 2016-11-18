
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_i2c.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "driverlib/i2c.h"
#include "sensorlib/hw_sht21.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/sht21.h"

#include "../global.h"
#include "sht21.h"

extern uint32_t g_ui32SysClock;

#define SHT21_I2C_ADDRESS  0x40
#define SHT21_TEMP_NOBLOCK 0xF3

tI2CMInstance g_sI2CInst;
tSHT21 g_sSHT21Inst;
volatile uint_fast8_t g_vui8DataFlag;
volatile uint_fast8_t g_vui8ErrorFlag;

//*****************************************************************************
//
// SHT21 Sensor callback function.  Called at the end of SHT21 sensor driver
// transactions. This is called from I2C interrupt context. Therefore, we just
// set a flag and let main do the bulk of the computations and display.
//
//*****************************************************************************
void SHT21AppCallback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    // If the transaction succeeded set the data flag to indicate to
    // application that this transaction is complete and data may be ready.
    if(ui8Status == I2CM_STATUS_SUCCESS)
    {
        g_vui8DataFlag = 1;
    }

    // Store the most recent status in case it was an error condition
    g_vui8ErrorFlag = ui8Status;
}

//*****************************************************************************
//
// SHT21 Application error handler.
//
//*****************************************************************************
void SHT21AppErrorHandler(char *pcFilename, uint_fast32_t ui32Line)
{
    // Set terminal color to red and print error status and locations
    UARTprintf("Error: %d, File: %s, Line: %d\r\n", g_vui8ErrorFlag, pcFilename, ui32Line);
    UARTprintf("See I2C status definitions in mpulib\\i2cm_drv.h\r\n");

    // Go to sleep wait for interventions.  A more robust application could
    // attempt corrective actions here.
    while(1)
    {
    	MAP_SysCtlReset();
    }
}

//*****************************************************************************
//
// Called by the NVIC as a result of I2C3 Interrupt. I2C3 is the I2C connection
// to the SHT21.
//
//*****************************************************************************
void SHT21I2CIntHandler(void)
{
    // Pass through to the I2CM interrupt handler provided by sensor library.
    // This is required to be at application level so that I2CMIntHandler can
    // receive the instance structure pointer as an argument.
    I2CMIntHandler(&g_sI2CInst);
}

//*****************************************************************************
//
// Function to wait for the SHT21 transactions to complete.
//
//*****************************************************************************
void SHT21AppI2CWait(char *pcFilename, uint_fast32_t ui32Line)
{
    // Put the processor to sleep while we wait for the I2C driver to
    // indicate that the transaction is complete.
    while((g_vui8DataFlag == 0) && (g_vui8ErrorFlag == 0))
    {
    	// Do Nothing
    }

    // If an error occurred call the error handler immediately.
    if(g_vui8ErrorFlag)
    {
        SHT21AppErrorHandler(pcFilename, ui32Line);
    }

    // clear the data flag for next use.
    g_vui8DataFlag = 0;
}

//*****************************************************************************
//
//
//
//*****************************************************************************
void SHT_Configure(void)
{
    // Set I2C1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);

    // PG0-1 are used for I2C1 to the SHT21.
    ROM_GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    ROM_GPIOPinConfigure(GPIO_PB3_I2C0SDA);
    ROM_GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    ROM_GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

    // Initialize I2C1 peripheral.
    I2CMInit(&g_sI2CInst, I2C0_BASE, INT_I2C0, 0xff, 0xff, g_ui32SysClock);

    // Initialize the SHT21
    SHT21Init(&g_sSHT21Inst, &g_sI2CInst, SHT21_I2C_ADDRESS, SHT21AppCallback, &g_sSHT21Inst);

    // Wait for the I2C transactions to complete before moving forward
    SHT21AppI2CWait(__FILE__, __LINE__);
}

void SHT_Process(void)
{
    float fTemperature, fHumidity;
    //uint32_t g_ui32SysClock = 120000000;

    // Write the command to start a humidity measurement
    SHT21Write(&g_sSHT21Inst, SHT21_CMD_MEAS_RH, g_sSHT21Inst.pui8Data, 0, SHT21AppCallback, &g_sSHT21Inst);

    // Wait for the I2C transactions to complete before moving forward
    SHT21AppI2CWait(__FILE__, __LINE__);

    // Wait 33 milliseconds before attempting to get the result. Datasheet
    // claims this can take as long as 29 milliseconds
    MAP_SysCtlDelay(g_ui32SysClock / (30 * 3));

    // Get the raw data from the sensor over the I2C bus
    SHT21DataRead(&g_sSHT21Inst, SHT21AppCallback, &g_sSHT21Inst);

    // Wait for the I2C transactions to complete before moving forward
    SHT21AppI2CWait(__FILE__, __LINE__);

    // Get a copy of the most recent raw data in floating point format.
    SHT21DataHumidityGetFloat(&g_sSHT21Inst, &fHumidity);

    // Write the command to start a temperature measurement
    SHT21Write(&g_sSHT21Inst, SHT21_CMD_MEAS_T, g_sSHT21Inst.pui8Data, 0, SHT21AppCallback, &g_sSHT21Inst);

    // Wait for the I2C transactions to complete before moving forward
    SHT21AppI2CWait(__FILE__, __LINE__);

    // Wait 100 milliseconds before attempting to get the result. Datasheet
    // claims this can take as long as 85 milliseconds
    MAP_SysCtlDelay(g_ui32SysClock / (10 * 3));

    // Read the conversion data from the sensor over I2C.
    SHT21DataRead(&g_sSHT21Inst, SHT21AppCallback, &g_sSHT21Inst);

    // Wait for the I2C transactions to complete before moving forward
    SHT21AppI2CWait(__FILE__, __LINE__);

    // Get the most recent temperature result as a float in celcius.
    SHT21DataTemperatureGetFloat(&g_sSHT21Inst, &fTemperature);

    global.humidity = fHumidity;
    global.temperature = fTemperature;

#if 0
    int32_t i32IntegerPart;
    int32_t i32FractionPart;

    // Convert the floats to an integer part and fraction part for easy
    // print. Humidity is returned as 0.0 to 1.0 so multiply by 100 to get
    // percent humidity.
    fHumidity *= 100.0f;
    i32IntegerPart = (int32_t) fHumidity;
    i32FractionPart = (int32_t) (fHumidity * 1000.0f);
    i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
    if(i32FractionPart < 0)
    {
        i32FractionPart *= -1;
    }

    // Print the humidity value using the integers we just created
    UARTprintf("Humidity %3d.%03d\t", i32IntegerPart, i32FractionPart);

    // Perform the conversion from float to a printable set of integers
    i32IntegerPart = (int32_t) fTemperature;
    i32FractionPart = (int32_t) (fTemperature * 1000.0f);
    i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
    if(i32FractionPart < 0)
    {
        i32FractionPart *= -1;
    }

    // Print the temperature as integer and fraction parts.
    UARTprintf("Temperature %3d.%03d\r\n", i32IntegerPart, i32FractionPart);

    // Delay for one second. This is to keep sensor duty cycle
    // to about 10% as suggested in the datasheet, section 2.4.
    // This minimizes self heating effects and keeps reading more accurate.
    //MAP_SysCtlDelay(g_ui32SysClock / 3);
#endif
}


