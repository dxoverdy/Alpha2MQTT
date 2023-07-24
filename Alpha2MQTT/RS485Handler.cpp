/*
Name:		RS485Handler.cpp
Created:	8/24/2022
Author:		Daniel Young

This file is part of Alpha2MQTT (A2M) which is released under GNU GENERAL PUBLIC LICENSE.
See file LICENSE or go to https://choosealicense.com/licenses/gpl-3.0/ for full license details.

Notes

Handles Modbus requests and responses in a tidy class separate from main program logic.
*/
#include "RS485Handler.h"

/*
Default Constructor

Initialisations and what not
*/
RS485Handler::RS485Handler()
{
	// Configure the pin for controlling TX/RX (if using MAX485 with DE/RE pins)
	pinMode(SERIAL_COMMUNICATION_CONTROL_PIN, OUTPUT);

	// Set pin 'LOW' for 'Receive' mode
	//digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_RX);

#if defined MP_ESP8266
	_RS485Serial = new SoftwareSerial(RX_PIN, TX_PIN);
	_RS485Serial->begin(DEFAULT_BAUD_RATE, SWSERIAL_8N1);
#elif defined MP_ESP32
	_RS485Serial = new HardwareSerial(2); // Serial 2 PIN16=RXgreen, pin17=TXwhite
	_RS485Serial->begin(DEFAULT_BAUD_RATE, SERIAL_8N1);
#endif
	
}

/*
Default Destructor

Disconnections, clean-up and what not
*/
RS485Handler::~RS485Handler()
{
	_RS485Serial->end();
	delete _RS485Serial;
	_RS485Serial = NULL;
}


/*
setBaudRate()

Sets the baud rate for communication
*/
void RS485Handler::setBaudRate(unsigned long baudRate)
{
	_RS485Serial->flush();
	_RS485Serial->begin(baudRate);
}


/*
setDebugOutput()

Want to safely output to debug window using our fixed length RAM rather than being casual
*/
void RS485Handler::setDebugOutput(char* _db)
{
	_debugOutput = _db;
}



/*
flushRS485

Flush the RS485 buffers in both directions.
The doc for Serial.flush() implies it only flushes outbound characters now... Assume _RS485Serial is the same.
*/
void RS485Handler::flushRS485()
{
	_RS485Serial->flush();
	
	// Not sure the delay needed.
	//delay(50);

	while (_RS485Serial->available())
	{
		_RS485Serial->read();
	}
}


/*
sendModbus

Calculates the CRC for any given data frame and sends it over RS485.
Following the send, kicks off a synchronous listen for response
*/
modbusRequestAndResponseStatusValues RS485Handler::sendModbus(uint8_t frame[], byte actualFrameSize, modbusRequestAndResponse* resp)
{
	//Calculate the CRC and overwrite the last two bytes.
	calcCRC(frame, actualFrameSize);

	// Make sure there are no spurious characters in the in/out buffer.
	flushRS485();

	//Send
	digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_TX);

	// Debug output the frame?
#ifdef DEBUG_OUTPUT_TX_RX
	outputFrameToSerial(true, frame, actualFrameSize);
#endif

	_RS485Serial->write(frame, actualFrameSize);
	// Ensure it's sent on its way.
	_RS485Serial->flush();

	// It's important to reset the SERIAL_COMMUNICATION_CONTROL_PIN as soon as
	// we finish sending so that the serial port can start to buffer the response.

	digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_RX);
	
	return listenResponse(resp);
}


/*
outputFrameToSerial
 
Outputs a transmitted or received frame (regardless of population) to assist with debugging.
*/
void RS485Handler::outputFrameToSerial(bool transmit, uint8_t frame[], byte actualFrameSize)
{
	//char debugOutput[200];
	char debugByte[5];


	_debugOutput[0] = '\0';
	if (transmit)
	{
		strncat(_debugOutput, "Tx: ", 4);
	}
	else
	{
		strncat(_debugOutput, "Rx: ", 4);
	}

	if (actualFrameSize == 0)
	{
		strncat(_debugOutput, "Nothing", 7);
	}
	else
	{
		for (int counter = 0; counter < actualFrameSize; counter++)
		{
			sprintf(debugByte, "%02X", frame[counter]);
			strncat(_debugOutput, debugByte, 2);
			if (counter < actualFrameSize - 1)
			{
				strncat(_debugOutput, " ", 1);
			}
		}
	}
	//sprintf(_debugOutput, "%s", debugOutput);
	Serial.println(_debugOutput);

}





/*
listenResponse

Listens for a response and processes what it is given that data frame formats vary based on function codes
Returns data in a stucture and returns a result to guide onward processing.
*/
modbusRequestAndResponseStatusValues RS485Handler::listenResponse(modbusRequestAndResponse* resp)
{
	uint8_t inFrame[MAX_FRAME_SIZE_ZERO_INDEXED];
	uint8_t inByteNumZeroIndexed = 0;
	uint8_t inExpectedTotalBytesZeroIndexed = 0;
	bool gotSlaveID = false;
	bool gotFunctionCode = false;
	bool gotData = false;

	bool timedOut = false;




	bool breakOut = false;

	modbusRequestAndResponse dummy;
	modbusRequestAndResponseStatusValues result = modbusRequestAndResponseStatusValues::preProcessing;


	if (!resp)
	{
		resp = &dummy;
	}

	resp->dataSize = 0;

	// On responses we know we are expecting at least 5 bytes (probably 6 for two byte error code) with a slave error, successes are longer
	// But that depends on the function code returned, so when we get to the function code we can adjust expected total bytes
	inExpectedTotalBytesZeroIndexed = MIN_FRAME_SIZE_ZERO_INDEXED;

	while ((inByteNumZeroIndexed <= inExpectedTotalBytesZeroIndexed))
	{
		timedOut = !checkForData();
		if (timedOut)
		{
			break;
		}
		inFrame[inByteNumZeroIndexed] = _RS485Serial->read();

#ifdef DEBUG_LEVEL2
		sprintf(_debugOutput, "Byte number zero indexed: %d", inByteNumZeroIndexed);
		Serial.println(_debugOutput);
#endif

		//Process the byte
		switch (inByteNumZeroIndexed)
		{
		case FRAME_POSITION_SLAVE_ID:
		{
			gotSlaveID = true;
#ifdef DEBUG_LEVEL2
			sprintf(_debugOutput, "Slave ID: %d", inFrame[FRAME_POSITION_SLAVE_ID]);
			Serial.println(_debugOutput);
#endif
			// First byte is Slave ID.  If not a match (unlikely) try again on the next byte.
			if (inFrame[FRAME_POSITION_SLAVE_ID] != ALPHA_SLAVE_ID)   
			{
				gotSlaveID = false;
				inByteNumZeroIndexed--;
			}
			break;
		}
		case FRAME_POSITION_FUNCTION_CODE:
		{
			gotFunctionCode = true;

			// Second byte is Function Code.
			resp->functionCode = inFrame[FRAME_POSITION_FUNCTION_CODE];
			
#ifdef DEBUG_LEVEL2
			sprintf(_debugOutput, "Function Code: %d", inFrame[FRAME_POSITION_FUNCTION_CODE]);
			Serial.println(_debugOutput);
#endif

			// Whether the task was successful or not determines how many remaining bytes are expected
			// 
			// A slave error is a function code not equal to one of the three request codes
			if (resp->functionCode != MODBUS_FN_WRITEDATAREGISTER && resp->functionCode != MODBUS_FN_WRITESINGLEREGISTER && resp->functionCode != MODBUS_FN_READDATAREGISTER)
			{
				// Slave Error
				// Slave Address, Function Code, Error Code (1 or 2 bytes) and 2 bytes crc
				// I want to keep minimum frame size as is, which works on the assumption of a one byte error code
				// But I want to cover off the unknown of a two byte error code and it is better to attempt a read of one extra byte
				// If the Alpha system doesn't send it, we will just get a timeout.  So here we will say we are expecting two bytes
				// of data, yet still working on a min frame size of 5 bytes (0-4) to determine responses which are too short.
				// When we get out of the reading process we will look at bytes returned in total and adjust.
#ifdef DEBUG_LEVEL2
				Serial.println("Slave Error");
#endif
				
				resp->dataSize = 1;
				inExpectedTotalBytesZeroIndexed++;
			}
			else
			{
				// Success
				// 
				// If a write success, we know expected bytes 
				if (resp->functionCode == MODBUS_FN_WRITEDATAREGISTER || resp->functionCode == MODBUS_FN_WRITESINGLEREGISTER)
				{
#ifdef DEBUG_LEVEL2
					//Serial.println("Write Success");
#endif
					// In case of single register, high byte address (1), low byte address (1), high byte of data (1), low byte of data (1), (2)*crc
					// In case of data register, high byte address (1), low byte address (1), high byte of reg count (1), low byte of reg count (1), (2)*crc
					inExpectedTotalBytesZeroIndexed = MAX_FRAME_SIZE_RESPONSE_WRITE_SUCCESS_ZERO_INDEXED;
					resp->dataSize = 4;
				}
				else
				{
#ifdef DEBUG_LEVEL2
					//Serial.println("Read Success");
#endif
					// Success Read
					// Get the next byte here so we know expected length
					timedOut = !checkForData();
					if (timedOut)
					{
						breakOut = true;
						break;
					}
					

					inByteNumZeroIndexed++;
					inFrame[inByteNumZeroIndexed] = _RS485Serial->read();
					resp->dataSize = inFrame[inByteNumZeroIndexed];

					// Assume numbytes is 2
					// slave + func + numbytes + 2 + 2 crc = 7 bytes, which is 0 to 6 when zero indexed
					// And 7 takeaway 3 bytes received is 4
					inExpectedTotalBytesZeroIndexed = inFrame[inByteNumZeroIndexed] + 4;

				}
			}

			break;
		}
		default:
		{
			gotData = true;
			// If reading there's an extra byte in the form of data length
			if (resp->functionCode == MODBUS_FN_READDATAREGISTER)
			{
				resp->data[inByteNumZeroIndexed - 3] = inFrame[inByteNumZeroIndexed];
			}
			else
			{
				resp->data[inByteNumZeroIndexed - 2] = inFrame[inByteNumZeroIndexed];
			}
		}
		}

		// This can only go true if we request the next byte for data bytes and it came back with nothing
		if (breakOut)
		{
			break;
		}

		// Move to the next byte
		inByteNumZeroIndexed++;
	}



	if (timedOut)
	{
#ifdef DEBUG
		sprintf(_debugOutput, "Timed Out (inByteNumZeroIndexed): %d", inByteNumZeroIndexed);
		Serial.println(_debugOutput);
		sprintf(_debugOutput, "Timed Out (gotSlaveID): %d", gotSlaveID);
		Serial.println(_debugOutput);
		sprintf(_debugOutput, "Timed Out (gotFunctionCode): %d", gotFunctionCode);
		Serial.println(_debugOutput);
		sprintf(_debugOutput, "Timed Out (resp->functionCode): %d", resp->functionCode);
		Serial.println(_debugOutput);
		sprintf(_debugOutput, "Timed Out (gotData): %d", gotData);
		Serial.println(_debugOutput);
		sprintf(_debugOutput, "Timed Out (resp->dataSize): %d", resp->dataSize);
		Serial.println(_debugOutput);
#endif
	}




	// Check what to report back
	if (inByteNumZeroIndexed == 0)
	{
		result = modbusRequestAndResponseStatusValues::noResponse;
		strcpy(resp->statusMqttMessage, MODBUS_REQUEST_AND_RESPONSE_NO_RESPONSE_MQTT_DESC);
		strcpy(resp->displayMessage, MODBUS_REQUEST_AND_RESPONSE_NO_RESPONSE_DISPLAY_DESC);

		// Debug output the frame?
#ifdef DEBUG_OUTPUT_TX_RX
		outputFrameToSerial(false, inFrame, inByteNumZeroIndexed);
#endif
	}
	else
	{
		// Debug output the frame?
#ifdef DEBUG_OUTPUT_TX_RX
		outputFrameToSerial(false, inFrame, inByteNumZeroIndexed);
#endif

		if (gotSlaveID && gotFunctionCode && gotData)
		{
			if (resp->functionCode != MODBUS_FN_READDATAREGISTER && resp->functionCode != MODBUS_FN_WRITEDATAREGISTER && resp->functionCode != MODBUS_FN_WRITESINGLEREGISTER)
			{
				// This is the fix for not knowing how many bytes an error code is on a slave error
				if (inByteNumZeroIndexed > MIN_FRAME_SIZE_ZERO_INDEXED)
				{
					// We got an extra byte
					resp->dataSize = 2;
				}
			}
		}

		if (!timedOut && gotSlaveID && gotFunctionCode && gotData)
		{
			// If we haven't timed out, and got everything we expected from the packet,
			// The previous loop will always end up +1 more than we have, as it ends with an increment.
			// Bring it back in line.
			inByteNumZeroIndexed--;
		}



		if (inByteNumZeroIndexed < MIN_FRAME_SIZE_ZERO_INDEXED)
		{
			result = modbusRequestAndResponseStatusValues::responseTooShort;
			strcpy(resp->statusMqttMessage, MODBUS_REQUEST_AND_RESPONSE_RESPONSE_TOO_SHORT_MQTT_DESC);
			strcpy(resp->displayMessage, MODBUS_REQUEST_AND_RESPONSE_RESPONSE_TOO_SHORT_DISPLAY_DESC);
		}
		else if (checkCRC(inFrame, inByteNumZeroIndexed + 1))
		{
			if (resp->functionCode == MODBUS_FN_WRITEDATAREGISTER)
			{
				result = modbusRequestAndResponseStatusValues::writeDataRegisterSuccess;
				strcpy(resp->statusMqttMessage, MODBUS_REQUEST_AND_RESPONSE_WRITE_DATA_REGISTER_SUCCESS_MQTT_DESC);
				strcpy(resp->displayMessage, MODBUS_REQUEST_AND_RESPONSE_WRITE_DATA_REGISTER_SUCCESS_DISPLAY_DESC);
			}
			else if (resp->functionCode == MODBUS_FN_WRITESINGLEREGISTER)
			{
				result = modbusRequestAndResponseStatusValues::writeSingleRegisterSuccess;
				strcpy(resp->statusMqttMessage, MODBUS_REQUEST_AND_RESPONSE_WRITE_SINGLE_REGISTER_SUCCESS_MQTT_DESC);
				strcpy(resp->displayMessage, MODBUS_REQUEST_AND_RESPONSE_WRITE_SINGLE_REGISTER_SUCCESS_DISPLAY_DESC);
			}
			else if (resp->functionCode == MODBUS_FN_READDATAREGISTER)
			{
				result = modbusRequestAndResponseStatusValues::readDataRegisterSuccess;
				strcpy(resp->statusMqttMessage, MODBUS_REQUEST_AND_RESPONSE_READ_DATA_REGISTER_SUCCESS_MQTT_DESC);
				strcpy(resp->displayMessage, MODBUS_REQUEST_AND_RESPONSE_READ_DATA_REGISTER_SUCCESS_DISPLAY_DESC);
			}
			else
			{
				result = modbusRequestAndResponseStatusValues::slaveError;
				strcpy(resp->statusMqttMessage, MODBUS_REQUEST_AND_RESPONSE_ERROR_MQTT_DESC);
				strcpy(resp->displayMessage, MODBUS_REQUEST_AND_RESPONSE_ERROR_DISPLAY_DESC);
			}
		}
		else
		{
			result = modbusRequestAndResponseStatusValues::invalidFrame;
			strcpy(resp->statusMqttMessage, MODBUS_REQUEST_AND_RESPONSE_INVALID_FRAME_MQTT_DESC);
			strcpy(resp->displayMessage, MODBUS_REQUEST_AND_RESPONSE_INVALID_FRAME_DISPLAY_DESC);
		}
	}
	


	if (result != modbusRequestAndResponseStatusValues::writeDataRegisterSuccess && result != modbusRequestAndResponseStatusValues::writeSingleRegisterSuccess && result != modbusRequestAndResponseStatusValues::readDataRegisterSuccess)
	{
#ifdef DEBUG_LEVEL2
		sprintf(_debugOutput, "Coming out of listenResponse with an issue - Function code (%d) and %s: %d", resp->functionCode, resp->statusMqttMessage);
		Serial.println(_debugOutput);
#endif
	}

	return result;
}












/*
checkCRC

Calculates the CRC for any given data frame and compares it to what came in
calcCRC is based on
https://github.com/angeloc/simplemodbusng/blob/master/SimpleModbusMaster/SimpleModbusMaster.cpp
*/
bool RS485Handler::checkCRC(uint8_t frame[], byte actualFrameSize)
{
	unsigned int calculated_crc, received_crc;

	received_crc = ((frame[actualFrameSize - 2] << 8) | frame[actualFrameSize - 1]);
	calcCRC(frame, actualFrameSize);
	calculated_crc = ((frame[actualFrameSize - 2] << 8) | frame[actualFrameSize - 1]);

	return (received_crc == calculated_crc);
}


/*
calcCRC

Calculates the CRC for any given data frame
calcCRC is based on
https://github.com/angeloc/simplemodbusng/blob/master/SimpleModbusMaster/SimpleModbusMaster.cpp
*/
void RS485Handler::calcCRC(uint8_t frame[], byte actualFrameSize)
{
	unsigned int temp = 0xffff, flag;

	for (unsigned char i = 0; i < actualFrameSize - 2; i++)
	{
		temp = temp ^ frame[i];

		for (unsigned char j = 1; j <= 8; j++)
		{
			flag = temp & 0x0001;
			temp >>= 1;

			if (flag)
				temp ^= 0xA001;
		}
	}

	// Bytes are reversed.
	frame[actualFrameSize - 2] = temp & 0xff;
	frame[actualFrameSize - 1] = temp >> 8;
}



/*
checkForData

Returns true if there is some data in the serial buffer, otherwise false
*/
bool RS485Handler::checkForData()
{
	int tries = 0;

	
	while ((!_RS485Serial->available()) && (tries++ < RS485_TRIES))
	{
		delay(50);
	}

	if (tries >= RS485_TRIES)
	{
		Serial.println("Timeout waiting for RS485 response.  Likely no more data coming.");
		return false;
	}
	else
	{
		return true;
	}
}