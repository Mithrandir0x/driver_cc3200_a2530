#ifndef HAL_CC3200_H_
#define HAL_CC3200_H_

#include <stdint.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "pin.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "spi.h"
#include "gpio.h"
#include "utils.h"

#ifdef DISABLED_PRINTF
#define printf(fmt, args...) {}
#elif defined DEBUG_UART_PRINTF
extern int Report(const char *, ...);
#define printf Report
#endif

#define MAP_SysCtlClockGet          80000000
#define MILLISECONDS_TO_TICKS(ms)   ( (MAP_SysCtlClockGet/1000) * (ms) )
#define TICKS_PER_MS                MILLISECONDS_TO_TICKS(1)

#define PIN_SPI_CS   PIN_08
#define PIN_SPI_CLK  PIN_05
#define PIN_SPI_MISO PIN_06
#define PIN_SPI_MOSI PIN_07

#define RADIO_ON()  MAP_GPIOPinWrite(GPIOA3_BASE, 0x80, 0x80)
#define RADIO_OFF() MAP_GPIOPinWrite(GPIOA3_BASE, 0x80, 0x00)

//  Zigbee Module SPI
#define SPI_SS_SET()   MAP_SPICSDisable(GSPI_BASE)
#define SPI_SS_CLEAR() MAP_SPICSEnable(GSPI_BASE)
#define SRDY_IS_HIGH() ( MAP_GPIOPinRead(GPIOA1_BASE, 0x08) == 0x08 )
#define SRDY_IS_LOW()  ( MAP_GPIOPinRead(GPIOA1_BASE, 0x08) == 0x00 )

// HAL SPI integration
void halSpiInitModule();
void spiWrite(uint8_t *bytes, uint8_t numBytes);

// Utility methods
void delayMs(uint16_t delay);

#endif
