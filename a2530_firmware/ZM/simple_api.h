/**
*  @file simple_api.h
*
*  @brief  public methods for simple_api.c
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

#ifndef SIMPLE_API_H
#define SIMPLE_API_H

#include "application_configuration.h"
#include "module_errors.h"

moduleResult_t sapiRegisterApplication(struct applicationConfiguration ac);
moduleResult_t sapiRegisterGenericApplication();
moduleResult_t sapiStartApplication();
moduleResult_t sapiSetJoiningPermissions(uint16_t destination, uint8_t timeout);
moduleResult_t sapiSendData(uint16_t destinationShortAddress, uint16_t clusterId, uint8_t* data, uint8_t dataLength);
moduleResult_t sapiStartModule(uint8_t deviceType, uint32_t channelMask, uint16_t panId);

//used in PERMIT_JOINING_REQUEST methods
#define NO_TIMEOUT                      0xFF

//used in sapiSendData options
#define SAPI_MAC_ACK                    0x00    //Require Acknowledgement from next device on route
#define SAPI_APS_ACK                    0x01    //Require Acknowledgement from final destination (if using Simple API)

#endif
