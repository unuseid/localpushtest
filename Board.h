/*
================================================================================
Function : Operation for SI446x
================================================================================
*/
#ifndef _BOARD_H_
#define _BOARD_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include "drivers/spi0.h"


/*Data type definations*/
typedef uint8_t  INT8U;
typedef int8_t   INT8S;
typedef uint16_t INT16U;
typedef int16_t  INT16S;
typedef uint32_t INT32U;
typedef int32_t  INT32S;
typedef enum
{
	BOOL_FALSE = 0,
	BOOL_TRUE  = 1
} BOOLEAN;

/*Bit field operations*/
//#define SetBit( Byte, Bit )  ( Byte ) |= ( 1<<( Bit ) )
//#define ClrBit( Byte, Bit )  ( Byte ) &= ~( 1<<( Bit ) )
#define GetBit( Byte, Bit )  ( ( Byte ) & ( 1<<( Bit ) ) )
#define ComBit( Byte, Bit )  ( Bytes ) ^= ( 1<<( Bit ) )
#define SetBits( Byte, Bits ) ( Byte ) |= ( Bits )
#define ClrBits( Byte, Bits ) ( Byte ) &= ~( Bits )
#define GetBits( Byte, Bits ) ( ( Byte ) & ( Bits ) )
#define ComBits( Byte, Bits ) ( Byte ) ^= ( Bits )

/*Bit map for fast calculation*/
#define BitMap( x )   ( 1<<( x ) )
/*Get MAX or MIN value*/
#define GetMax( x1, x2 ) ( ( x1 ) > ( x2 ) ? ( x1 ) : ( x2 ) )
#define GetMin( x1, x2 ) ( ( x1 ) > ( x2 ) ? ( x2 ) : ( x1 ) )

/*Exchange a byte via the SPI bus*/
INT8U SPI_ExchangeByte( INT8U input );

/*Initialize the SPI bus*/
void Si4463_SPI_INIT( void );

/*Initialize the other GPIOs of the board*/
void Si4463_GPIO_INIT( void );

#define SI4463_IRQ_PORT	GPIO_PORTG_BASE
#define SI4463_IRQ_PIN	GPIO_PIN_0

#define SI4463_SDN_PORT	GPIO_PORTG_BASE
#define SI4463_SDN_PIN	GPIO_PIN_1

#define SI4463_CTS_PORT	GPIO_PORTG_BASE
#define SI4463_CTS_PIN	GPIO_PIN_3

#endif //_BOARD_H_
/*
=================================================================================
------------------------------------End of FILE----------------------------------
=================================================================================
*/
