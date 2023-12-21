/*
Name:		Alpha Slave Address Finder.ino
Created:	22/Jan/2023
Author:		Daniel Young

This file is part of Alpha2MQTT (A2M) which is released under GNU GENERAL PUBLIC LICENSE.
See file LICENSE or go to https://choosealicense.com/licenses/gpl-3.0/ for full license details.

Notes

First, go and customise options such as WIFI_SSID and the rest of it below.
*/

// Supporting files
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>

// Output various levels of detail to the serial monitor
#define DEBUG
//#define DEBUG_LEVEL2


#define OUTPUT_SERIAL		// Output Slave ID detail to the serial monitor (9600 baud)
#define OUTPUT_WIFIANDMQTT	// Connect to WiFi and MQTT to output Slave ID detail as an MQTT message
#define OUTPUT_DISPLAY		// Output Slave ID detail to the display


#if !defined OUTPUT_SERIAL && !defined OUTPUT_WIFIANDMQTT && !defined OUTPUT_DISPLAY
#error You need to specify at least one output type!
#endif

// These timers are used in the main loop.
#define ITERATE_INTERVAL 10000
#define DISPLAY_FINAL_OUTPUT_FOR 20000
#define UPDATE_STATUS_BAR_INTERVAL 500

// Update with your Wifi details
#define WIFI_SSID		"Stardust"
#define WIFI_PASSWORD	""

// Update with your MQTT Broker details
#define MQTT_SERVER	"192.168.1.135"
#define MQTT_PORT	1883
#define MQTT_USERNAME	"Alpha"			// Empty string for none.
#define MQTT_PASSWORD	""


// The device name is used as the MQTT base topic and presence on the network.
// If you need more than one Alpha2MQTT on your network, give them unique names.
#define DEVICE_NAME "Alpha2MQTT"

// x 50mS to wait for RS485 input chars.  300ms as per Modbus documentation, but I got timeouts on that.  However 400ms works without issue
#define RS485_TRIES 8
#define INVERTER_BAUD_RATE 9600







// The ESP8266 has limited memory and so reserving lots of RAM to build a payload and MQTT buffer causes out of memory exceptions.
// 4096 works well given the RAM requirements of Alpha2MQTT.
// If you aren't using an ESP8266 you may be able to increase this.
// At 4096 (4095 usable) you can request up to around 70 to 80 registers on a schedule or by request.
// Alpha2MQTT on boot will request a buffer size of (MAX_MQTT_PAYLOAD_SIZE + MQTT_HEADER_SIZE) for MQTT, and
// MAX_MQTT_PAYLOAD_SIZE for building payloads.  If these fail and your device doesn't boot, you can assume you've set this too high.
#define MAX_MQTT_PAYLOAD_SIZE 4096
#define MIN_MQTT_PAYLOAD_SIZE 512
#define MQTT_HEADER_SIZE 512


// We will test the SOC register as it is common across all
#define REG_BATTERY_HOME_R_SOC 0x0102	// 0.1/bit, 2 Bytes, Unsigned Short
#define REG_BATTERY_HOME_R_SOC_REGCOUNT 1

// MQTT message to fire out
#define MQTT_MES_ITERATE_SECOND_TEN "/iterate/second/ten"

// Number of Slave IDs to try
#define SLAVE_ID_START 0
#define SLAVE_ID_END 255

// Frame and Function Codes
#define MAX_FRAME_SIZE_ZERO_INDEXED 63
#define MIN_FRAME_SIZE_ZERO_INDEXED 4
#define MAX_FRAME_SIZE_RESPONSE_WRITE_SUCCESS_ZERO_INDEXED 7

#define MODBUS_FN_READDATAREGISTER 0x03
#define MODBUS_FN_WRITEDATAREGISTER 0x10
#define MODBUS_FN_WRITESINGLEREGISTER 0x06

#define FRAME_POSITION_SLAVE_ID 0
#define FRAME_POSITION_FUNCTION_CODE 1


// Wemos OLED Shield set up. 64x48, pins D1 and D2
#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 _display(OLED_RESET);


// SoftwareSerial is used to create a second serial port, which will be deidcated to RS485.
// The built-in serial port remains available for flashing and debugging.
#define RS485_TX HIGH						// Transmit control pin goes high
#define RS485_RX LOW						// Receive control pin goes low

#define SERIAL_COMMUNICATION_CONTROL_PIN D5	// Transmission set pin
#define RX_PIN D6							// Serial Receive pin
#define TX_PIN D7							// Serial Transmit pin

// Ensure we stick to fixed values by forcing from a selection of values for a Modbus request & response
enum modbusRequestAndResponseStatusValues
{
	preProcessing,
	invalidFrame,
	responseTooShort,
	noResponse,
	noMQTTPayload,
	invalidMQTTPayload,
	readDataRegisterSuccess,
	slaveError,
	payloadExceededCapacity,
	addedToPayload,
	notValidIncomingTopic
};
#define MODBUS_REQUEST_AND_RESPONSE_PREPROCESSING_MQTT_DESC "preProcessing"
#define MODBUS_REQUEST_AND_RESPONSE_INVALID_FRAME_MQTT_DESC "invalidFrame"
#define MODBUS_REQUEST_AND_RESPONSE_RESPONSE_TOO_SHORT_MQTT_DESC "responseTooShort"
#define MODBUS_REQUEST_AND_RESPONSE_NO_RESPONSE_MQTT_DESC "noResponse"
#define MODBUS_REQUEST_AND_RESPONSE_NO_MQTT_PAYLOAD_MQTT_DESC "noMQTTPayload"
#define MODBUS_REQUEST_AND_RESPONSE_INVALID_MQTT_PAYLOAD_MQTT_DESC "invalidMQTTPayload"
#define MODBUS_REQUEST_AND_RESPONSE_READ_DATA_REGISTER_SUCCESS_MQTT_DESC "readDataRegisterSuccess"
#define MODBUS_REQUEST_AND_RESPONSE_ERROR_MQTT_DESC "slaveError"
#define MODBUS_REQUEST_AND_RESPONSE_PAYLOAD_EXCEEDED_CAPACITY_MQTT_DESC "payloadExceededCapacity"
#define MODBUS_REQUEST_AND_RESPONSE_ADDED_TO_PAYLOAD_MQTT_DESC "addedToPayload"

#define MAX_CHARACTER_VALUE_LENGTH 21
#define MAX_MQTT_NAME_LENGTH 81
#define MAX_MQTT_STATUS_LENGTH 51
#define MAX_FORMATTED_DATA_VALUE_LENGTH 129
#define MAX_DATA_TYPE_DESC_LENGTH 20
#define MAX_FORMATTED_DATE_LENGTH 21
#define OLED_CHARACTER_WIDTH 11



// This is the request and return object for the sendModbus() function.
// Some vars are populated prior to the request, which can be used in the response to handle the data
// appropriately, for example typecasting and cleansing.
struct modbusRequestAndResponse
{
	//uint8_t errorLevel;
	uint8_t data[MAX_FRAME_SIZE_ZERO_INDEXED] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	uint8_t dataSize = 0;
	uint8_t functionCode = 0;
};


// Device parameters
char _version[6] = "v1.00";


#ifdef OUTPUT_WIFIANDMQTT
// WiFi parameters
WiFiClient _wifi;

// MQTT parameters
PubSubClient _mqtt(_wifi);

// Buffer Size (and therefore payload size calc)
int _bufferSize;
int _maxPayloadSize;

// I want to declare this once at a modular level, keep the heap somewhere in check.
char* _mqttPayload;
#endif



SoftwareSerial* _RS485Serial;




// OLED variables
char _oledOperatingIndicator = '*';
char _oledLine2[OLED_CHARACTER_WIDTH] = "";
char _oledLine3[OLED_CHARACTER_WIDTH] = "";
char _oledLine4[OLED_CHARACTER_WIDTH] = "";


// Fixed char array for messages to the serial port
char _debugOutput[100];


uint8_t _alphaSlaveID = 0;


// Prototypes
void flushRS485();
modbusRequestAndResponseStatusValues listenResponse(modbusRequestAndResponse* resp);
bool checkForData();
modbusRequestAndResponseStatusValues sendModbus(uint8_t frame[], byte actualFrameSize, modbusRequestAndResponse* resp);
bool checkCRC(uint8_t frame[], byte actualFrameSize);
void calcCRC(uint8_t frame[], byte actualFrameSize);
void iterateSlaveIds(char* topic);

#ifdef OUTPUT_WIFIANDMQTT
modbusRequestAndResponseStatusValues addToPayload(char* addition);
void sendMqtt(char* topic);
void emptyPayload();
#endif






/*
setup

The setup function runs once when you press reset or power the board
*/
void setup()
{
	// Set up serial for debugging using an appropriate baud rate
	// This is for communication with the development environment, NOT the Alpha system
	// See Definitions.h for this.
	Serial.begin(9600);


	// Configure LED for output
	pinMode(LED_BUILTIN, OUTPUT);


	// Set up the software serial for communicating with the MAX
	// Configure the pin for controlling TX/RX (if using MAX485 with DE/RE pins)
	pinMode(SERIAL_COMMUNICATION_CONTROL_PIN, OUTPUT);

	// Set pin 'LOW' for 'Receive' mode
	digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_RX);

	_RS485Serial = new SoftwareSerial(RX_PIN, TX_PIN);
	_RS485Serial->begin(INVERTER_BAUD_RATE);

	// Half second wait to give things time to kick in
	delay(500);

	// Turn on the OLED
	_display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize OLED with the I2C addr 0x3C (for the 64x48)
	_display.clearDisplay();
	_display.display();
	updateOLED(false, "", "", _version);


	// Configure WIFI
#ifdef OUTPUT_WIFIANDMQTT
	setupWifi();

	// Configure MQTT to the address and port specified above
	_mqtt.setServer(MQTT_SERVER, MQTT_PORT);


#ifdef DEBUG
	sprintf(_debugOutput, "About to request buffer");
	Serial.println(_debugOutput);
#endif
	for (_bufferSize = (MAX_MQTT_PAYLOAD_SIZE + MQTT_HEADER_SIZE); _bufferSize >= MIN_MQTT_PAYLOAD_SIZE + MQTT_HEADER_SIZE; _bufferSize = _bufferSize - 1024)
	{
#ifdef DEBUG
		sprintf(_debugOutput, "Requesting a buffer of : %d bytes", _bufferSize);
		Serial.println(_debugOutput);
#endif

		if (_mqtt.setBufferSize(_bufferSize))
		{

			_maxPayloadSize = _bufferSize - MQTT_HEADER_SIZE;
#ifdef DEBUG
			sprintf(_debugOutput, "_bufferSize: %d,\r\n\r\n_maxPayload (Including null terminator): %d", _bufferSize, _maxPayloadSize);
			Serial.println(_debugOutput);
#endif

			// Example, 2048, if declared as 2048 is positions 0 to 2047, and position 2047 needs to be zero.  2047 usable chars in payload.
			_mqttPayload = new char[_maxPayloadSize];
			emptyPayload();

			break;
		}
		else
		{
#ifdef DEBUG
			sprintf(_debugOutput, "Coudln't allocate buffer of %d bytes", _bufferSize);
			Serial.println(_debugOutput);
#endif
		}
	}

	// Connect to MQTT
	mqttReconnect();
#endif



	updateOLED(false, "", "", _version);
}




/*
loop

The loop function runs overand over again until power down or reset
*/
void loop()
{
	// Refresh LED Screen, will cause the status asterisk to flicker
	updateOLED(true, "", "", "");


#ifdef OUTPUT_WIFIANDMQTT
	// Make sure WiFi is good
	if (WiFi.status() != WL_CONNECTED)
	{
		setupWifi();
	}

	// make sure mqtt is still connected
	if ((!_mqtt.connected()) || !_mqtt.loop())
	{
		mqttReconnect();
	}
#endif

	// Read and do appropriate actions
	sendData();
}





void flushRS485()
{
	_RS485Serial->flush();

	// Not sure the delay needed.
	//delay(200);

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
modbusRequestAndResponseStatusValues sendModbus(uint8_t frame[], byte actualFrameSize, modbusRequestAndResponse* resp)
{
	//Calculate the CRC and overwrite the last two bytes.
	calcCRC(frame, actualFrameSize);

	// Make sure there are no spurious characters in the in/out buffer.
	flushRS485();

	//Send
	digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_TX);
	_RS485Serial->write(frame, actualFrameSize);

	// It's important to reset the SERIAL_COMMUNICATION_CONTROL_PIN as soon as
	// we finish sending so that the serial port can start to buffer the response.
	digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_RX);

	return listenResponse(resp);
}


/*
setBaudRate()

Sets the baud rate for communication
*/
void setBaudRate(unsigned long baudRate)
{
	_RS485Serial->flush();
	_RS485Serial->begin(baudRate);
}


/*
listenResponse

Listens for a response and processes what it is given that data frame formats vary based on function codes
Returns data in a stucture and returns a result to guide onward processing.
*/
modbusRequestAndResponseStatusValues listenResponse(modbusRequestAndResponse* resp)
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
			if (inFrame[FRAME_POSITION_SLAVE_ID] != _alphaSlaveID)
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
	}
	else
	{
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
		}
		else if (checkCRC(inFrame, inByteNumZeroIndexed + 1))
		{
			if (resp->functionCode == MODBUS_FN_READDATAREGISTER)
			{
				result = modbusRequestAndResponseStatusValues::readDataRegisterSuccess;
			}
			else
			{
				result = modbusRequestAndResponseStatusValues::slaveError;
			}
		}
		else
		{
			result = modbusRequestAndResponseStatusValues::invalidFrame;
		}
	}



	if (result != modbusRequestAndResponseStatusValues::readDataRegisterSuccess)
	{
#ifdef DEBUG_LEVEL2
		sprintf(_debugOutput, "Coming out of listenResponse with an issue - Function code (%d) and %s: %d", resp->functionCode);
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
bool checkCRC(uint8_t frame[], byte actualFrameSize)
{
	unsigned int calculated_crc, received_crc;

	received_crc = ((frame[actualFrameSize - 2] << 8) | frame[actualFrameSize - 1]);
	calcCRC(frame, actualFrameSize);
	calculated_crc = ((frame[actualFrameSize - 2] << 8) | frame[actualFrameSize - 1]);

	return (received_crc = calculated_crc);
}


/*
calcCRC

Calculates the CRC for any given data frame
calcCRC is based on
https://github.com/angeloc/simplemodbusng/blob/master/SimpleModbusMaster/SimpleModbusMaster.cpp
*/
void calcCRC(uint8_t frame[], byte actualFrameSize)
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
bool checkForData()
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

/*
setupWifi

Connect to WiFi
*/
#ifdef OUTPUT_WIFIANDMQTT
void setupWifi()
{
	// We start by connecting to a WiFi network
#ifdef DEBUG
	sprintf(_debugOutput, "Connecting to %s", WIFI_SSID);
	Serial.println(_debugOutput);
#endif

	// Set up in Station Mode - Will be connecting to an access point
	WiFi.mode(WIFI_STA);

	// And connect to the details defined at the top
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

	// And continually try to connect to WiFi.  If it doesn't, the device will just wait here before continuing
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(250);
		updateOLED(false, "Connecting", "WiFi...", _version);
	}

	// Set the hostname for this Arduino
	WiFi.hostname(DEVICE_NAME);

	// Output some debug information
#ifdef DEBUG
	Serial.print("WiFi connected, IP is");
	Serial.print(WiFi.localIP());
#endif

	// Connected, so ditch out with blank screen
	updateOLED(false, "", "", _version);
}
#endif



/*
checkTimer

Check to see if the elapsed interval has passed since the passed in millis() value. If it has, return true and update the lastRun.
Note that millis() overflows after 50 days, so we need to deal with that too... in our case we just zero the last run, which means the timer
could be shorter but it's not critical... not worth the extra effort of doing it properly for once in 50 days.
*/
bool checkTimer(unsigned long* lastRun, unsigned long interval)
{
	unsigned long now = millis();

	if (*lastRun > now)
		*lastRun = 0;

	if (now >= *lastRun + interval)
	{
		*lastRun = now;
		return true;
	}

	return false;
}

/*
updateOLED

Update the OLED. Use "NULL" for no change to a line or "" for an empty line.
Three parameters representing each of the three lines available for status indication - Top line functionality fixed
*/
void updateOLED(bool justStatus, const char* line2, const char* line3, const char* line4)
{
	static unsigned long updateStatusBar = 0;


	_display.clearDisplay();
	_display.setTextSize(1);
	_display.setTextColor(WHITE);
	_display.setCursor(0, 0);

	char line1Contents[OLED_CHARACTER_WIDTH];
	char line2Contents[OLED_CHARACTER_WIDTH];
	char line3Contents[OLED_CHARACTER_WIDTH];
	char line4Contents[OLED_CHARACTER_WIDTH];

	// Ensure only dealing with 10 chars passed in, and null terminate.
	if (strlen(line2) > OLED_CHARACTER_WIDTH - 1)
	{
		strncpy(line2Contents, line2, OLED_CHARACTER_WIDTH - 1);
		line2Contents[OLED_CHARACTER_WIDTH - 1] = 0;
	}
	else
	{
		strcpy(line2Contents, line2);
	}


	if (strlen(line3) > OLED_CHARACTER_WIDTH - 1)
	{
		strncpy(line3Contents, line3, OLED_CHARACTER_WIDTH - 1);
		line3Contents[OLED_CHARACTER_WIDTH - 1] = 0;
	}
	else
	{
		strcpy(line3Contents, line3);
	}


	if (strlen(line4) > OLED_CHARACTER_WIDTH - 1)
	{
		strncpy(line4Contents, line4, OLED_CHARACTER_WIDTH - 1);
		line4Contents[OLED_CHARACTER_WIDTH - 1] = 0;
	}
	else
	{
		strcpy(line4Contents, line4);
	}

	// Only update the operating indicator once per half second.
	if (checkTimer(&updateStatusBar, UPDATE_STATUS_BAR_INTERVAL))
	{
		// Simply swap between space and asterisk every time we come here to give some indication of activity
		_oledOperatingIndicator = (_oledOperatingIndicator == '*') ? ' ' : '*';
	}

	// There's ten characters we can play with, width wise.
	sprintf(line1Contents, "Finding...");
	_display.println(line1Contents);




	// Next line

	_display.setCursor(0, 12);
	if (!justStatus)
	{
		_display.println(line2Contents);
		strcpy(_oledLine2, line2Contents);
	}
	else
	{
		_display.println(_oledLine2);
	}



	_display.setCursor(0, 24);
	if (!justStatus)
	{
		_display.println(line3Contents);
		strcpy(_oledLine3, line3Contents);
	}
	else
	{
		_display.println(_oledLine3);
	}

	_display.setCursor(0, 36);
	if (!justStatus)
	{
		_display.println(line4Contents);
		strcpy(_oledLine4, line4Contents);
	}
	else
	{
		_display.println(_oledLine4);
	}
	// Refresh the display
	_display.display();
}









/*
mqttReconnect

This function reconnects the ESP8266 to the MQTT broker
*/
#ifdef OUTPUT_WIFIANDMQTT
void mqttReconnect()
{
	// Loop until we're reconnected
	while (true)
	{
		_mqtt.disconnect();		// Just in case.
		delay(200);

#ifdef DEBUG
		Serial.print("Attempting MQTT connection...");
#endif

		updateOLED(false, "Connecting", "MQTT...", _version);
		delay(100);

		// Attempt to connect
		if (_mqtt.connect(DEVICE_NAME, MQTT_USERNAME, MQTT_PASSWORD))
		{
			Serial.println("Connected MQTT");

			// Connected
			break;

		}

		// Wait 1 second before retrying
		delay(1000);
	}
}
#endif

/*
addToPayload

Safely adds characters to the payload buffer, ensuring no overruns.
*/
#ifdef OUTPUT_WIFIANDMQTT
modbusRequestAndResponseStatusValues addToPayload(char* addition)
{
	int targetRequestedSize = strlen(_mqttPayload) + strlen(addition);

	// If max payload size is 2048 it is stored as (0-2047), however character 2048  (position 2047) is null terminator so 2047 chars usable usable
	if (targetRequestedSize > _maxPayloadSize - 1)
	{
		// Safely print using snprintf
		snprintf(_mqttPayload, _maxPayloadSize, "{\r\n    \"mqttError\": \"Length of payload exceeds %d bytes.  Length would be %d bytes.\"\r\n}", _maxPayloadSize - 1, targetRequestedSize);

		return modbusRequestAndResponseStatusValues::payloadExceededCapacity;
	}
	else
	{
		// Add to the payload by sprintf back on itself with the addition
		sprintf(_mqttPayload, "%s%s", _mqttPayload, addition);

		return modbusRequestAndResponseStatusValues::addedToPayload;
	}
}
#endif

/*
sendData

Runs once every loop, checks to see if the time period has elapsed for iteration
If so, fir eoff the code
*/
void sendData()
{
	static unsigned long lastRunTenSeconds = 0;

	// Update all parameters and send to MQTT.
	if (checkTimer(&lastRunTenSeconds, ITERATE_INTERVAL))
	{
#ifdef DEBUG
		Serial.print("Iterate interval has elapsed, now enumerating all potential Slave IDs...");
#endif
		iterateSlaveIds(DEVICE_NAME MQTT_MES_ITERATE_SECOND_TEN);
	}
}


void iterateSlaveIds(char* topic)
{
	char stateAddition[128] = ""; // 128 should cover individual additions to the payload
	char line2Contents[OLED_CHARACTER_WIDTH];
	char line3Contents[OLED_CHARACTER_WIDTH];
	char line4Contents[OLED_CHARACTER_WIDTH];

	unsigned long knownBaudRates[7] = { 9600, 115200, 19200, 57600, 38400, 14400, 4800 };

	uint8_t alphaSlaveIdToTry = 0;
	char testingHexSlaveAddress[5] = "";
	char successfulHexSlaveAddress[5] = "";
	char testingBaudRate[10] = "";
	char successfulBaudRate[10] = "";

	bool found = false;
	bool thisWorked = false;
	int baudRateIterator = -1;
	bool gotResponse = false;

	// Empty the string
	sprintf(successfulHexSlaveAddress, "");
	sprintf(successfulBaudRate, "");

	// For storing results
	modbusRequestAndResponseStatusValues result;
	modbusRequestAndResponseStatusValues resultAddedToPayload;
	modbusRequestAndResponse response;

#ifdef OUTPUT_DISPLAY
	updateOLED(false, "", "", "");
#endif

#ifdef OUTPUT_WIFIANDMQTT
	emptyPayload();
	resultAddedToPayload = addToPayload("{\r\n");
#else
	resultAddedToPayload = modbusRequestAndResponseStatusValues::addedToPayload;
#endif


	if (resultAddedToPayload == modbusRequestAndResponseStatusValues::addedToPayload)
	{


		while (!gotResponse)
		{
			// Starts at -1, so increment to 0 for example
			baudRateIterator++;

			// Go back to zero if beyond the bounds
			if (baudRateIterator > (sizeof(knownBaudRates) / sizeof(knownBaudRates[0])) - 1)
			{
				baudRateIterator = 0;
			}

			// Update the display
			sprintf(testingBaudRate, "%u", knownBaudRates[baudRateIterator]);

			// Set the rate
			setBaudRate(knownBaudRates[baudRateIterator]);


			alphaSlaveIdToTry = SLAVE_ID_START;
			while (alphaSlaveIdToTry < SLAVE_ID_END)
			{
				alphaSlaveIdToTry++;

				_alphaSlaveID = alphaSlaveIdToTry;

				sprintf(testingHexSlaveAddress, "0x%02x", _alphaSlaveID);

				// Read the SOC register using the 
				// Generate a frame without CRC (ending 0, 0), sendModbus will do the rest
				uint8_t	frame[] = { _alphaSlaveID, MODBUS_FN_READDATAREGISTER, REG_BATTERY_HOME_R_SOC >> 8, REG_BATTERY_HOME_R_SOC & 0xff, 0, REG_BATTERY_HOME_R_SOC_REGCOUNT, 0, 0 };

				// And send to the device, it's all synchronos so by the time we get a response we will know if success or failure
				result = sendModbus(frame, sizeof(frame), &response);

				thisWorked = (result == modbusRequestAndResponseStatusValues::readDataRegisterSuccess);
				if (thisWorked)
				{
					sprintf(successfulHexSlaveAddress, testingHexSlaveAddress);
					sprintf(successfulBaudRate, testingBaudRate);
				}
				found = found || thisWorked;


	#ifdef OUTPUT_SERIAL
				sprintf(_debugOutput, "Baud Rate:\t%s\t%s", testingBaudRate, thisWorked ? "YES" : "NO");
				Serial.println(_debugOutput);
				sprintf(_debugOutput, "Slave ID:\t%s\t%s", testingHexSlaveAddress, thisWorked ? "YES" : "NO");
				Serial.println(_debugOutput);
	#endif

	#ifdef OUTPUT_DISPLAY
				sprintf(line2Contents, "%s", testingBaudRate);
				sprintf(line3Contents, "%s:%s", testingHexSlaveAddress, thisWorked ? "YES" : "NO");
				sprintf(line4Contents, "%s:%s", successfulHexSlaveAddress, found ? "YES" : "NO");

				updateOLED(false, line2Contents, line3Contents, found ? line4Contents : "");
	#endif

	#ifdef OUTPUT_WIFIANDMQTT
				sprintf(stateAddition, "\"%s\":\"%s\",\r\n\"baudRate\":\"%s\"\r\n", testingHexSlaveAddress, thisWorked ? "Y" : "N", testingBaudRate);
				// Let the onward process also know if the buffer failed.
				resultAddedToPayload = addToPayload(stateAddition);
				if (resultAddedToPayload == modbusRequestAndResponseStatusValues::payloadExceededCapacity)
				{
					// If the response to addStateInfo is payload exceeded get out, We will permit failing registers to carry on and try the next one.
					break;
				}
	#endif


				if (alphaSlaveIdToTry == SLAVE_ID_END)
				{
					// Incrementing another would cause a wrap back to 0 from 255.
					break;
				}
			}

		}




#ifdef OUTPUT_WIFIANDMQTT
		if (resultAddedToPayload != modbusRequestAndResponseStatusValues::payloadExceededCapacity)
		{
			sprintf(stateAddition, "\"found\":\"%s\"\r\n\"successfulSlaveId\":\"%s\",\r\n\"baudRate\":\"%s\"\r\n", found ? "Y" : "N", successfulHexSlaveAddress, successfulBaudRate);

			// Let the onward process also know if the buffer failed.
			resultAddedToPayload = addToPayload(stateAddition);
			if (resultAddedToPayload == modbusRequestAndResponseStatusValues::payloadExceededCapacity)
			{
				// Footer, if we haven't already bombed out with payload issues
				resultAddedToPayload = addToPayload("}");
			}
		}

		// Make sure WiFi is good
		if (WiFi.status() != WL_CONNECTED)
		{
			setupWifi();
		}

		// make sure mqtt is still connected
		if ((!_mqtt.connected()) || !_mqtt.loop())
		{
			mqttReconnect();
		}

		// And send
		sendMqtt(topic);
#endif

#ifdef OUTPUT_DISPLAY
		updateOLED(false, line2Contents, line3Contents, found ? line4Contents : "");
#endif

#ifdef OUTPUT_SERIAL
		if (found)
		{
			sprintf(_debugOutput, "Found:\t%s, Baud:\t%s", successfulHexSlaveAddress, successfulBaudRate);
		}
		else
		{
			sprintf(_debugOutput, "Not found...");
		}
		Serial.println(_debugOutput);
#endif

	}

	else
	{
#ifdef DEBUG
		Serial.println("Invalid buffer");
#endif
	}

	// And wait for 20 seconds until try again
	delay(DISPLAY_FINAL_OUTPUT_FOR);
}




/*
sendMqtt

Sends whatever is in the modular level payload to the specified topic.
*/
#ifdef OUTPUT_WIFIANDMQTT
void sendMqtt(char* topic)
{
	// Attempt a send
	if (!_mqtt.publish(topic, _mqttPayload))
	{
#ifdef DEBUG
		sprintf(_debugOutput, "MQTT publish failed to %s", topic);
		Serial.println(_debugOutput);
		Serial.println(_mqttPayload);
#endif
	}
	else
	{
#ifdef DEBUG
		//sprintf(_debugOutput, "MQTT publish success");
		//Serial.println(_debugOutput);
#endif
	}

	// Empty payload for next use.
	emptyPayload();
	return;
}
#endif

/*
emptyPayload

Clears every char so end of string can be easily found
*/
#ifdef OUTPUT_WIFIANDMQTT
void emptyPayload()
{
	for (int i = 0; i < _maxPayloadSize; i++)
	{
		_mqttPayload[i] = '\0';
	}
}
#endif


/*
uint32_t freeMemory()
{

	return ESP.getFreeHeap();
}
*/