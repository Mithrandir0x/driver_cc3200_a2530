
#include "hal_cc3200.h"

#define SPI_IF_BIT_RATE  100000

void halSpiInitModule()
{
	// Reset SPI
	MAP_SPIReset(GSPI_BASE);

	// Configure SPI interface
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
