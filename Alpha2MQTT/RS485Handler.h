/*
Name:		RS485Handler.h
Created:	8/24/2022
Author:		Daniel Young

This file is part of Alpha2MQTT (A2M) which is released under GNU GENERAL PUBLIC LICENSE.
See file LICENSE or go to https://choosealicense.com/licenses/gpl-3.0/ for full license details.

Notes

Handles Modbus requests and responses in a tidy class separate from main program logic.
*/
#ifndef _RS485Handler_h
#define _RS485Handler_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "Definitions.h"

#if defined MP_ESP8266
#include <SoftwareSerial.h>
#elif defined MP_ESP32
#include <HardwareSerial.h>
#endif

// SoftwareSerial is used to create a second serial port, which will be deidcated to RS485.
// The built-in serial port remains available for flashing and debugging.

#define RS485_TX HIGH						// Transmit control pin goes high
#define RS485_RX LOW						// Receive control pin goes low
#define DEFAULT_BAUD_RATE 9600				// Just a default, Alpha2MQTT will cycle baud rates until one is found

#if defined MP_ESP8266
#define SERIAL_COMMUNICATION_CONTROL_PIN D5	// Transmission set pin
#define RX_PIN D6							// Serial Receive pin
#define TX_PIN D7							// Serial Transmit pin
#elif defined MP_ESP32
#define SERIAL_COMMUNICATION_CONTROL_PIN 33	// Transmission set pin
#define RX_PIN 16							// Serial Receive pin
#define TX_PIN 17							// Serial Transmit pin
#endif

class RS485Handler
{

	private:
#if defined MP_ESP8266
		SoftwareSerial* _RS485Serial;
#elif defined MP_ESP32
		HardwareSerial* _RS485Serial;
#endif

		char* _debugOutput;
		void flushRS485();
		modbusRequestAndResponseStatusValues listenResponse(modbusRequestAndResponse* resp);
		bool checkForData();
		void outputFrameToSerial(bool transmit, uint8_t frame[], byte actualFrameSize);
		uint16_t baudRate;

	protected:


	public:
		RS485Handler();
		~RS485Handler();
		modbusRequestAndResponseStatusValues sendModbus(uint8_t frame[], byte actualFrameSize, modbusRequestAndResponse* resp);
		bool checkCRC(uint8_t frame[], byte actualFrameSize);
		void calcCRC(uint8_t frame[], byte actualFrameSize);
		void setDebugOutput(char* _db);
		void setBaudRate(unsigned long baudRate);
};


#endif

