// Standard includes
#include <stdio.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "hw_apps_rcm.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"

// Common interface includes
#include "gpio_if.h"
#include "uart_if.h"

// Anaren includes
#include <HAL/hal.h>
#include <ZM/module.h>
#include <Common/utilities.h>
#include <ZM/module_errors.h>
#include <ZM/zm_phy.h>

// Pinmux generated include
#include "pinmux.h"

#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

// Access to the Anaren's SPI Buffer
extern uint8_t zmBuf[ZIGBEE_MODULE_BUFFER_SIZE];

static void BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

int main()
{
    moduleResult_t result = MODULE_SUCCESS;

    //
    // Initialize Board configurations
    //
    BoardInit();

    //
    // Power on the corresponding GPIO port B for 9,10,11.
    // Set up the GPIO lines to mode 0 (GPIO)
    //
    PinMuxConfig();
//    GPIO_IF_LedConfigure(LED1|LED2|LED3);

//    GPIO_IF_LedOff(MCU_ALL_LED_IND);

    InitTerm();
    
    Message("TEST [Get Version]\n\r");

    //
    // Enable the SPI module clock
    //
    Message("Initializing SPI Module clock...\n\r");
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);

    //
    // Reset the peripheral
    //
    Message("Resetting peripheral...\n\r");
    MAP_PRCMPeripheralReset(PRCM_GSPI);

    Message("Initializing module...\n\r");
    moduleInit();

    Message("Resetting module...\n\r");
    result = moduleReset();
    if (result == MODULE_SUCCESS)
    {
        /* Display the contents of the received SYS_RESET_IND message */
        displaySysResetInd();
    } else {
        Report("ERROR 0x%02X\r\n", result);
    }

    Message("Getting MAC address...\n\r");
//  while (1)
//  {
        result = zbGetDeviceInfo(DIP_MAC_ADDRESS);
        if (result == MODULE_SUCCESS)
        {
            uint8_t* mac = zmBuf+SRSP_DIP_VALUE_FIELD;
            Report("MAC (as sent, LSB first):");
            printHexBytes(mac, 8);

            /* Note: the MAC address comes over the wire in reverse order (LSB first)
            So we swap the order of the bytes so we can display it correctly. */
            uint8_t temp[8];
            int i;
            for (i=0; i<8; i++)
            {
                temp[i] = mac[7-i];
            }
            Report("MAC (correct, MSB first):");
            printHexBytes(temp, 8);
            printf("\r\n");
        }
        else
        {
        	Report("ERROR 0x%02X\r\n", result);
        }
//      delayMs(1000);
//  }
    Message("Done\n\r");
    while (1);
}
