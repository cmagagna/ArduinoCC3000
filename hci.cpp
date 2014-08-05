/**************************************************************************
*
*  This file is part of the ArduinoCC3000 library.
*
*  Version 1.0.1b
* 
*  Copyright (C) 2013 Chris Magagna - cmagagna@yahoo.com
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*  Don't sue me if my code blows up your board and burns down your house
*
*
*  This code is based on version 13 of the TI CC3000HostDriver
*  reference library. Changes to the reference library file,
*  if any, are listed below:
*
*  + File renamed from *.c to *.cpp to force the Arduino compiler to 
     treat it as a C++ file
     
   + #include <arduino.h> added

   + The line
          #include "spi.h"
     was changed to
          #include "ArduinoCC3000SPI.h"
     because Arduino already has a "SPI.h" library
* 
****************************************************************************/










/*****************************************************************************
*
*  hci.c  - CC3000 Host Driver Implementation.
*  Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*****************************************************************************/

//*****************************************************************************
//
//! \addtogroup hci_app
//! @{
//
//*****************************************************************************

#include <arduino.h>

#include "cc3000_common.h"
#include "hci.h"
#include "ArduinoCC3000SPI.h"
#include "evnt_handler.h"
#include "wlan.h"

#define SL_PATCH_PORTION_SIZE		(1000)


//*****************************************************************************
//
//!  hci_command_send
//!
//!  @param  usOpcode     command operation code
//!  @param  pucBuff      pointer to the command's arguments buffer
//!  @param  ucArgsLength length of the arguments
//!
//!  @return              none
//!
//!  @brief               Initiate an HCI command.
//
//*****************************************************************************
unsigned short 
hci_command_send(unsigned short usOpcode, unsigned char *pucBuff,
                     unsigned char ucArgsLength)
{ 
	unsigned char *stream;
	
	stream = (pucBuff + SPI_HEADER_SIZE);
	
	UINT8_TO_STREAM(stream, HCI_TYPE_CMND);
	stream = UINT16_TO_STREAM(stream, usOpcode);
	UINT8_TO_STREAM(stream, ucArgsLength);
	
	//Update the opcode of the event we will be waiting for
	SpiWrite(pucBuff, ucArgsLength + SIMPLE_LINK_HCI_CMND_HEADER_SIZE);
	
	return(0);
}

//*****************************************************************************
//
//!  hci_data_send
//!
//!  @param  usOpcode        command operation code
//!	 @param  ucArgs					 pointer to the command's arguments buffer
//!  @param  usArgsLength    length of the arguments
//!  @param  ucTail          pointer to the data buffer
//!  @param  usTailLength    buffer length
//!
//!  @return none
//!
//!  @brief              Initiate an HCI data write operation
//
//*****************************************************************************
long
hci_data_send(unsigned char ucOpcode, 
							unsigned char *ucArgs,
							unsigned short usArgsLength, 
							unsigned short usDataLength,
							const unsigned char *ucTail,
							unsigned short usTailLength)
{
	unsigned char *stream;
	
	stream = ((ucArgs) + SPI_HEADER_SIZE);
	
	UINT8_TO_STREAM(stream, HCI_TYPE_DATA);
	UINT8_TO_STREAM(stream, ucOpcode);
	UINT8_TO_STREAM(stream, usArgsLength);
	stream = UINT16_TO_STREAM(stream, usArgsLength + usDataLength + usTailLength);
	
	// Send the packet over the SPI
	SpiWrite(ucArgs, SIMPLE_LINK_HCI_DATA_HEADER_SIZE + usArgsLength + usDataLength + usTailLength);
	
	return(ESUCCESS);
}


//*****************************************************************************
//
//!  hci_data_command_send
//!
//!  @param  usOpcode      command operation code
//!  @param  pucBuff       pointer to the data buffer
//!  @param  ucArgsLength  arguments length
//!  @param  ucDataLength  data length
//!
//!  @return none
//!
//!  @brief              Prepeare HCI header and initiate an HCI data write operation
//
//*****************************************************************************
void hci_data_command_send(unsigned short usOpcode, unsigned char *pucBuff,
                     unsigned char ucArgsLength,unsigned short ucDataLength)
{ 
 	unsigned char *stream = (pucBuff + SPI_HEADER_SIZE);
	
	UINT8_TO_STREAM(stream, HCI_TYPE_DATA);
	UINT8_TO_STREAM(stream, usOpcode);
	UINT8_TO_STREAM(stream, ucArgsLength);
	stream = UINT16_TO_STREAM(stream, ucArgsLength + ucDataLength);
	
	// Send the command over SPI on data channel
	SpiWrite(pucBuff, ucArgsLength + ucDataLength + SIMPLE_LINK_HCI_DATA_CMND_HEADER_SIZE);
	
	return;
}

//*****************************************************************************
//
//!  hci_patch_send
//!
//!  @param  usOpcode      command operation code
//!  @param  pucBuff       pointer to the command's arguments buffer
//!  @param  patch         pointer to patch content buffer 
//!  @param  usDataLength  data length
//!
//!  @return              none
//!
//!  @brief               Prepeare HCI header and initiate an HCI patch write operation
//
//*****************************************************************************
void
hci_patch_send(unsigned char ucOpcode, unsigned char *pucBuff, char *patch, unsigned short usDataLength)
{ 
 	unsigned char *data_ptr = (pucBuff + SPI_HEADER_SIZE);
	unsigned short usTransLength;
	unsigned char *stream = (pucBuff + SPI_HEADER_SIZE);
	
	UINT8_TO_STREAM(stream, HCI_TYPE_PATCH);
	UINT8_TO_STREAM(stream, ucOpcode);
	stream = UINT16_TO_STREAM(stream, usDataLength + SIMPLE_LINK_HCI_PATCH_HEADER_SIZE);
	
	if (usDataLength <= SL_PATCH_PORTION_SIZE)
	{
		UINT16_TO_STREAM(stream, usDataLength);
		stream = UINT16_TO_STREAM(stream, usDataLength);
		memcpy((pucBuff + SPI_HEADER_SIZE) + HCI_PATCH_HEADER_SIZE, patch, usDataLength);
		
		// Update the opcode of the event we will be waiting for
		SpiWrite(pucBuff, usDataLength + HCI_PATCH_HEADER_SIZE);
	}
	else
	{
		
		usTransLength = (usDataLength/SL_PATCH_PORTION_SIZE);
		UINT16_TO_STREAM(stream, usDataLength + SIMPLE_LINK_HCI_PATCH_HEADER_SIZE + usTransLength*SIMPLE_LINK_HCI_PATCH_HEADER_SIZE);
		stream = UINT16_TO_STREAM(stream, SL_PATCH_PORTION_SIZE);
		memcpy(pucBuff + SPI_HEADER_SIZE + HCI_PATCH_HEADER_SIZE, patch, SL_PATCH_PORTION_SIZE);
		usDataLength -= SL_PATCH_PORTION_SIZE;
		patch += SL_PATCH_PORTION_SIZE;
		
		// Update the opcode of the event we will be waiting for
		SpiWrite(pucBuff, SL_PATCH_PORTION_SIZE + HCI_PATCH_HEADER_SIZE);
		
		while (usDataLength)
		{
			if (usDataLength <= SL_PATCH_PORTION_SIZE)
			{
				usTransLength = usDataLength;
				usDataLength = 0;
				
			}
			else
			{
				usTransLength = SL_PATCH_PORTION_SIZE;
				usDataLength -= usTransLength;
			}
			
			*(unsigned short *)data_ptr = usTransLength;
			memcpy(data_ptr + SIMPLE_LINK_HCI_PATCH_HEADER_SIZE, patch, usTransLength);
			patch += usTransLength;
			
			// Update the opcode of the event we will be waiting for
			SpiWrite((unsigned char *)data_ptr, usTransLength + sizeof(usTransLength));
		}
	}
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//
//*****************************************************************************
