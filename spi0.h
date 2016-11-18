/*
 * spi0.h
 *
 *  Created on: 2016. 7. 1.
 *      Author: ryan
 */

#ifndef HEMOSU_R4_01_MODULE_DRIVERS_SPI0_H_
#define HEMOSU_R4_01_MODULE_DRIVERS_SPI0_H_

//*****************************************************************************
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

void SPI0_init(uint32_t speed);
void SPI0_nss_set(void);
void SPI0_nss_clear(void);
uint8_t SPI0_transfer(uint8_t data);

//*****************************************************************************
// Mark the end of the C bindings section for C++ compilers.
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif /* HEMOSU_R4_01_MODULE_DRIVERS_SPI0_H_ */
