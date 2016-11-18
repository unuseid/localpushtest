/*
================================================================================
Function : Operation for SI446x
================================================================================
*/
#include "board.h"


/*
=================================================================================
SPI_ExchangeByte( );
Function : Exchange a byte via the SPI bus
INTPUT   : input, The input byte
OUTPUT   : The output byte from SPI bus
=================================================================================
*/
INT8U SPI_ExchangeByte( INT8U input )
{
	return SPI0_transfer(input);
}
/*
=================================================================================
SPI_Initial( );
Function : Initialize the SPI bus
INTPUT   : None
OUTPUT   : None
=================================================================================
*/
void Si4463_SPI_INIT( void )
{
	SPI0_init(8000000);
}
/*
=================================================================================
GPIO_Initial( );
Function : Initialize the other GPIOs of the board
INTPUT   : None
OUTPUT   : None
=================================================================================
*/
void Si4463_GPIO_INIT( void )
{
    GPIOPinTypeGPIOOutput(SI4463_SDN_PORT, SI4463_SDN_PIN);
    GPIOPinTypeGPIOInput(SI4463_CTS_PORT, SI4463_CTS_PIN);
}

/*
=================================================================================
------------------------------------End of FILE----------------------------------
=================================================================================
*/
