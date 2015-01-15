#ifndef HAL_CC3200_H_
#define HAL_CC3200_H_

#include <stdint.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "spi.h"
#include "rom.h"
#include "rom_map.h"

void halSpiInitModule();

#endif
