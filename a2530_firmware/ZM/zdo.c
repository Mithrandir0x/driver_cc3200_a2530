/**
* @file zdo.c
*
* @brief Methods that implement the Zigbee Device Objects (ZDO) interface.
* 
* The AF/ZDO interface is a more powerful version of the Simple API and allows you to configure, send, and receive Zigbee data.
* This file acts as an interface between the user's application and the Module physical interface.
* Module interface could be either SPI or UART.
* Refer to Interface Specification for more information.
*
* @note For more information, define ZDO_VERBOSE. It is recommended to define this on a per-project basis. 
* In IAR, this can be done in Project Options : C/C++ compiler : Preprocessor
* In the defined symbols box, add:
* ZDO_VERBOSE
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

#include "zdo.h"
#include "module.h"
#include "../HAL/hal.h"
#include "../Common/utilities.h"
#include "module_errors.h"
#include "zm_phy_spi.h"
#include <string.h>                 //for memcpy()
#include <stdint.h>

extern uint8_t zmBuf[ZIGBEE_MODULE_BUFFER_SIZE];

#define METHOD_ZDO_STARTUP_FROM_APP                    0x31
/** Starts the Zigbee stack in the Module using the settings from a previous afRegisterApplication().
After this start request process completes, the device is ready to send, receive, and route network traffic.
@note On a coordinator in a trivial test setup, it takes approximately 300mSec between sending 
START_REQUEST and receiving START_REQUEST_SRSP and then another 200-1000mSec from when we receive 
START_REQUEST_SRSP to when we receive START_CONFIRM. Set START_CONFIRM_TIMEOUT based on size of your network.
@note ZDO_STARTUP_FROM_APP field StartDelay not used
@pre afRegisterApplication() was a success.
@post We will see Device Status change to DEV_ROUTER, DEV_ZB_COORD, or DEV_END_DEVICE correspondingly if everything was ok.
*/
moduleResult_t zdoStartApplication()
{
#ifdef ZDO_VERBOSE    
    printf("Start Application with AF/ZDO...");
#endif    
#define ZDO_STARTUP_FROM_APP_PAYLOAD_LEN 1    
    zmBuf[0] = ZDO_STARTUP_FROM_APP_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_STARTUP_FROM_APP);
    zmBuf[2] = LSB(ZDO_STARTUP_FROM_APP);      
    
#define NO_START_DELAY 0
    zmBuf[3] = NO_START_DELAY;
    RETURN_RESULT(sendMessage(), ZDO_STARTUP_FROM_APP);
}

#define METHOD_ZDO_IEEE_ADDR_REQ                    0x32
#define METHOD_ZDO_IEEE_ADDR_RSP                    0x33
/** Requests a remote device's MAC Address (64-bit IEEE Address) given a short address.
@param shortAddress the short address to locate
@param requestType must be SINGLE_DEVICE_RESPONSE or INCLUDE_ASSOCIATED_DEVICES. 
If SINGLE_DEVICE_RESPONSE is selected, then only information about the requested device will be returned. 
If INCLUDE_ASSOCIATED_DEVICES is selected, then the short addresses of the selected device's children will be returned too.
@param startIndex If INCLUDE_ASSOCIATED_DEVICES was selected, then there may be too many children to 
fit in one ZDO_IEEE_ADDR_RSP message. So, use startIndex to get the next set of children's short addresses.
@post zmBuf contains the ZDO_IEEE_ADDR_RSP message.
*/
moduleResult_t zdoRequestIeeeAddress(uint16_t shortAddress, uint8_t requestType, uint8_t startIndex)
{
    RETURN_INVALID_PARAMETER_IF_TRUE(((requestType != SINGLE_DEVICE_RESPONSE) && (requestType != INCLUDE_ASSOCIATED_DEVICES)), METHOD_ZDO_IEEE_ADDR_REQ);
#ifdef ZDO_VERBOSE     
    printf("Requesting IEEE Address for short address %04X, requestType %s, startIndex %u\r\n", 
           shortAddress, (requestType == 0) ? "Single" : "Extended", startIndex);
#endif 
    
#define ZDO_IEEE_ADDR_REQ_PAYLOAD_LEN 4
    zmBuf[0] = ZDO_IEEE_ADDR_REQ_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_IEEE_ADDR_REQ);             
    zmBuf[2] = LSB(ZDO_IEEE_ADDR_REQ);      
    
    zmBuf[3] = LSB(shortAddress);
    zmBuf[4] = MSB(shortAddress);
    zmBuf[5] = requestType;
    zmBuf[6] = startIndex;
    
#ifdef ZDO_IEEE_ADDR_RSP_HANDLED_BY_APPLICATION           //Return control to main application
    RETURN_RESULT(sendMessage(), METHOD_ZDO_IEEE_ADDR_REQ);
#else
    RETURN_RESULT_IF_FAIL(sendMessage(), METHOD_ZDO_IEEE_ADDR_REQ);     
    
#define ZDO_IEEE_ADDR_RSP_TIMEOUT 10
    RETURN_RESULT_IF_FAIL(waitForMessage(ZDO_IEEE_ADDR_RSP, ZDO_IEEE_ADDR_RSP_TIMEOUT), METHOD_ZDO_IEEE_ADDR_RSP);
    RETURN_RESULT(zmBuf[ZDO_IEEE_ADDR_RSP_STATUS_FIELD], METHOD_ZDO_IEEE_ADDR_RSP);
#endif
}

#define METHOD_ZDO_NWK_ADDR_REQ                     0x34
#define METHOD_ZDO_NWK_ADDR_RSP                     0x35
/** Requests a remote device's Short Address for a given long address.
@param ieeeAddress the long address to locate, LSB first!
@param requestType must be SINGLE_DEVICE_RESPONSE or INCLUDE_ASSOCIATED_DEVICES. 
If SINGLE_DEVICE_RESPONSE is selected, then only information about the requested device will be returned. 
If INCLUDE_ASSOCIATED_DEVICES is selected, then the short addresses of the selected device's children will be returned too.
@param startIndex If INCLUDE_ASSOCIATED_DEVICES was selected, then there may be too many children to 
fit in one ZDO_NWK_ADDR_RSP message. So, use startIndex to get the next set of children's short addresses.
@note DOES NOT WORK FOR SLEEPING END DEVICES
@note may not work correctly when using UART interface
@post An ZDO_NWK_ADDR_RSP message will be received, with one or more entries.
@return a pointer to the beginning of the payload, or a pointer to indeterminate data if error.
*/
moduleResult_t zdoNetworkAddressRequest(uint8_t* ieeeAddress, uint8_t requestType, uint8_t startIndex)
{
    RETURN_INVALID_PARAMETER_IF_TRUE(((requestType != SINGLE_DEVICE_RESPONSE) && (requestType != INCLUDE_ASSOCIATED_DEVICES)), METHOD_ZDO_NWK_ADDR_REQ);
    
#ifdef ZDO_VERBOSE     
    printf("Requesting Network Address for long address ");
    printHexBytes(ieeeAddress, 8);
    printf("requestType %s, startIndex %u\r\n", (requestType == 0) ? "Single" : "Extended", startIndex);
#endif
#define ZDO_NWK_ADDR_REQ_PAYLOAD_LEN 10
    zmBuf[0] = ZDO_NWK_ADDR_REQ_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_NWK_ADDR_REQ);
    zmBuf[2] = LSB(ZDO_NWK_ADDR_REQ);      
    
    memcpy(zmBuf+3, ieeeAddress, 8);
    zmBuf[11] = requestType;
    zmBuf[12] = startIndex;
    
#ifdef ZDO_NWK_ADDR_RSP_HANDLED_BY_APPLICATION  //Main application will wait for ZDO_NWK_ADDR_RSP message.    
    RETURN_RESULT(sendMessage(), METHOD_ZDO_NWK_ADDR_REQ);
#else
    RETURN_RESULT_IF_FAIL(sendMessage(), METHOD_ZDO_NWK_ADDR_REQ);     
    
#define ZDO_NWK_ADDR_RSP_TIMEOUT 10
    RETURN_RESULT_IF_FAIL(waitForMessage(ZDO_NWK_ADDR_RSP, ZDO_NWK_ADDR_RSP_TIMEOUT), METHOD_ZDO_NWK_ADDR_RSP);
    RETURN_RESULT(zmBuf[ZDO_NWK_ADDR_RSP_STATUS_FIELD], METHOD_ZDO_NWK_ADDR_RSP);
#endif
}

/** Displays the returned value of ZdoUserDescriptorRequest()
@param rsp points to the beginning of the response
@pre rsp holds a valid response
*/
void displayZdoUserDescriptorResponse(uint8_t* rsp)
{
    
    if (rsp[2] != MODULE_SUCCESS)
    {
        printf("Failed (Error Code %02X)\r\n", rsp[0]);
        return;
    }
    
        printf("User Descriptor Response Received - Source Address=0x%04X, Network Address=0x%04X\r\n", CONVERT_TO_INT(rsp[0], rsp[1]), CONVERT_TO_INT(rsp[3], rsp[4]));
        printf("Length=%u, User Descriptor=", rsp[5]);
        // Display the user descriptor
        int i;
        for (i=0; i<rsp[5]; i++)
            printf("%c", rsp[6+i]);
        printf("\r\n");
    
}

/** Displays the returned value of ZdoNodeDescriptorRequest()
@param rsp points to the beginning of the response
@pre rsp holds a valid response
*/
void displayZdoNodeDescriptorResponse(uint8_t* rsp)
{
    
    if (rsp[2] != MODULE_SUCCESS)
    {
        printf("Failed (Error Code %02X)\r\n", rsp[0]);
        return;
    }
    
        printf("Node Descriptor Response Received - Source Address=0x%04X, Network Address=0x%04X\r\n", CONVERT_TO_INT(rsp[0], rsp[1]), CONVERT_TO_INT(rsp[3], rsp[4]));
    printf("    Logical Type / ComplexDesc / User Desc = 0x%02X\r\n", rsp[5]);
    printf("    APSFlags / Frequency Band = %02X\r\n", rsp[6]);
    printf("    Mac Capabilities = 0x%02X\r\n", rsp[7]);
    printf("    Manufacturer Code = 0x%04X\r\n", CONVERT_TO_INT(rsp[8], rsp[9]));
    printf("    Max Buffer Size = 0x%02X\r\n", rsp[10]);
    printf("    Max In Transfer Size = 0x%04X\r\n", CONVERT_TO_INT(rsp[11], rsp[12]));
    printf("    Server Mask = 0x%04X\r\n", CONVERT_TO_INT(rsp[13], rsp[14]));
    printf("    Max Out Transfer Size = 0x%04X\r\n", CONVERT_TO_INT(rsp[15], rsp[16]));
    printf("    Descriptor Capabilities = 0x%02X\r\n", rsp[17]);    
    
}

/** Displays the returned value of zdoNetworkAddressRequest() or zdoRequestIeeeAddress()
Both response messages use the same format.
@param rsp points to the beginning of the response
@pre rsp holds a valid response
*/
void displayZdoAddressResponse(uint8_t* rsp)
{
    if (rsp[0] != MODULE_SUCCESS)
    {
        printf("Failed to find that device (Error Code %02X)\r\n", rsp[0]);
    }
    else
    {
        printf("Device Found! MAC (MSB first): ");
        int i;
        for (i=8; i>0; i--)
            printf("%02X ", rsp[i]);
        printf(", Short Address:%04X\r\n", CONVERT_TO_INT(rsp[9], rsp[10]));
        if (rsp[12] > 0)
        {
            printf("%u Associated Devices, starting at #%u:", rsp[12], rsp[11]);
            int j;
            for (j=0; j<rsp[12]; j++)
                printf("(%04X) ", CONVERT_TO_INT(rsp[(j+13)], rsp[(j+14)]));
            printf("\r\n");
        } else {
            printf("\r\n");
        }
    }
}

/** Displays the parsed fields in a ZDO_END_DEVICE_ANNCE_IND message 
@param announce points to the start of a ZDO_END_DEVICE_ANNCE_IND: (zmBuf)
*/
void displayZdoEndDeviceAnnounce(uint8_t* announce)
{    
    if (announce[ZDO_END_DEVICE_ANNCE_IND_CAPABILITIES_FIELD] & 
        ZDO_END_DEVICE_ANNCE_IND_CAPABILITIES_FLAG_DEVICETYPE_ROUTER) {
        printf("ROUTER ");
    } else {
        printf("END DEVICE ");
    }
    printf("Announce From:%04X Addr:%04X MAC:", GET_ZDO_END_DEVICE_ANNCE_IND_FROM_ADDRESS(), GET_ZDO_END_DEVICE_ANNCE_IND_SRC_ADDRESS());
    int i;
    for (i=11; i>3; i--)
        printf("%02X", announce[i]);
    printf(" Capabilities:%02X\r\n", announce[ZDO_END_DEVICE_ANNCE_IND_CAPABILITIES_FIELD]);
}

#define METHOD_ZDO_USER_DESC_REQ                    0x36
#define METHOD_ZDO_USER_DESC_RSP                    0x37
/** Requests a remote device's user descriptor. This is a 16 byte text field that may be used for
anything and may be read/written remotely.
@param destinationAddress the short address of the destination
@param networkAddressOfInterest the short address of the device the query is intended for
@post zmBuf contains the ZDO_USER_DESC_RSP message.
*/
moduleResult_t zdoUserDescriptorRequest(uint16_t destinationAddress, uint16_t networkAddressOfInterest)
{
#ifdef ZDO_VERBOSE     
    printf("Requesting User Descriptor for destination %04X, NWK address %04X\r\n", destinationAddress, networkAddressOfInterest);
#endif 
    
#define ZDO_USER_DESC_REQ_PAYLOAD_LEN 4
    zmBuf[0] = ZDO_USER_DESC_REQ_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_USER_DESC_REQ);             
    zmBuf[2] = LSB(ZDO_USER_DESC_REQ);      
    
    zmBuf[3] = LSB(destinationAddress);
    zmBuf[4] = MSB(destinationAddress);
    zmBuf[5] = LSB(networkAddressOfInterest);
    zmBuf[6] = MSB(networkAddressOfInterest);
    
#ifdef ZDO_USER_DESC_RSP_HANDLED_BY_APPLICATION           //Return control to main application
    RETURN_RESULT(sendMessage(), METHOD_ZDO_USER_DESC_REQ);
#else
    RETURN_RESULT_IF_FAIL(sendMessage(), METHOD_ZDO_USER_DESC_REQ);     
    
    // Now wait for the response...
#define ZDO_USER_DESC_RSP_TIMEOUT 10
    RETURN_RESULT_IF_FAIL(waitForMessage(ZDO_USER_DESC_RSP, ZDO_USER_DESC_RSP_TIMEOUT), METHOD_ZDO_USER_DESC_RSP);
    RETURN_RESULT(zmBuf[ZDO_USER_DESC_RSP_STATUS_FIELD], METHOD_ZDO_USER_DESC_RSP);
#endif
}

#define METHOD_ZDO_NODE_DESC_REQ                    0x38
#define METHOD_ZDO_NODE_DESC_RSP                    0x39
/** Requests a remote device's node descriptor. 
@param destinationAddress the short address of the destination
@param networkAddressOfInterest the short address of the device the query is intended for
@post zmBuf contains the ZDO_NODE_DESC_RSP message.
*/
moduleResult_t zdoNodeDescriptorRequest(uint16_t destinationAddress, uint16_t networkAddressOfInterest)
{
#ifdef ZDO_VERBOSE     
    printf("Requesting Node Descriptor for destination %04X, NWK address %04X\r\n", destinationAddress, networkAddressOfInterest);
#endif 
    
#define ZDO_NODE_DESC_REQ_PAYLOAD_LEN 4
    zmBuf[0] = ZDO_USER_DESC_REQ_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_NODE_DESC_REQ);             
    zmBuf[2] = LSB(ZDO_NODE_DESC_REQ);      
    
    zmBuf[3] = LSB(destinationAddress);
    zmBuf[4] = MSB(destinationAddress);
    zmBuf[5] = LSB(networkAddressOfInterest);
    zmBuf[6] = MSB(networkAddressOfInterest);
    
#ifdef ZDO_NODE_DESC_RSP_HANDLED_BY_APPLICATION           //Return control to main application
    RETURN_RESULT(sendMessage(), METHOD_ZDO_NODE_DESC_REQ);
#else
    RETURN_RESULT_IF_FAIL(sendMessage(), METHOD_ZDO_NODE_DESC_REQ);     
    
    // Now wait for the response...
#define ZDO_NODE_DESC_RSP_TIMEOUT 10
    RETURN_RESULT_IF_FAIL(waitForMessage(ZDO_NODE_DESC_RSP, ZDO_NODE_DESC_RSP_TIMEOUT), METHOD_ZDO_NODE_DESC_RSP);
    RETURN_RESULT(zmBuf[ZDO_NODE_DESC_RSP_STATUS_FIELD], METHOD_ZDO_NODE_DESC_RSP);
#endif
}

#define METHOD_ZDO_USER_DESC_SET                    0x3A
/** Sets a remote device's user descriptor. 
@param destinationAddress the short address of the destination
@param networkAddressOfInterest the short address of the device
*/
moduleResult_t zdoUserDescriptorSet(uint16_t destinationAddress, uint16_t networkAddressOfInterest, 
                                    uint8_t* userDescriptor, uint8_t userDescriptorLength)
{
#ifdef ZDO_VERBOSE     
    printf("Setting User Descriptor for destination %04X, NWK address %04X, length %u\r\n", 
           destinationAddress, networkAddressOfInterest, userDescriptorLength);
#endif 
    
#define ZDO_USER_DESC_SET_PAYLOAD_LEN_HEADER   5
    zmBuf[0] = ZDO_USER_DESC_REQ_PAYLOAD_LEN + userDescriptorLength;
    zmBuf[1] = MSB(ZDO_USER_DESC_SET);             
    zmBuf[2] = LSB(ZDO_USER_DESC_SET);      
    
    zmBuf[3] = LSB(destinationAddress);
    zmBuf[4] = MSB(destinationAddress);
    zmBuf[5] = LSB(networkAddressOfInterest);
    zmBuf[6] = MSB(networkAddressOfInterest);
    zmBuf[7] = userDescriptorLength;
    memcpy(zmBuf+8, userDescriptor, userDescriptorLength);
    
    displayZmBuf();

    RETURN_RESULT(sendMessage(), METHOD_ZDO_USER_DESC_SET);
}
