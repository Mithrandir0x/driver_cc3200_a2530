/**
*  @file zdo.h
*
*  @brief  public methods for zdo.c
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

#ifndef ZDO_H
#define ZDO_H

#include "application_configuration.h"
#include "module_errors.h"

moduleResult_t zdoStartApplication();
moduleResult_t zdoRequestIeeeAddress(uint16_t shortAddress, uint8_t requestType, uint8_t startIndex);
moduleResult_t zdoNetworkAddressRequest(uint8_t* ieeeAddress, uint8_t requestType, uint8_t startIndex);
void displayZdoAddressResponse(uint8_t* rsp);
void displayZdoEndDeviceAnnounce(uint8_t* announce);
moduleResult_t zdoUserDescriptorRequest(uint16_t destinationAddress, uint16_t networkAddressOfInterest);
moduleResult_t zdoNodeDescriptorRequest(uint16_t destinationAddress, uint16_t networkAddressOfInterest);
moduleResult_t zdoUserDescriptorSet(uint16_t destinationAddress, uint16_t networkAddressOfInterest, 
                                    uint8_t* userDescriptor, uint8_t userDescriptorLength);
void displayZdoUserDescriptorResponse(uint8_t* rsp);
void displayZdoNodeDescriptorResponse(uint8_t* rsp);

#define SINGLE_DEVICE_RESPONSE                          0
#define INCLUDE_ASSOCIATED_DEVICES                      1

//for ZDO_END_DEVICE_ANNCE_IND
#define FROM_ADDRESS_LSB                                (SRSP_PAYLOAD_START)
#define FROM_ADDRESS_MSB                                (SRSP_PAYLOAD_START+1)
#define SRC_ADDRESS_LSB                                 (SRSP_PAYLOAD_START+2)
#define SRC_ADDRESS_MSB                                 (SRSP_PAYLOAD_START+3)
#define IS_ZDO_END_DEVICE_ANNCE_IND()                   (CONVERT_TO_INT(zmBuf[SRSP_CMD_LSB_FIELD], zmBuf[SRSP_CMD_MSB_FIELD]) == ZDO_END_DEVICE_ANNCE_IND)
#define GET_ZDO_END_DEVICE_ANNCE_IND_FROM_ADDRESS()     (CONVERT_TO_INT(zmBuf[FROM_ADDRESS_LSB], zmBuf[FROM_ADDRESS_MSB]))
#define GET_ZDO_END_DEVICE_ANNCE_IND_SRC_ADDRESS()      (CONVERT_TO_INT(zmBuf[SRC_ADDRESS_LSB], zmBuf[SRC_ADDRESS_MSB]))
#define ZDO_END_DEVICE_ANNCE_IND_MAC_START_FIELD        (SRSP_PAYLOAD_START+4)
#define ZDO_END_DEVICE_ANNCE_IND_CAPABILITIES_FIELD                  (SRSP_PAYLOAD_START + 9)

#define ZDO_END_DEVICE_ANNCE_IND_CAPABILITIES_FLAG_DEVICETYPE_ROUTER     0x02
#define ZDO_END_DEVICE_ANNCE_IND_CAPABILITIES_FLAG_MAINS_POWERED         0x04
#define ZDO_END_DEVICE_ANNCE_IND_CAPABILITIES_FLAG_RX_ON_WHEN_IDLE       0x08
#define ZDO_END_DEVICE_ANNCE_IND_CAPABILITIES_FLAG_SECURITY_CAPABILITY   0x40


#define ZDO_USER_DESC_RSP_STATUS_FIELD                  (SRSP_PAYLOAD_START + 2)
#define ZDO_NODE_DESC_RSP_STATUS_FIELD                  (SRSP_PAYLOAD_START + 2)

#define ZDO_IEEE_ADDR_RSP_STATUS_FIELD                  (SRSP_PAYLOAD_START)
#define ZDO_NWK_ADDR_RSP_STATUS_FIELD                   (SRSP_PAYLOAD_START)
#endif
