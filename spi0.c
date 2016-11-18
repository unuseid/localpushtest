/*
 * spi0.c
 *
 *  Created on: 2016. 7. 1.
 *      Author: ryan
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/ssi.h"

#include "utils/uartstdio.h"

#include "drivers/spi0.h"

#define SPI0_NSS_PORT			GPIO_PORTA_BASE
#define SPI0_NSS_PIN			GPIO_PIN_3

/* must be supplied by the application */
extern uint32_t g_ui32SysClock;
#define DELAY_US(n)	SysCtlDelay(g_ui32SysClock/1000000 * n)

void SPI0_init(uint32_t speed)
{
	// initialize SPI pins
    ROM_GPIOPinTypeGPIOOutput(SPI0_NSS_PORT, SPI0_NSS_PIN);	// set NSEL pin to output
    ROM_GPIOPinWrite(SPI0_NSS_PORT, SPI0_NSS_PIN, SPI0_NSS_PIN);	// turn off chip select

    // The SSI0 peripheral must be enabled for use.
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);

    // PA2-5 are used for SSI0.
    ROM_GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    //ROM_GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    ROM_GPIOPinConfigure(GPIO_PA4_SSI0XDAT0);
    ROM_GPIOPinConfigure(GPIO_PA5_SSI0XDAT1);

    // Configure the GPIO settings for the SSI pins.
    ROM_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5);

    // Set the SSI output pins to 4MA drive strength and engage the pull-up on the receive line.
    MAP_GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPU);

    // Configure and enable the SSI0 port for SPI master mode.
    ROM_SSIConfigSetExpClk(SSI0_BASE, g_ui32SysClock, SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, speed, 8);
    ROM_SSIEnable(SSI0_BASE);

    // Clear rx fifo
    uint32_t tmp;
    while(SSIDataGetNonBlocking(SSI0_BASE, &tmp)) {}
}

void SPI0_nss_set(void)
{
	ROM_GPIOPinWrite(SPI0_NSS_PORT, SPI0_NSS_PIN, SPI0_NSS_PIN);
}

void SPI0_nss_clear(void)
{
	ROM_GPIOPinWrite(SPI0_NSS_PORT, SPI0_NSS_PIN, 0);
}


uint8_t SPI0_transfer(uint8_t data)
{
	uint32_t temp;
    SSIDataPut(SSI0_BASE, data);
    SSIDataGet(SSI0_BASE, &temp);
    return (uint8_t)temp;
}

