/*
Name:		RegisterHandler.h
Created:	24/Aug/2022
Author:		Daniel Young

This file is part of Alpha2MQTT (A2M) which is released under GNU GENERAL PUBLIC LICENSE.
See file LICENSE or go to https://choosealicense.com/licenses/gpl-3.0/ for full license details.

Notes

Handles AlphaESS specific functions as of v1.23 AlphaESS Modbus documentation
Namely:
	0x03 READ DATA REGISTER
	0x06 WRITE SINGLE REGISTER
	0x0A WRITE DATA REGISTER

I presumed when I started that WRITE SINGLE REGISTER was for single two byte registers, and was
put in to provide a simpler way to write the bulk of the smaller registers without needing to worry
about sending over data lengths, etc.  On my Smile B3 at least, WRITE SINGLE REGISTER does nothing,
it does not attempt any data changes and it does not yield any response (nor slave error) from the
system.  I found that WRITE DATA REGISTER will write any appropriate single (2 byte) or double (4 byte)
register, and we just leverage 'Number of bytes' to guide it accordingly.
*/
#ifndef _REGISTERHANDLER_h
#define _REGISTERHANDLER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "Definitions.h"
#include "RS485Handler.h"

class RegisterHandler
{
	private:
		RS485Handler* _modBus;

		// We will have a function to set serial number prefix as error codes depend on whether the
		// system serial number begings AL or AE.
		// Default AL
		char _serialNumberPrefix[3] = "AL";
		void createFormattedDateTime(char *target, uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

	protected:


	public:
		RegisterHandler();
		RegisterHandler(RS485Handler* modBus);
		~RegisterHandler();

		void setModbus(RS485Handler* modBus);
		void setSerialNumberPrefix(uint8_t char1, uint8_t char2);
		modbusRequestAndResponseStatusValues readHandledRegister(uint16_t registerAddress, modbusRequestAndResponse* rs);
		modbusRequestAndResponseStatusValues readRawRegister(uint16_t registerAddress, modbusRequestAndResponse* rs);
		modbusRequestAndResponseStatusValues writeRawSingleRegister(uint16_t registerAddress, uint16_t value, modbusRequestAndResponse* rs);
		modbusRequestAndResponseStatusValues writeRawDataRegister(uint16_t registerAddress, uint32_t value, modbusRequestAndResponse* rs);
};


#endif

