/**
* @file simple_api.c
*
* @brief Methods that implement the Simple Application Programming Interface (Simple API).
* 
* This Simple API is an easy-to-use interface to configure, send, and receive Zigbee data.
* This file acts as an interface between the user's application and the Module physical interface, 
* either SPI or UART. Refer to Wiki for more information.
*
* @note For more information, define SIMPLE_API_VERBOSE. It is recommended to define this on a per-project basis. 
* In IAR, this can be done in Project Options : C/C++ compiler : Preprocessor
* In the defined symbols box, add:
* SIMPLE_API_VERBOSE
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

#include "simple_api.h"
#include "module.h"
#include "application_configuration.h"
#include "../HAL/hal.h"
#include "../Common/utilities.h"
#include <string.h> //for memcpy()
#include "module_errors.h"
#include "zm_phy_spi.h"
#include <stdint.h>

extern uint8_t zmBuf[ZIGBEE_MODULE_BUFFER_SIZE];

/** Incremented for each SEND_DATA_REQUEST, wraps around to 0. */
uint8_t sequenceNumber = 0;
                
#define METHOD_SAPI_REGISTER_APPLICATION              0x4100
/** Configures the Module for our application.
Sets which endpoint, profileId, etc. we're using as well as binding information.
@note Simple API can only register one application.
@param the applicationConfiguration to register.
@pre Module was initialized and ZCD_NV_LOGICAL_TYPE has been set (either COORDINATOR/ROUTER/END_DEVICE).
@post can now use sapiStartApplication()
@see applicationConfiguration
*/
moduleResult_t sapiRegisterApplication(struct applicationConfiguration ac)
{
#ifdef SIMPLE_API_VERBOSE
    printf("Register Application Configuration with Simple API\r\n");
#endif
    RETURN_INVALID_PARAMETER_IF_TRUE( (ac.endPoint == 0), METHOD_SAPI_REGISTER_APPLICATION);
    RETURN_INVALID_CLUSTER_IF_TRUE( ((ac.numberOfBindingInputClusters > MAX_BINDING_CLUSTERS) || (ac.numberOfBindingOutputClusters > MAX_BINDING_CLUSTERS)), METHOD_SAPI_REGISTER_APPLICATION);

    uint8_t bufferIndex;
    
    //zmBuf[0] (length) will be set later
    zmBuf[1] = MSB(ZB_APP_REGISTER_REQUEST);
    zmBuf[2] = LSB(ZB_APP_REGISTER_REQUEST); 
    
    zmBuf[3] = ac.endPoint;
    zmBuf[4] = LSB(ac.profileId);
    zmBuf[5] = MSB(ac.profileId);
    zmBuf[6] = LSB(ac.deviceId);
    zmBuf[7] = MSB(ac.deviceId);
    zmBuf[8] = ac.deviceVersion;
    zmBuf[9] = 0x00;                     //unused, set to zero
    zmBuf[10] = ac.numberOfBindingInputClusters;
    bufferIndex = 11;
    uint8_t i = 0;
    for (i = 0; i<ac.numberOfBindingInputClusters; i++)
    {
        zmBuf[bufferIndex++] = LSB(ac.bindingInputClusters[i]);
        zmBuf[bufferIndex++] = MSB(ac.bindingInputClusters[i]);
    }
    zmBuf[bufferIndex++] = ac.numberOfBindingOutputClusters;
    for (i = 0; i<ac.numberOfBindingOutputClusters; i++)
    {
        zmBuf[bufferIndex++] = LSB(ac.bindingOutputClusters[i]);
        zmBuf[bufferIndex++] = MSB(ac.bindingOutputClusters[i]);
    }
    zmBuf[0] = bufferIndex;
    RETURN_RESULT(sendMessage(), METHOD_SAPI_REGISTER_APPLICATION); 
}

#define METHOD_SAPI_REGISTER_GENERIC_APPLICATION              0x4200
/** Configures the Module for a "generic" application: one endpoint, no binding or fancy stuff. 
Sets which endpoint, profileId, etc. we're using as well as binding information
@see sapiRegisterApplication()
*/
moduleResult_t sapiRegisterGenericApplication()
{
    #define ZB_APP_REGISTER_REQUEST_PAYLOAD_LEN 9    
    zmBuf[0] = ZB_APP_REGISTER_REQUEST_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZB_APP_REGISTER_REQUEST);
    zmBuf[2] = LSB(ZB_APP_REGISTER_REQUEST); 
    
    zmBuf[3] = DEFAULT_ENDPOINT;
    zmBuf[4] = LSB(DEFAULT_PROFILE_ID);
    zmBuf[5] = MSB(DEFAULT_PROFILE_ID);
    zmBuf[6] = LSB(DEVICE_ID);
    zmBuf[7] = MSB(DEVICE_ID);
    zmBuf[8] = DEVICE_VERSION;
    zmBuf[9] = 0; //unused
    zmBuf[10] = 0; //number of binding input clusters
    zmBuf[11] = 0; //number of binding output clusters
    RETURN_RESULT(sendMessage(), METHOD_SAPI_REGISTER_APPLICATION);
}

#define METHOD_SAPI_START_APPLICATION              0x4300
/** Starts the Zigbee stack in the Module using the settings from a previous sapiRegisterApplication().
After this start request process completes, the device is ready to send, receive, and route network traffic.
@note On a coordinator in a trivial test setup, it takes approximately 300mSec between 
START_REQUEST and receiving START_REQUEST_SRSP and then another 200-1000mSec from when we receive 
START_REQUEST_SRSP to when we receive START_CONFIRM. 
@note Set START_CONFIRM_TIMEOUT based on size of your network.
@pre sapiRegisterApplication() was a success.
@note we will receive a START_CONFIRM if everything was ok. 
Should always receive a start confirm for a coordinator. 
Will only receive a start confirm on a router if we could join a zigbee network.
*/
moduleResult_t sapiStartApplication()
{
#ifdef SIMPLE_API_VERBOSE    
    printf("Start Application with Simple API...");
#endif    
    #define ZB_APP_START_REQUEST_PAYLOAD_LEN 0
    zmBuf[0] = ZB_APP_START_REQUEST_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZB_APP_START_REQUEST);
    zmBuf[2] = LSB(ZB_APP_START_REQUEST);     
    RETURN_RESULT_IF_FAIL(sendMessage(), METHOD_SAPI_START_APPLICATION);     

#define ZB_START_CONFIRM_TIMEOUT 5 
     RETURN_RESULT_IF_FAIL(waitForMessage(ZB_START_CONFIRM, ZB_START_CONFIRM_TIMEOUT), METHOD_SAPI_START_APPLICATION);
     RETURN_RESULT(zmBuf[SRSP_PAYLOAD_START], METHOD_SAPI_START_APPLICATION);    
}

#define METHOD_SAPI_SET_JOINING_PERMISSIONS              0x4400
/** Sets the Joining Permissions for this device or other devices. 
By default, after a setStartupOptions(CLEAR_CONFIG), joining is set to allow all devices, indefinitely.
@param destination which short address to set joining permissions for, or ALL_ROUTERS_AND_COORDINATORS 
to set joining permissions for all. 
@param timeout how long in seconds to allow permissions, or if set to NO_TIMEOUT then joining will be on indefinitely.
*/
moduleResult_t sapiSetJoiningPermissions(uint16_t destination, uint8_t timeout)
{
#ifdef SIMPLE_API_VERBOSE 
    printf("Allowing Joining for destination ");
    if (destination == ALL_ROUTERS_AND_COORDINATORS) 
        printf("ALL_ROUTERS_AND_COORDINATORS");
    else
        printf("0x%04X", destination);
    if (timeout == NO_TIMEOUT) 
        printf(" forever\r\n");
    else
        printf(" for %u seconds\r\n", timeout);
#endif
    
    #define ZB_PERMIT_JOINING_REQUEST_PAYLOAD_LEN 3
    zmBuf[0] = ZB_PERMIT_JOINING_REQUEST_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZB_PERMIT_JOINING_REQUEST);
    zmBuf[2] = LSB(ZB_PERMIT_JOINING_REQUEST);   
    
    zmBuf[3] = LSB(destination);
    zmBuf[4] = MSB(destination);
    zmBuf[5] = timeout;
    RETURN_RESULT(sendMessage(), METHOD_SAPI_SET_JOINING_PERMISSIONS);      
}


#define METHOD_SAPI_SEND_DATA              0x4500
/** Sends a message to another device over the Zigbee network.
@note On a coordinator in a trivial test setup, it takes approximately 10mSec from sending 
ZB_SEND_DATA_REQUEST to when we receive ZB_SEND_DATA_CONFIRM.
@note   <code>handle</code> is an optional user-definable reference number to match SEND_DATA_CONFIRM messages 
with the SEND_DATA_REQUEST message. This can be used to determine which SEND_DATA_REQUEST generated an error.
@note   The Module will automatically require an ACK from the next device on the route when sending data. 
To require an ACK from the final destination, change MAC_ACK to APS_ACK. The downside is that this increases network traffic.
@note   The <code>radius</code> is the maximum number of hops that this packet can travel through before it will be dropped. 
Should be set to the maximum number of hops expected in the network.
@note   adjust ZB_SEND_DATA_CONFIRM_TIMEOUT based on network size, number of hops, etc.
@param  destinationShortAddress the short address of the destination, or ALL_DEVICES or ALL_ROUTERS_AND_COORDINATORS to broadcast the message.
@param  clusterId which cluster this message is for. User definable. Zigbee supports up to 2^16 
clusters. A cluster is typically a particular command, e.g. "turn on lights" or "get temperature". If
using a predefined Zigbee Alliance Application Profile then this cluster will follow the Zigbee Cluster Library.
@param  data is the data to send.
@param  dataLength is how many bytes of data to send. Must be less than MAXIMUM_PAYLOAD_LENGTH.
@pre    sapiStartApplication() was a success, START_CONFIRM received without any errors.
@pre    there is another device on the network with short address of <code>destinationShortAddress</code> 
and that device has successfully started its application using sapiStartApplication().
@post   we will receive a ZB_SEND_DATA_CONFIRM if the message was successfully sent. 
*/
moduleResult_t sapiSendData(uint16_t destinationShortAddress, uint16_t clusterId, uint8_t* data, uint8_t dataLength)   
{
#ifdef SIMPLE_API_VERBOSE     
    printf("Sending %u bytes to cluster %u (%04X) at Short Address %u (0x%04X)\r\n", dataLength, clusterId, clusterId, destinationShortAddress, destinationShortAddress);
#endif    
    RETURN_INVALID_LENGTH_IF_TRUE( ((dataLength > MAXIMUM_PAYLOAD_LENGTH) || (dataLength == 0)), METHOD_SAPI_SEND_DATA);
    RETURN_INVALID_CLUSTER_IF_TRUE( (clusterId == 0), METHOD_SAPI_SEND_DATA);

    #define ZB_SEND_DATA_REQUEST_PAYLOAD_LEN 8
    zmBuf[0] = ZB_SEND_DATA_REQUEST_PAYLOAD_LEN + dataLength;
    zmBuf[1] = MSB(ZB_SEND_DATA_REQUEST);
    zmBuf[2] = LSB(ZB_SEND_DATA_REQUEST);  
    
    zmBuf[3] = LSB(destinationShortAddress); 
    zmBuf[4] = MSB(destinationShortAddress); 
    zmBuf[5] = LSB(clusterId); 
    zmBuf[6] = MSB(clusterId); 
    zmBuf[7] = sequenceNumber++;  //handle aka Transaction Sequence Number
    zmBuf[8] = SAPI_MAC_ACK;  
    zmBuf[9] = DEFAULT_RADIUS;
    zmBuf[10] = dataLength;  
    memcpy(zmBuf+ZB_SEND_DATA_REQUEST_PAYLOAD_LEN+3, data, dataLength);
    
    RETURN_RESULT_IF_FAIL(sendMessage(), METHOD_SAPI_SEND_DATA);  
    #define ZB_SEND_DATA_CONFIRM_TIMEOUT    5
    
    //Now, wait for message, and verify that it's a ZB_SEND_DATA_CONFIRM, else timeout.
    //NOTE: Do not print anything out here, or else you might miss the ZB_SEND_DATA_CONFIRM!    
    RETURN_RESULT_IF_FAIL(waitForMessage(ZB_SEND_DATA_CONFIRM, ZB_SEND_DATA_CONFIRM_TIMEOUT), METHOD_SAPI_SEND_DATA);
    RETURN_RESULT(zmBuf[SRSP_PAYLOAD_START+1], METHOD_SAPI_SEND_DATA); //verify status is succesS
}

#define MODULE_TYPE_MASK                    0x0F    
#define IS_VALID_MODULE_TYPE(x)             ((x>=0x20) && (x<=0x24))

/** 
Configures the RF power based on the type of module.
@param productId the productId as returned from a SYS_RESET_IND. If unknown productId (most likely
caused by older firmware) then the method exits with status MODULE_SUCCESS.
*/
static moduleResult_t setModuleRfPower(uint8_t productId)
{
    if (!(IS_VALID_MODULE_TYPE(productId)))
        return MODULE_SUCCESS;
    
    printf(" ANAREN MODULE CONFIGURED FOR ");
#ifdef REGION_NORTH_AMERICA
    printf("US (U S A / C A N A D A)");    //0xX0, 0xX1, 0xX2, 0xX3, 0xX4
    const uint8_t rfPowerLevelTable[] =         {3, 3, 10, 20, 18};      
    //const uint8_t rfPowerLevelTable[] =         {0xF5, 0xF5, 0xE5, 0xE5, 0xC5};    //US    
#elif defined REGION_EUROPE
    printf("EU (E U R O P E)");        //0xX0, 0xX1, 0xX2, 0xX3, 0xX4
    const uint8_t rfPowerLevelTable[]   =     {0xF5, 0xF5, 0xE5, 0x95, 0x95};       //Europe
    //const uint8_t rfPowerLevelsEurope[] =     {3, 3, 10,  14,  14};
#else
#error "Region must be defined"
#endif
    printf("\r\n WARNING: FOR COMPLIANT OPERATION THE CORRECT OPERATING REGION MUST BE SELECTED\r\n");
    printf(" SEE INSERT IN BOOSTERPACK FOR HOW TO SET\r\n");
    
    uint8_t rfPowerLevel = rfPowerLevelTable[(productId & MODULE_TYPE_MASK)];
    uint8_t actualRfPowerLevel = 0;  // will be overwritten by sysSetTxPower()
    moduleResult_t result = sysSetTxPower(rfPowerLevel, &actualRfPowerLevel);  //METHOD_SET_MODULE_RF_POWER
    printf("RF Power Level Set to %d\r\n", rfPowerLevel);
#ifdef SIMPLE_API_VERBOSE
    printf("Actual level = %d\r\n", actualRfPowerLevel);
#endif   
    return result;
}

#define METHOD_SAPI_START_MODULE              0x4600
/** Start the Module and join a network, using the Simple API interface.
@param deviceType the type of device we're starting, e.g. ROUTER.
@param channelMask the channelMask we'd like to use. Parameter must be one or more channels 'OR'd 
together, e.g. (CHANNEL_MASK_13 | CHANNEL_MASK_17 | CHANNEL_MASK_21) for channels 13,17,21. May also
be set to ANY_CHANNEL_MASK which will use all channels.
@param panId if you want to use a specific PAN_ID. Otherwise set to ANY_PAN.
@note Example usage is sapiStartModule(ROUTER, CHANNEL_MASK_11 | CHANNEL_MASK_13, ANY_PAN)
will start the module to be a router, using channels 11 and 13, on any PAN ID, using no binding.
@see module.c for more information about each of these steps.
*/
moduleResult_t sapiStartModule(uint8_t deviceType, uint32_t channelMask, uint16_t panId)
{
    printf("Module Startup\r\n");
    
    RETURN_RESULT_IF_FAIL(moduleReset(), METHOD_SAPI_START_MODULE);    /* Initialize the Module */
    /* Read the productId - this indicates the model of module used. */
    uint8_t productId = zmBuf[SYS_RESET_IND_PRODUCTID_FIELD];     
    RETURN_RESULT_IF_FAIL(setModuleRfPower(productId), METHOD_SAPI_START_MODULE);
    RETURN_RESULT_IF_FAIL(setStartupOptions(STARTOPT_CLEAR_CONFIG + STARTOPT_CLEAR_STATE), METHOD_SAPI_START_MODULE);     /* Set Startup Options (will restore the Module to default values on reset) */
    RETURN_RESULT_IF_FAIL(moduleReset(), METHOD_SAPI_START_MODULE); /* Reset the Module */
    RETURN_RESULT_IF_FAIL(setZigbeeDeviceType(deviceType), METHOD_SAPI_START_MODULE);     /* Set Zigbee Device Type to be ROUTER */
    RETURN_RESULT_IF_FAIL(setChannelMask(channelMask), METHOD_SAPI_START_MODULE);
    RETURN_RESULT_IF_FAIL(sapiRegisterGenericApplication(), METHOD_SAPI_START_MODULE);    /* Configure the Module for our application */
    RETURN_RESULT(sapiStartApplication(), METHOD_SAPI_START_MODULE);
}
