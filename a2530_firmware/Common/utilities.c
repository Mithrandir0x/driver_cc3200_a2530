/**
*  @file utilities.c
*
*  @brief Miscellaneous utility methods
*
*  Miscellaneous helper utilities.
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

void printHexBytes(uint8_t* toPrint, uint16_t numBytes)
{
    int i = 0;
    for (i=0; i<numBytes; i++)
        printf("%02X ", toPrint[i]);
    printf("\r\n");
}

/** Fills the messagePartBuffer with DEADBEEF to aid in debugging */
void initializeBuffer(uint8_t* buf, uint16_t len)
{
    int i = 0;
  for (i=0; i<len; i+=4)  // initialize buffer
  {
    buf[i] = 0xDE;
    buf[(i+1)] = 0xAD;
    buf[(i+2)] = 0xBE;
    buf[(i+3)] = 0xEF;
  }
}


/** Prints a number in its binary form */
void printBinary(uint8_t n)
{
    unsigned int i;
    i = 1<<(sizeof(n) * 8 - 1);
    while (i > 0) {
        if (n & i) {
            printf("1");
        } else {
            printf("0");
        }
        i >>= 1;
    }
}

/** returns >1 if is printable: ASCII values 0x20 (space) through 0x7E (~) */
char isAsciiPrintableCharacter(uint8_t c)
{
    return ((c >= 0x20) && (c <= 0x7E));
}


/** Returns the mean from the array of values */
uint16_t getAverage(uint16_t* values, uint8_t numValues)
{
    unsigned long sum = 0;
    uint8_t i = 0;
    for (i=0; i<numValues; i++)
        sum += values[i];
    float average = sum / ((float) numValues);
    return (unsigned int) average;
}

/** Rounds an integer to the given number of places */
uint16_t roundInteger(uint16_t number, uint16_t place)
{
    unsigned int i = 1;
    while (place > 0)
    {
        i = i * 10;
        place--;
    }
    int r = number % i;
    if (r < (i / 2))
        return number - r;
    else
        return number - r + i;
}

/** Returns the maximum value from the array of values */
uint16_t max(uint16_t values[], uint16_t numValues)
{
    uint16_t maximum = 0;
    uint16_t i = 0;
    for (i=0; i<numValues; i++)
    {
        if (values[i] > maximum)
            maximum = values[i];
    }  
    return maximum;
}

/** Returns the minimum value from the array of values */
uint16_t min(uint16_t values[], uint16_t numValues)
{
    uint16_t minimum = 0xFFFF;
    uint16_t i = 0;
    for (i=0; i<numValues; i++)
    {
        if (values[i] < minimum)
            minimum = values[i];
    }
    return minimum;
}
