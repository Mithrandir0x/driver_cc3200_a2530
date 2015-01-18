
#include "hal_cc3200.h"

// This defines the SPI clock rate to 4MHz.
#define SPI_IF_BIT_RATE  4000000

void delayMs(uint16_t delay)
{
	UtilsDelay(26667 * (unsigned long) delay); // Wait for a millisecond
}

void halSpiInitModule()
{
	// Reset SPI
	MAP_SPIReset(GSPI_BASE);

	// Configure SPI interface

	// The SPI Interface must be configured as follows:
	//   SPI master.
	//   Clock speed must be greater than 500kHz and less than 4 MHz.
	//   Clock polarity 0 and clock phase 0 on CC2530.
	//   Bit order MSB first.
	// (Extract from https://www.anaren.com/air-wiki-zigbee/Physical_Interface)

	MAP_SPIConfigSetExpClk(GSPI_BASE,
			MAP_PRCMPeripheralClockGet(PRCM_GSPI),
			SPI_IF_BIT_RATE,
			SPI_MODE_MASTER,
			SPI_SUB_MODE_0,
			( SPI_SW_CTRL_CS |   // Chip-Select Control Mode (Software or Hardware)
				SPI_4PIN_MODE |
				SPI_TURBO_OFF |
				SPI_CS_ACTIVEHIGH |
				SPI_WL_8));

	// Enable SPI for communication
	MAP_SPIEnable(GSPI_BASE);
}

void spiWrite(uint8_t *bytes, uint8_t numBytes)
{
	MAP_SPITransfer(GSPI_BASE, (unsigned char *) bytes, 0, numBytes, 0);
}
