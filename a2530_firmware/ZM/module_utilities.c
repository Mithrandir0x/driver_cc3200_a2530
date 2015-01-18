/**
* @file module_utilities.c
*
* @brief Simple utilities for interfacing with the Module. 
*
* $Rev: 1642 $
* $Author: dsmith $
* $Date: 2012-10-31 11:17:04 -0700 (Wed, 31 Oct 2012) $
*
* @section support Support
* Please refer to the wiki at www.anaren.com/air-wiki-zigbee for more information. Additional support
* is available via email at the following addresses:
* - Questions on how to use the product: AIR@anaren.com
* - Feature requests, comments, and improvements:  featurerequests@teslacontrols.com
* - Consulting engagements: sales@teslacontrols.com
*
* @section license License
* Copyright (c) 2012 Tesla Controls. All rights reserved. This Software may only be used with an 
* Anaren A2530E24AZ1, A2530E24CZ1, A2530R24AZ1, or A2530R24CZ1 module. Redistribution and use in 
* source and binary forms, with or without modification, are subject to the Software License 
* Agreement in the file "anaren_eula.txt"
* 
* YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS” 
* WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY 
* WARRANTY OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO 
* EVENT SHALL ANAREN MICROWAVE OR TESLA CONTROLS BE LIABLE OR OBLIGATED UNDER CONTRACT, NEGLIGENCE, 
* STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR 
* INDIRECT DAMAGES OR EXPENSE INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, 
* PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF SUBSTITUTE 
* GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY 
* DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
*/

#include "../HAL/hal.h"
#include "module.h"
#include "af.h"
#include "zdo.h"
#include "module_errors.h"
#include "module_utilities.h"
#include "zm_phy.h"
#include "../Common/utilities.h"
#include <stddef.h>

extern unsigned char zmBuf[ZIGBEE_MODULE_BUFFER_SIZE];

 /** Default configuration for a standard coordinator. Modify in application as needed. */
const struct moduleConfiguration DEFAULT_MODULE_CONFIGURATION_COORDINATOR = {
COORDINATOR,
DEFAULT_CHANNEL_MASK,
ANY_PAN,
DEFAULT_POLL_RATE_MS,
DEFAULT_STARTUP_OPTIONS,
SECURITY_MODE_OFF,
NULL
};

 /** Default configuration for a standard router. Modify in application as needed. */
const struct moduleConfiguration DEFAULT_MODULE_CONFIGURATION_ROUTER = {
ROUTER,
DEFAULT_CHANNEL_MASK,
ANY_PAN,
DEFAULT_POLL_RATE_MS,
DEFAULT_STARTUP_OPTIONS,
SECURITY_MODE_OFF,
NULL
};

 /** Default configuration for a standard end device. Modify in application as needed. */
const struct moduleConfiguration DEFAULT_MODULE_CONFIGURATION_END_DEVICE = {
END_DEVICE,
DEFAULT_CHANNEL_MASK,
ANY_PAN,
DEFAULT_POLL_RATE_MS,
DEFAULT_STARTUP_OPTIONS,
SECURITY_MODE_OFF,
NULL
};

  /** How often to poll the module in milliseconds to see if the desired deviceState has been received */
#define WFDS_POLL_INTERVAL_MS   100

#define METHOD_WAIT_FOR_DEVICE_STATE              0x6000
/** 
Private method used to wait until a message is received indicating that we are on the network. 
Exits if received message is a ZDO_STATE_CHANGE_IND and the state matches what we want. 
Else loops until timeout. 
@note Since this is basically a blocking wait, you can also implement this in your application.
@param expectedState the deviceState we are expecting - DEV_ZB_COORD etc.
@param timeoutMs the amount of milliseconds to wait before returning an error. Should be an integer
multiple of WFDS_POLL_INTERVAL_MS.
@todo modify this if using UART.
*/
moduleResult_t waitForDeviceState(unsigned char expectedState, uint16_t timeoutMs)
{
  RETURN_INVALID_PARAMETER_IF_TRUE( ((!(IS_VALID_DEVICE_STATE(expectedState))) || (timeoutMs < WFDS_POLL_INTERVAL_MS)), METHOD_WAIT_FOR_DEVICE_STATE);
  
  uint16_t intervals = timeoutMs / WFDS_POLL_INTERVAL_MS;                   // how many times to check
  uint8_t state = 0xFF;

  while (intervals--)
  {
    if (moduleHasMessageWaiting())                                          // If there's a message waiting for us
    {
      getMessage();

      if (CONVERT_TO_INT(zmBuf[2], zmBuf[1]) == ZDO_STATE_CHANGE_IND)       // if it's a state change message
      {
        state = zmBuf[SRSP_PAYLOAD_START];
        printf("%s, ", getDeviceStateName(state));                          // display the name of the state in the message
        if (state == expectedState)                                         // if it's the state we're expecting
          return MODULE_SUCCESS;                                                //Then we're done!
      } //else we received a different type of message so we just ignore it
    }
    delayMs(WFDS_POLL_INTERVAL_MS);
  }
  // We've completed the loop without receiving the sate that we want; so therefore we've timed out.
  RETURN_RESULT(TIMEOUT, METHOD_WAIT_FOR_DEVICE_STATE);
}


/** Error value returned from getDeviceStateForDeviceType() if the deviceType is not recognized */
#define INVALID_DEVICETYPE 33

/** 
Private method to get the expected deviceState (in ZDO_STATE_CHANGE_IND) that corresponds to this deviceType. 
@return the device state: DEV_ROUTER, DEV_END_DEVICE, or DEV_ZB_COORD accordingly, or INVALID_DEVICETYPE.
*/
uint8_t getDeviceStateForDeviceType(uint8_t deviceType)
{
  switch (deviceType)
  {
  case ROUTER: 
    return DEV_ROUTER;
  case END_DEVICE: 
    return DEV_END_DEVICE;
  case COORDINATOR: 
    return DEV_ZB_COORD;
  default: 
    return INVALID_DEVICETYPE;
  }
}

#define MODULE_REGION_NORTH_AMERICA         0x0001
#define MODULE_REGION_EUROPE                0x0002

/* Build ID is used for the module's configuration (antenna vs. connector, range extender or not, etc.) 
The Least Significant Nybble (e.g. 7 of the byte 0x67) is the Module Type. The other Nybble is undefined.
This information is hardcoded into the module firmware based on the module part number. */
#define MINIMUM_BUILD_ID                    0x10
#define MODULE_TYPE_MASK                    0x0F    
#define MAXIMUM_MODULE_TYPE                 0x05
#define MAX_OLD_BUILD_ID                    0x1F        //Everything below this is an old build
#define IS_VALID_MODULE_TYPE(x)             ((x>=0x20) && (x<=0x24))
#define BUILD_ID_FW_2_4_0                   0x7E


#define METHOD_SET_MODULE_RF_POWER          0x6200
/**
Configure Module RF output power to meet FCC certification limits. This depends on the module
configuration (whether it has a range extender or not) and the region of the world where the module 
is used. The region of the world is set with DIP switches on the LaunchPad.
@param productId an identifier indicating which firmware version is loaded
@note the method sysSetTxPower() only works with module firmware 2.5.1 and later. If an earlier
firmware is detected then the method will exit.
@pre productId > MINIMUM_BUILD_ID. It may still be invalid, though (e.g. 0x1E, which is not supported)
@note version 2.5.1 release 1 module firmware builds are configured as the following productIds:
- 0x10: No PA/LNA
- 0x11: Not Used
- 0x12: CC2590
- 0x13: CC2591
@note version 2.5.1 release 2 module firmware builds and RF power levels are as follows. 
The first number ("actual") is what is measured over the air.
The second number ("radio") is the value to use when calling sysSetTxPower.
The third number (in hex) is the corresponding PA_table value.
 - 0x20: No PA/LNA / A2530R24A  US:+4 actual/+3radio (0xF5); Europe:+4 actual/+3radio(0xF5)
 - 0x21: No PA/LNA / A2530R24C  US:+4 actual/+3radio (0xF5); Europe:+4 actual/+3radio(0xF5)
 - 0x22: CC2590                 US:+10 actual/+10radio(0xE5); Europe:+10 actual/+10radio(0xE5) Note: will never see in field
 - 0x23: CC2591/A2530E24A       US:+15 actual/+20radio(0xE5); Europe:+8 actual/+14radio(0x95)
 - 0x24: CC2591/A2530E24C       US:+13 actual/+18radio(0xC5); Europe:+8 actual/+14radio(0x95)   
@note version 2.4.0 firmware builds have a productId of 0x7E
@see setModuleRfPower() in simple_api.c
*/
static moduleResult_t setModuleRfPower(uint8_t productId, uint16_t moduleConfiguration)
{
#ifdef MODULE_UTILITIES_VERBOSE    
    printf("productId = %02X, moduleConfiguration = %04X\r\n", productId, moduleConfiguration);
#endif
    if ((productId == BUILD_ID_FW_2_4_0) || (!(IS_VALID_MODULE_TYPE(productId))))
    {
        printf("Using Default RF Setting\r\n");
        return MODULE_SUCCESS;
    }
                                           //0xX0, 0xX1, 0xX2, 0xX3, 0xX4
    // US PA_TABLE Settings                 {0xF5, 0xF5, 0xE5, 0xE5, 0xC5};    
    // EUROPE PA_TABLE Settings             {0xF5, 0xF5, 0xE5, 0x95, 0x95};
    const uint8_t rfPowerLevelsUS[] =         {3, 3, 10, 20, 18};    
    const uint8_t rfPowerLevelsEurope[] =     {3, 3, 10,  14,  14}; 
 
     // Which table to use
    const uint8_t* rfPowerLevelTable;  
    
    // The name of the operating region
    char* regionName;

    // Now, set which RF Power Level Table to use and region name based on moduleConfiguration setting
    if (moduleConfiguration & 0x01) // Only use Least significant bit; ignore others
    {
        regionName = "EU (E U R O P E)";
        rfPowerLevelTable = rfPowerLevelsEurope;        
    } else {
        regionName = "US (U S A / C A N A D A)";
        rfPowerLevelTable = rfPowerLevelsUS;
    }
    
    // Get module name. This is stored in the module's User Description
    RETURN_RESULT_IF_FAIL(getConfigurationParameter(ZCD_NV_USERDESC), METHOD_SET_MODULE_RF_POWER);
    uint8_t* moduleName = zmBuf + ZB_READ_CONFIGURATION_START_OF_VALUE_FIELD + 1;

    // Warn the user about using incorrect region setting and tell them how to change it
    printf(" * ANAREN MODULE %s CONFIGURED FOR %s\r\n", moduleName, regionName);
    printf(" * WARNING: FOR COMPLIANCE, SELECT CORRECT OPERATING REGION\r\n");
    printf(" * SET SWITCH 1 ON S4 ON FOR US, OFF FOR EUROPE\r\n\r\n");

    // Get the RF power from the lookup table
    uint8_t rfPowerLevel = rfPowerLevelTable[(productId & MODULE_TYPE_MASK)];

    // Now, set the RF power:
    uint8_t actualRfPowerLevel = 0;
    moduleResult_t result = sysSetTxPower(rfPowerLevel, &actualRfPowerLevel);  //METHOD_SET_MODULE_RF_POWER
#ifdef MODULE_UTILITIES_VERBOSE     
    printf("Setting RF Power Level Set to %d; actual level = %d\r\n", rfPowerLevel, actualRfPowerLevel);
#endif   
    return result;
}



#define METHOD_START_MODULE              0x6100
/**
Start the Module and join a network, using the AF/ZDO interface.
@param moduleConfiguration the settings to use to start the module
@param applicationConfiguration the settings to use to start the Zigbee Application
@see struct moduleConfiguration in module_utilities.h for information about each field of the moduleConfiguration
@see struct applicationConfiguration in application_configuration.h for information about each field of the applicationConfiguration
@see module.c for more information about each of these steps.
*/
moduleResult_t startModule(const struct moduleConfiguration* mc, const struct applicationConfiguration* ac)
{
	printf("Module Startup\r\n");
    /* Initialize the Module */
    RETURN_RESULT_IF_FAIL(moduleReset(), METHOD_START_MODULE);
    /* Clear out any old network or state information (if requested) */
    RETURN_RESULT_IF_FAIL(setStartupOptions(mc->startupOptions), METHOD_START_MODULE);   

/* This section is for reading Operating region from GPIO pins - omit if using different method */
    /* First, configure GPIOs as inputs: */
    RETURN_RESULT_IF_FAIL(sysGpio(GPIO_SET_DIRECTION , GPIO_DIRECTION_ALL_INPUTS), METHOD_START_MODULE);    
    RETURN_RESULT_IF_FAIL(sysGpio(GPIO_SET_INPUT_MODE , GPIO_INPUT_MODE_ALL_PULL_UPS), METHOD_START_MODULE); 
    /* Now, read GPIO inputs 0 & 1: */
    RETURN_RESULT_IF_FAIL(sysGpio(GPIO_READ, (GPIO_0 | GPIO_1)), METHOD_START_MODULE);
    uint16_t moduleConfiguration = zmBuf[SYS_GPIO_READ_RESULT_FIELD];
/* Done reading GPIO inputs. If application needs them as outputs then it shall configure accordingly */
    
    /* Reset the Module to apply the changes we just set */
    RETURN_RESULT_IF_FAIL(moduleReset(), METHOD_START_MODULE);
    /* Read the productId - this indicates the model of module used. */
    uint8_t productId = zmBuf[SYS_RESET_IND_PRODUCTID_FIELD]; 
    /* If this is not valid (bad firmware) then stop */
    RETURN_RESULT_IF_EXPRESSION_TRUE((productId < MINIMUM_BUILD_ID), METHOD_START_MODULE,
                                     ZM_INVALID_MODULE_CONFIGURATION);       
    /* Configure the module's RF output */
    setModuleRfPower(productId, moduleConfiguration);
    /* Set any end device options */
    if (mc->deviceType == END_DEVICE)
	    RETURN_RESULT_IF_FAIL(setPollRate(mc->endDevicePollRate), METHOD_START_MODULE);
    /* Configure which RF Channels to use. If none set then this will default to 11. */
    RETURN_RESULT_IF_FAIL(setZigbeeDeviceType(mc->deviceType), METHOD_START_MODULE);     // Set Zigbee Device Type
    RETURN_RESULT_IF_FAIL(setChannelMask(mc->channelMask), METHOD_START_MODULE);
    RETURN_RESULT_IF_FAIL(setPanId(mc->panId), METHOD_START_MODULE);
    RETURN_RESULT_IF_FAIL(setCallbacks(CALLBACKS_ENABLED), METHOD_START_MODULE);
    
	                                                    // Note: ZCD_NV_SECURITY_MODE defaults to 01
	  if (mc->securityMode != SECURITY_MODE_OFF)        // Note: If a coordinator has ZCD_NV_SECURITY_MODE = 00, router must have ZCD_NV_SECURITY_MODE = 01 or else they won't communicate
	  {
	      RETURN_RESULT_IF_FAIL(setSecurityMode(mc->securityMode), METHOD_START_MODULE);
	      RETURN_RESULT_IF_FAIL(setSecurityKey(mc->securityKey), METHOD_START_MODULE);
	  }

	  if (ac == GENERIC_APPLICATION_CONFIGURATION)	  //TODO: use custom applicationFramework if this isn't null:
	  {
	    RETURN_RESULT_IF_FAIL(afRegisterGenericApplication(), METHOD_START_MODULE);    // Configure the Module for our application
	  } else {
	    printf("Custom Application Frameworks not supported\r\n");
	    return INVALID_PARAMETER;
	  }

	  RETURN_RESULT_IF_FAIL(zdoStartApplication(), METHOD_START_MODULE);		// Start your engines

	  // Wait until this device has joined a network. Device State will change to DEV_ROUTER or
	  // DEV_COORD to indicate that the device has correctly joined a network.

	#ifdef ZDO_STATE_CHANGE_IND_HANDLED_BY_APPLICATION  //if you're handling this in your application instead...
	  return MODULE_SUCCESS;
	#else
	#define TEN_SECONDS 10000
	#define FIFTEEN_SECONDS 15000
	#define START_TIMEOUT FIFTEEN_SECONDS
	  RETURN_RESULT(waitForDeviceState(getDeviceStateForDeviceType(mc->deviceType), START_TIMEOUT), METHOD_START_MODULE);
	#endif

}


/** 
Retrieves a pending message from the module and displays the type of message it is.
Ignores the message if length = 0.
@pre moduleHasMessageWaiting() is true
*/
void displayMessages()
{
  getMessage();
  if (zmBuf[SRSP_LENGTH_FIELD] > 0)
  {
    switch ( (CONVERT_TO_INT(zmBuf[SRSP_CMD_LSB_FIELD], zmBuf[SRSP_CMD_MSB_FIELD])) )
    {
    case AF_DATA_CONFIRM:
      {
        printf("AF_DATA_CONFIRM\r\n");
        break;
      }
    case AF_INCOMING_MSG:
      {
        printf("AF_INCOMING_MSG\r\n");
        printAfIncomingMsgHeader(zmBuf);
        printf("\r\n");
#ifdef VERBOSE_MESSAGE_DISPLAY  
        printf("Payload: ");
        printHexBytes(zmBuf+SRSP_HEADER_SIZE+17, zmBuf[SRSP_HEADER_SIZE+16]);   //print out message payload
#endif
        break;
      }
    case AF_INCOMING_MSG_EXT:
      {
        printf("AF_INCOMING_MSG_EXT\r\n");
        uint16_t len = AF_INCOMING_MESSAGE_EXT_LENGTH();
        printf("Extended Message Received, L%u ", len);
        break;
      }
    case ZDO_IEEE_ADDR_RSP:
      {
        printf("ZDO_IEEE_ADDR_RSP\r\n");
        displayZdoAddressResponse(zmBuf + SRSP_PAYLOAD_START);
        break;
      }
    case ZDO_NWK_ADDR_RSP:
      {
        printf("ZDO_NWK_ADDR_RSP\r\n");
        displayZdoAddressResponse(zmBuf + SRSP_PAYLOAD_START);
        break;
      }
    case ZDO_END_DEVICE_ANNCE_IND:
      {
        printf("ZDO_END_DEVICE_ANNCE_IND received!\r\n");
        displayZdoEndDeviceAnnounce(zmBuf);
        break;
      }
    case ZB_FIND_DEVICE_CONFIRM:
      {
        printf("ZB_FIND_DEVICE_CONFIRM\r\n");
        break;
      }
    default:
      {
        printf("Message received, type 0x%04X\r\n", (CONVERT_TO_INT(zmBuf[SRSP_CMD_LSB_FIELD], zmBuf[SRSP_CMD_MSB_FIELD])));
        printHexBytes(zmBuf, (zmBuf[SRSP_LENGTH_FIELD] + SRSP_HEADER_SIZE));
      }
    }
    zmBuf[SRSP_LENGTH_FIELD] = 0;
  } //ignore messages with length == 0
}
