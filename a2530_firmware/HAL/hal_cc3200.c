
#include "hal_cc3200.h"

#ifdef ZM_PHY_SPI_VERBOSE
#include "../Common/utilities.h"
#endif

// This defines the SPI clock rate to 4MHz.
#define SPI_IF_BIT_RATE  1000000

void delayMs(uint16_t delay)
{
	MAP_UtilsDelay(26667 * (unsigned long) delay); // Wait for a millisecond
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

	// SPI CS HAS TO BE ACTIVE LOW (From https://www.anaren.com/air-wiki-zigbee/Module_Hardware_Interface)

	MAP_SPIConfigSetExpClk(GSPI_BASE,
			MAP_PRCMPeripheralClockGet(PRCM_GSPI),
			SPI_IF_BIT_RATE,
			SPI_MODE_MASTER,
			SPI_SUB_MODE_0,
			( SPI_HW_CTRL_CS |   // Chip-Select Control Mode (Software or Hardware)
				SPI_4PIN_MODE |
				SPI_TURBO_OFF |
				SPI_CS_ACTIVELOW |
				SPI_WL_8));

	// Enable SPI for communication
	MAP_SPIEnable(GSPI_BASE);
}

void spiWrite(uint8_t *bytes, uint8_t numBytes)
{
	long result = MAP_SPITransfer(GSPI_BASE, (unsigned char *) bytes, (unsigned char *) bytes, numBytes, 0);
#ifdef ZM_PHY_SPI_VERBOSE
 	if ( result != 0 ) {
 		printf("Problem risen when trying to transfer bytes to SPI\n\r");
 	}
#endif
}
