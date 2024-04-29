/*
Name:		Definitions.h
Created:	24/Aug/2022
Author:		Daniel Young

This file is part of Alpha2MQTT (A2M) which is released under GNU GENERAL PUBLIC LICENSE.
See file LICENSE or go to https://choosealicense.com/licenses/gpl-3.0/ for full license details.

Notes

Customise these options as per README.txt.  Please read README.txt before continuing.
*/

#ifndef _Definitions_h
#define _Definitions_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


// Compiling for ESP8266 or ESP32?
//#define MP_ESP32
#define MP_ESP8266
#if (!defined MP_ESP8266) && (!defined MP_ESP32)
#error You must specify the microprocessor in use
#endif

#if (defined MP_ESP8266) && (defined MP_ESP32)
#error You must only select one microprocessor from the list
#endif


// If values for some registers such as voltage or temperatures appear to be out by a decimal place or two, try the following:
// Documentation declares 1V - However Presume 0.1 as result appears to reflect this.  I.e. my voltage reading was 2421, * 0.1 for 242.1
#define GRID_VOLTAGE_MULTIPLIER 0.1
// Documentation declares 0.001V - My min cell voltage is reading as 334, so * 0.001 = 0.334V.  I consider the document wrong, think it should be 0.01
#define CELL_VOLTAGE_MULTIPLIER 0.01
// Documentation declares 0.1D - Mine returns 2720, so assuming actually multiplied by 0.01 to bring to something realistic
#define INVERTER_TEMP_MULTIPLIER 0.01
// Documentation declares 0.1kWh - My value was 308695, and according to web interface my total PV is 3086kWh, so multiplier seems wrong
#define TOTAL_ENERGY_MUTLIPLIER 0.01


// After some liaison with a user of Alpha2MQTT on a 115200 baud rate, this fixed inconsistent retrieval
// such as sporadic NO-RESP.  It works by introducing a delay between requests sent to the inverter meaning it
// has time to 'breathe'
// 80ms is a default starting point which is 1/12 of a second.  If it corrects the issue try reducing the delay to 60, 40, etc until you find a happy place.
// If you want to make use of it, uncomment the next line and change 80 as necessary
//#define REQUIRE_DELAY_DUE_TO_INCONSISTENT_RETRIEVAL
#define REQUIRED_DELAY_DUE_TO_INCONSISTENT_RETRIEVAL 80

// Update with your Wifi details
#define WIFI_SSID		"Stardust"
#define WIFI_PASSWORD	""

// Update with your MQTT Broker details
#define MQTT_SERVER	"192.168.1.135"
#define MQTT_PORT	1883
#define MQTT_USERNAME	"Alpha"			// Empty string for none.
#define MQTT_PASSWORD	"Inverter1"


// The device name is used as the MQTT base topic and presence on the network.
// If you need more than one Alpha2MQTT on your network, give them unique names.
#define DEVICE_NAME "Alpha2MQTT"

// If your OLED doesn't have an RST pin, set this to true.
// An OLED Shield compatible with an ESP8266 does have a RESET pin and it is linked to GPIO0 if using an ESP8266.
// If you are using the same OLED Shield with an ESP32, by default for this project it is linked to GIO13.
//#define OLED_HAS_RST_PIN true

// Default address of inverter is 0x55 as per Alpha Modbus documentation.  If you have altered it, reflect that change here.
#define ALPHA_SLAVE_ID 0x55

// The ESP8266 has limited memory and so reserving lots of RAM to build a payload and MQTT buffer causes out of memory exceptions.
// 4096 works well given the RAM requirements of Alpha2MQTT.
// If you aren't using an ESP8266 you may be able to increase this.
// At 4096 (4095 usable) you can request up to around 70 to 80 registers on a schedule or by request.
// Alpha2MQTT on boot will request a buffer size of (MAX_MQTT_PAYLOAD_SIZE + MQTT_HEADER_SIZE) for MQTT, and
// MAX_MQTT_PAYLOAD_SIZE for building payloads.  If these fail and your device doesn't boot, you can assume you've set this too high.
#define MAX_MQTT_PAYLOAD_SIZE 4096
#define MIN_MQTT_PAYLOAD_SIZE 512
#define MQTT_HEADER_SIZE 512


// x 50mS to wait for RS485 input chars.  300ms as per Modbus documentation, but I got timeouts on that.  However 400ms works without issue
#define RS485_TRIES 8 // 16

// I beg to differ on this, I'd say it's more 0.396 based on my tests
// However make it easily customisable here
#define DISPATCH_SOC_MULTIPLIER 0.4


// A user informed me that their router leverages leases on network connections which can't be disabled.
// I.e. when lease expires, WiFi doesn't drop but data stops.
// If FORCE_RESTART is defined, then the system will auto-reboot every X many hours as configured in FORCE_RESTART_HOURS
//#define FORCE_RESTART
#define FORCE_RESTART_HOURS 49


//#if (!defined INVERTER_SMILE_B3) && (!defined INVERTER_SMILE5) && (!defined INVERTER_SMILE_T10) && (!defined INVERTER_STORION_T30)
//#error You must specify the inverter type.
//#endif




// Handled Registers as per 1.23 documentation
// Network meter - configuration
#define REG_GRID_METER_RW_GRID_METER_CT_ENABLE										0x0000	// 1/bit								// 2 Bytes		// Unsigned Short
#define REG_GRID_METER_RW_GRID_METER_CT_RATE										0x0001	// 1/bit								// 2 Bytes		// Unsigned Short

// Network meter - operation						
#define REG_GRID_METER_R_TOTAL_ENERGY_FEED_TO_GRID_1								0x0010	// 0.01kWh/bit							// 4 Bytes		// Unsigned Integer
//#define REG_GRID_METER_R_TOTAL_ENERGY_FEED_TO_GRID_2								0x0011	// 0.01kWh/bit						
#define REG_GRID_METER_R_TOTAL_ENERGY_CONSUMED_FROM_GRID_1							0x0012	// 0.01kWh/bit							// 4 Bytes		// Unsigned Integer
//#define REG_GRID_METER_R_TOTAL_ENERGY_CONSUMED_FROM_GRID_2							0x0013	// 0.01kWh/bit						
#define REG_GRID_METER_R_VOLTAGE_OF_A_PHASE											0x0014	// 1V									// 2 Bytes		// Unsigned Short
#define REG_GRID_METER_R_VOLTAGE_OF_B_PHASE											0x0015	// 1V									// 2 Bytes		// Unsigned Short
#define REG_GRID_METER_R_VOLTAGE_OF_C_PHASE											0x0016	// 1V									// 2 Bytes		// Unsigned Short
#define REG_GRID_METER_R_CURRENT_OF_A_PHASE											0x0017	// 0.1A									// 2 Bytes		// Short
#define REG_GRID_METER_R_CURRENT_OF_B_PHASE											0x0018	// 0.1A									// 2 Bytes		// Short
#define REG_GRID_METER_R_CURRENT_OF_C_PHASE											0x0019	// 0.1A									// 2 Bytes		// Short
#define REG_GRID_METER_R_FREQUENCY													0x001A	// 0.01Hz								// 2 Bytes		// Unsigned Short
#define REG_GRID_METER_R_ACTIVE_POWER_OF_A_PHASE_1									0x001B	// 1W/bit								// 4 Bytes		// Integer
//#define REG_GRID_METER_R_ACTIVE_POWER_OF_A_PHASE_2									0x001C	// 1W/bit						
#define REG_GRID_METER_R_ACTIVE_POWER_OF_B_PHASE_1									0x001D	// 1W/bit								// 4 Bytes		// Integer
//#define REG_GRID_METER_R_ACTIVE_POWER_OF_B_PHASE_2									0x001E	// 1W/bit						
#define REG_GRID_METER_R_ACTIVE_POWER_OF_C_PHASE_1									0x001F	// 1W/bit								// 4 Bytes		// Integer
//#define REG_GRID_METER_R_ACTIVE_POWER_OF_C_PHASE_2									0x0020	// 1W/bit						
#define REG_GRID_METER_R_TOTAL_ACTIVE_POWER_1										0x0021	// 1W/bit								// 4 Bytes		// Integer
//#define REG_GRID_METER_R_TOTAL_ACTIVE_POWER_2										0x0022	// 1W/bit						
#define REG_GRID_METER_R_REACTIVE_POWER_OF_A_PHASE_1								0x0023	// 1var									// 4 Bytes		// Integer
//#define REG_GRID_METER_R_REACTIVE_POWER_OF_A_PHASE_2								0x0024	// 1var						
#define REG_GRID_METER_R_REACTIVE_POWER_OF_B_PHASE_1								0x0025	// 1var									// 4 Bytes		// Integer
//#define REG_GRID_METER_R_REACTIVE_POWER_OF_B_PHASE_2								0x0026	// 1var						
#define REG_GRID_METER_R_REACTIVE_POWER_OF_C_PHASE_1								0x0027	// 1var									// 4 Bytes		// Integer
//#define REG_GRID_METER_R_REACTIVE_POWER_OF_C_PHASE_2								0x0028	// 1var						
#define REG_GRID_METER_R_TOTAL_REACTIVE_POWER_1										0x0029	// 1var									// 4 Bytes		// Integer
//#define REG_GRID_METER_R_TOTAL_REACTIVE_POWER_2										0x002A	// 1var						
#define REG_GRID_METER_R_APPARENT_POWER_OF_A_PHASE_1								0x002B	// 1VA									// 4 Bytes		// Integer
//#define REG_GRID_METER_R_APPARENT_POWER_OF_A_PHASE_2								0x002C	// 1VA						
#define REG_GRID_METER_R_APPARENT_POWER_OF_B_PHASE_1								0x002D	// 1VA									// 4 Bytes		// Integer
//#define REG_GRID_METER_R_APPARENT_POWER_OF_B_PHASE_2								0x002E	// 1VA						
#define REG_GRID_METER_R_APPARENT_POWER_OF_C_PHASE_1								0x002F	// 1VA									// 4 Bytes		// Integer
//#define REG_GRID_METER_R_APPARENT_POWER_OF_C_PHASE_2								0x0030	// 1VA						
#define REG_GRID_METER_R_TOTAL_APPARENT_POWER_1										0x0031	// 1VA									// 4 Bytes		// Integer
//#define REG_GRID_METER_R_TOTAL_APPARENT_POWER_2										0x0032	// 1VA						
#define REG_GRID_METER_R_POWER_FACTOR_OF_A_PHASE									0x0033	// 0.01									// 2 Bytes		// Short
#define REG_GRID_METER_R_POWER_FACTOR_OF_B_PHASE									0x0034	// 0.01									// 2 Bytes		// Short
#define REG_GRID_METER_R_POWER_FACTOR_OF_C_PHASE									0x0035	// 0.01									// 2 Bytes		// Short
#define REG_GRID_METER_R_TOTAL_POWER_FACTOR											0x0036	// 0.01									// 2 Bytes		// Short

// PV meter - configuration						
#define REG_PV_METER_RW_PV_METER_CT_ENABLE											0x0080	// 1/bit								// 2 Bytes		// Unsigned Short
#define REG_PV_METER_RW_PV_METER_CT_RATE											0x0081	// 1/bit								// 2 Bytes		// Unsigned Short

// PV meter - operation						
#define REG_PV_METER_R_TOTAL_ENERGY_FEED_TO_GRID_1									0x0090	// 0.01kWh/bit							// 4 Bytes		// Unsigned Integer
//#define REG_PV_METER_R_TOTAL_ENERGY_FEED_TO_GRID_2									0x0091	// 0.01kWh/bit						
#define REG_PV_METER_R_TOTAL_ENERGY_CONSUMED_FROM_GRID_1							0x0092	// 0.01kWh/bit							// 4 Bytes		// Unsigned Integer
//#define REG_PV_METER_R_TOTAL_ENERGY_CONSUMED_FROM_GRID_2							0x0093	// 0.01kWh/bit						
#define REG_PV_METER_R_VOLTAGE_OF_A_PHASE											0x0094	// 1V									// 2 Bytes		// Unsigned Short
#define REG_PV_METER_R_VOLTAGE_OF_B_PHASE											0x0095	// 1V									// 2 Bytes		// Unsigned Short
#define REG_PV_METER_R_VOLTAGE_OF_C_PHASE											0x0096	// 1V									// 2 Bytes		// Unsigned Short
#define REG_PV_METER_R_CURRENT_OF_A_PHASE											0x0097	// 0.1A									// 2 Bytes		// Short
#define REG_PV_METER_R_CURRENT_OF_B_PHASE											0x0098	// 0.1A									// 2 Bytes		// Short
#define REG_PV_METER_R_CURRENT_OF_C_PHASE											0x0099	// 0.1A									// 2 Bytes		// Short
#define REG_PV_METER_R_FREQUENCY													0x009A	// 0.01Hz								// 2 Bytes		// Unsigned Short
#define REG_PV_METER_R_ACTIVE_POWER_OF_A_PHASE_1									0x009B	// 1W/bit								// 4 Bytes		// Integer
//#define REG_PV_METER_R_ACTIVE_POWER_OF_A_PHASE_2									0x009C	// 1W/bit						
#define REG_PV_METER_R_ACTIVE_POWER_OF_B_PHASE_1									0x009D	// 1W/bit								// 4 Bytes		// Integer
//#define REG_PV_METER_R_ACTIVE_POWER_OF_B_PHASE_2									0x009E	// 1W/bit						
#define REG_PV_METER_R_ACTIVE_POWER_OF_C_PHASE_1									0x009F	// 1W/bit								// 4 Bytes		// Integer
//#define REG_PV_METER_R_ACTIVE_POWER_OF_C_PHASE_2									0x00A0	// 1W/bit						
#define REG_PV_METER_R_TOTAL_ACTIVE_POWER_1											0x00A1	// 1W/bit								// 4 Bytes		// Integer
//#define REG_PV_METER_R_TOTAL_ACTIVE_POWER_2											0x00A2	// 1W/bit						
#define REG_PV_METER_R_REACTIVE_POWER_OF_A_PHASE_1									0x00A3	// 1var									// 4 Bytes		// Integer
//#define REG_PV_METER_R_REACTIVE_POWER_OF_A_PHASE_2									0x00A4	// 1var						
#define REG_PV_METER_R_REACTIVE_POWER_OF_B_PHASE_1									0x00A5	// 1var									// 4 Bytes		// Integer
//#define REG_PV_METER_R_REACTIVE_POWER_OF_B_PHASE_2									0x00A6	// 1var						
#define REG_PV_METER_R_REACTIVE_POWER_OF_C_PHASE_1									0x00A7	// 1var									// 4 Bytes		// Integer
//#define REG_PV_METER_R_REACTIVE_POWER_OF_C_PHASE_2									0x00A8	// 1var						
#define REG_PV_METER_R_TOTAL_REACTIVE_POWER_1										0x00A9	// 1var									// 4 Bytes		// Integer
//#define REG_PV_METER_R_TOTAL_REACTIVE_POWER_2										0x00AA	// 1var						
#define REG_PV_METER_R_APPARENT_POWER_OF_A_PHASE_1									0x00AB	// 1VA									// 4 Bytes		// Integer
//#define REG_PV_METER_R_APPARENT_POWER_OF_A_PHASE_2									0x00AC	// 1VA						
#define REG_PV_METER_R_APPARENT_POWER_OF_B_PHASE_1									0x00AD	// 1VA									// 4 Bytes		// Integer
//#define REG_PV_METER_R_APPARENT_POWER_OF_B_PHASE_2									0x00AE	// 1VA						
#define REG_PV_METER_R_APPARENT_POWER_OF_C_PHASE_1									0x00AF	// 1VA									// 4 Bytes		// Integer
//#define REG_PV_METER_R_APPARENT_POWER_OF_C_PHASE_2									0x00B0	// 1VA						
#define REG_PV_METER_R_TOTAL_APPARENT_POWER_1										0x00B1	// 1VA									// 4 Bytes		// Integer
//#define REG_PV_METER_R_TOTAL_APPARENT_POWER_2										0x00B2	// 1VA						
#define REG_PV_METER_R_POWER_FACTOR_OF_A_PHASE										0x00B3	// 0.01									// 2 Bytes		// Short
#define REG_PV_METER_R_POWER_FACTOR_OF_B_PHASE										0x00B4	// 0.01									// 2 Bytes		// Short
#define REG_PV_METER_R_POWER_FACTOR_OF_C_PHASE										0x00B5	// 0.01									// 2 Bytes		// Short
#define REG_PV_METER_R_TOTAL_POWER_FACTOR											0x00B6	// 0.01									// 2 Bytes		// Short

// Battery - HOME Series						
#define REG_BATTERY_HOME_R_VOLTAGE													0x0100	// 0.1V/bit								// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_CURRENT													0x0101	// 0.1A/bit								// 2 Bytes		// Short
#define REG_BATTERY_HOME_R_SOC														0x0102	// 0.1/bit								// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_STATUS													0x0103	// <<Note1 - BATTERY STATUS LOOKUP>>	// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_RELAY_STATUS												0x0104	// <<Note2 - BATTERY RELAY STATUS LU>>	// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_PACK_ID_OF_MIN_CELL_VOLTAGE								0x0105	// 0.001V/bit							// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_CELL_ID_OF_MIN_CELL_VOLTAGE								0x0106	// 0.001V/bit							// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_MIN_CELL_VOLTAGE											0x0107	// 0.001V/bit							// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_PACK_ID_OF_MAX_CELL_VOLTAGE								0x0108	// 0.001V/bit							// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_CELL_ID_OF_MAX_CELL_VOLTAGE								0x0109	// 0.001V/bit							// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_MAX_CELL_VOLTAGE											0x010A	// 0.001V/bit							// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_PACK_ID_OF_MIN_CELL_TEMPERATURE							0x010B	// 0.001D/bit							// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_CELL_ID_OF_MIN_CELL_TEMPERATURE							0x010C	// 0.001D/bit							// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_MIN_CELL_TEMPERATURE										0x010D	// 0.001D/bit							// 2 Bytes		// Short
#define REG_BATTERY_HOME_R_PACK_ID_OF_MAX_CELL_TEMPERATURE							0x010E	// 0.001D/bit							// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_CELL_ID_OF_MAX_CELL_TEMPERATURE							0x010F	// 0.001D/bit							// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_MAX_CELL_TEMPERATURE										0x0110	// 0.001D/bit							// 2 Bytes		// Short
#define REG_BATTERY_HOME_R_MAX_CHARGE_CURRENT										0x0111	// 0.1A/bit								// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_MAX_DISCHARGE_CURRENT									0x0112	// 0.1A/bit								// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_CHARGE_CUT_OFF_VOLTAGE									0x0113	// 0.1V/bit								// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_DISCHARGE_CUT_OFF_VOLTAGE								0x0114	// 0.1V/bit								// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_BMU_SOFTWARE_VERSION										0x0115	//										// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_LMU_SOFTWARE_VERSION										0x0116	//										// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_ISO_SOFTWARE_VERSION										0x0117	//										// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_BATTERY_NUMBER											0x0118	// Battery modules number				// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_BATTERY_CAPACITY											0x0119	// 0.1kWh/bit							// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_BATTERY_TYPE												0x011A	// <<Note3 - BATTERY TYPE LOOKUP>>		// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_BATTERY_SOH												0x011B	// 0.1/bit								// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_BATTERY_WARNING_1										0x011C	// Reserve								// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_WARNING_2										0x011D	// Reserve
#define REG_BATTERY_HOME_R_BATTERY_FAULT_1											0x011E	// <<Note4 - BATTERY ERROR LOOKUP>>		// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_FAULT_2											0x011F	// <<Note4 - BATTERY ERROR LOOKUP>>
#define REG_BATTERY_HOME_R_BATTERY_CHARGE_ENERGY_1									0x0120	// 0.1kWh/bit							// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_CHARGE_ENERGY_2									0x0121	// 0.1kWh/bit
#define REG_BATTERY_HOME_R_BATTERY_DISCHARGE_ENERGY_1								0x0122	// 0.1kWh/bit							// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_DISCHARGE_ENERGY_2								0x0123	// 0.1kWh/bit
#define REG_BATTERY_HOME_R_BATTERY_ENERGY_CHARGE_FROM_GRID_1						0x0124	// 0.1kWh/bit							// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_ENERGY_CHARGE_FROM_GRID_2						0x0125	// 0.1kWh/bit
#define REG_BATTERY_HOME_R_BATTERY_POWER											0x0126	// 1W/bit, - Charge, + Discharge		// 2 Bytes		// Short
#define REG_BATTERY_HOME_R_BATTERY_REMAINING_TIME									0x0127	// 1 minute/bit							// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_BATTERY_IMPLEMENTATION_CHARGE_SOC						0x0128	// 0.1/bit (Rate_SOC UPS_SOC)			// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_BATTERY_IMPLEMENTATION_DISCHARGE_SOC						0x0129	// 0.1/bit (Rate_SOC UPS_SOC)			// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_BATTERY_REMAINING_CHARGE_SOC								0x012A	// 0.1/bit (Rate_SOC Remain_SOC)		// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_BATTERY_REMAINING_DISCHARGE_SOC							0x012B	// 0.1/bit (Remain_SOC UPS_SOC)			// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_BATTERY_MAX_CHARGE_POWER									0x012C	// 1W/bit								// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_BATTERY_MAX_DISCHARGE_POWER								0x012D	// 1W/bit								// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_RW_BATTERY_MOS_CONTROL										0x012E	// <<BATTERY MOS CONTROL LOOKUP>>		// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_BATTERY_SOC_CALIBRATION									0x012F	// <<BATTERY SOC CALIBRATION LOOKUP>>	// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_BATTERY_SINGLE_CUT_ERROR_CODE							0x0130	// 										// 2 Bytes		// Unsigned Short
#define REG_BATTERY_HOME_R_BATTERY_FAULT_1_1										0x0131	// 										// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_FAULT_1_2										0x0132	//
#define REG_BATTERY_HOME_R_BATTERY_FAULT_2_1										0x0133	// 										// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_FAULT_2_2										0x0134	//
#define REG_BATTERY_HOME_R_BATTERY_FAULT_3_1										0x0135	// 										// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_FAULT_3_2										0x0136	//
#define REG_BATTERY_HOME_R_BATTERY_FAULT_4_1										0x0137	// 										// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_FAULT_4_2										0x0138	//
#define REG_BATTERY_HOME_R_BATTERY_FAULT_5_1										0x0139	// 										// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_FAULT_5_2										0x013A	//
#define REG_BATTERY_HOME_R_BATTERY_FAULT_6_1										0x013B	// 										// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_FAULT_6_2										0x013C	//
#define REG_BATTERY_HOME_R_BATTERY_WARNING_1_1										0x013D	// 										// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_WARNING_1_2										0x013E	//
#define REG_BATTERY_HOME_R_BATTERY_WARNING_2_1										0x013F	// 										// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_WARNING_2_2										0x0140	//
#define REG_BATTERY_HOME_R_BATTERY_WARNING_3_1										0x0141	// 										// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_WARNING_3_2										0x0142	//
#define REG_BATTERY_HOME_R_BATTERY_WARNING_4_1										0x0143	// 										// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_WARNING_4_2										0x0144	//
#define REG_BATTERY_HOME_R_BATTERY_WARNING_5_1										0x0145	// 										// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_WARNING_5_2										0x0146	//
#define REG_BATTERY_HOME_R_BATTERY_WARNING_6_1										0x0147	// 										// 4 Bytes		// Unsigned Integer
//#define REG_BATTERY_HOME_R_BATTERY_WARNING_6_2										0x0148	//

// Inverter - HOME Series	
#define REG_INVERTER_HOME_R_VOLTAGE_L1												0x0400	// 0.1V/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_VOLTAGE_L2												0x0401	// 0.1V/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_VOLTAGE_L3												0x0402	// 0.1V/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_CURRENT_L1												0x0403	// 0.1A/bit								// 2 Bytes		// Short
#define REG_INVERTER_HOME_R_CURRENT_L2												0x0404	// 0.1A/bit								// 2 Bytes		// Short
#define REG_INVERTER_HOME_R_CURRENT_L3												0x0405	// 0.1A/bit								// 2 Bytes		// Short
#define REG_INVERTER_HOME_R_POWER_L1_1												0x0406	// 1W/bit								// 4 Bytes		// Integer
//#define REG_INVERTER_HOME_R_POWER_L1_2												0x0407	// 1W/bit
#define REG_INVERTER_HOME_R_POWER_L2_1												0x0408	// 1W/bit								// 4 Bytes		// Integer
//#define REG_INVERTER_HOME_R_POWER_L2_2												0x0409	// 1W/bit
#define REG_INVERTER_HOME_R_POWER_L3_1												0x040A	// 1W/bit								// 4 Bytes		// Integer
//#define REG_INVERTER_HOME_R_POWER_L3_2												0x040B	// 1W/bit
#define REG_INVERTER_HOME_R_POWER_TOTAL_1											0x040C	// 1W/bit								// 4 Bytes		// Integer
//#define REG_INVERTER_HOME_R_POWER_TOTAL_2											0x040D	// 1W/bit
#define REG_INVERTER_HOME_R_BACKUP_VOLTAGE_L1										0x040E	// 0.1V/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_BACKUP_VOLTAGE_L2										0x040F	// 0.1V/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_BACKUP_VOLTAGE_L3										0x0410	// 0.1V/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_BACKUP_CURRENT_L1										0x0411	// 0.1A/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_BACKUP_CURRENT_L2										0x0412	// 0.1A/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_BACKUP_CURRENT_L3										0x0413	// 0.1A/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_BACKUP_POWER_L1_1										0x0414	// 1W/bit								// 4 Bytes		// Unsigned Integer
//#define REG_INVERTER_HOME_R_BACKUP_POWER_L1_2										0x0415	// 1W/bit
#define REG_INVERTER_HOME_R_BACKUP_POWER_L2_1										0x0416	// 1W/bit								// 4 Bytes		// Unsigned Integer
//#define REG_INVERTER_HOME_R_BACKUP_POWER_L2_2										0x0417	// 1W/bit
#define REG_INVERTER_HOME_R_BACKUP_POWER_L3_1										0x0418	// 1W/bit								// 4 Bytes		// Unsigned Integer
//#define REG_INVERTER_HOME_R_BACKUP_POWER_L3_2										0x0419	// 1W/bit
#define REG_INVERTER_HOME_R_BACKUP_POWER_TOTAL_1									0x041A	// 1W/bit								// 4 Bytes		// Unsigned Integer
//#define REG_INVERTER_HOME_R_BACKUP_POWER_TOTAL_2									0x041B	// 1W/bit
#define REG_INVERTER_HOME_R_FREQUENCY												0x041C	// 0.1Hz/bit							// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_PV1_VOLTAGE												0x041D	// 0.1V/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_PV1_CURRENT												0x041E	// 0.1A/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_PV1_POWER_1												0x041F	// 1W/bit								// 4 Bytes		// Unsigned Integer
//#define REG_INVERTER_HOME_R_PV1_POWER_2												0x0420	// 1W/bit
#define REG_INVERTER_HOME_R_PV2_VOLTAGE												0x0421	// 0.1V/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_PV2_CURRENT												0x0422	// 0.1A/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_PV2_POWER_1												0x0423	// 1W/bit								// 4 Bytes		// Unsigned Integer
//#define REG_INVERTER_HOME_R_PV2_POWER_2												0x0424	// 1W/bit
#define REG_INVERTER_HOME_R_PV3_VOLTAGE												0x0425	// 0.1V/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_PV3_CURRENT												0x0426	// 0.1A/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_PV3_POWER_1												0x0427	// 1W/bit								// 4 Bytes		// Unsigned Integer
//#define REG_INVERTER_HOME_R_PV3_POWER_2												0x0428	// 1W/bit
#define REG_INVERTER_HOME_R_PV4_VOLTAGE												0x0429	// 0.1V/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_PV4_CURRENT												0x042A	// 0.1A/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_PV4_POWER_1												0x042B	// 1W/bit								// 4 Bytes		// Unsigned Integer
//#define REG_INVERTER_HOME_R_PV4_POWER_2												0x042C	// 1W/bit
#define REG_INVERTER_HOME_R_PV5_VOLTAGE												0x042D	// 0.1V/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_PV5_CURRENT												0x042E	// 0.1A/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_PV5_POWER_1												0x042F	// 1W/bit								// 4 Bytes		// Unsigned Integer
//#define REG_INVERTER_HOME_R_PV5_POWER_2												0x0430	// 1W/bit
#define REG_INVERTER_HOME_R_PV6_VOLTAGE												0x0431	// 0.1V/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_PV6_CURRENT												0x0432	// 0.1A/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_PV6_POWER_1												0x0433	// 1W/bit								// 4 Bytes		// Unsigned Integer
//#define REG_INVERTER_HOME_R_PV6_POWER_2												0x0434	// 1W/bit
#define REG_INVERTER_HOME_R_INVERTER_TEMP											0x0435	// 0.1D/bit								// 2 Bytes		// Unsigned Short
#define REG_INVERTER_HOME_R_INVERTER_WARNING_1_1									0x0436	// Reserve								// 4 Bytes		// Unsigned Integer
//#define REG_INVERTER_HOME_R_INVERTER_WARNING_1_2									0x0437	// Reserve
#define REG_INVERTER_HOME_R_INVERTER_WARNING_2_1									0x0438	// Reserve								// 4 Bytes		// Unsigned Integer
//#define REG_INVERTER_HOME_R_INVERTER_WARNING_2_2									0x0439	// Reserve
#define REG_INVERTER_HOME_R_INVERTER_FAULT_1_1										0x043A	// Reserve								// 4 Bytes		// Unsigned Integer
//#define REG_INVERTER_HOME_R_INVERTER_FAULT_1_2										0x043B	// Reserve
#define REG_INVERTER_HOME_R_INVERTER_FAULT_2_1										0x043C	// Reserve								// 4 Bytes		// Unsigned Integer
//#define REG_INVERTER_HOME_R_INVERTER_FAULT_2_2										0x043D	// Reserve
#define REG_INVERTER_HOME_R_INVERTER_TOTAL_PV_ENERGY_1								0x043E	// 0.1kWh/bit							// 4 Bytes		// Unsigned Integer
//#define REG_INVERTER_HOME_R_INVERTER_TOTAL_PV_ENERGY_2								0x043F	// 0.1kWh/bit
#define REG_INVERTER_HOME_R_WORKING_MODE											0x0440	// <<Note5 - INVERTER OPERATION LOOKUP>>// 2 Bytes		// Unsigned Short

// Inverter Information
																					// labelled in documentation as starting 0x0680, believe this wrong.
#define REG_INVERTER_INFO_R_MASTER_SOFTWARE_VERSION_1								0x0640	//										// 10 Bytes		// Unsigned Char
//#define REG_INVERTER_INFO_R_MASTER_SOFTWARE_VERSION_2								0x0641	//
//#define REG_INVERTER_INFO_R_MASTER_SOFTWARE_VERSION_3								0x0642	//
//#define REG_INVERTER_INFO_R_MASTER_SOFTWARE_VERSION_4								0x0643	//
//#define REG_INVERTER_INFO_R_MASTER_SOFTWARE_VERSION_5								0x0644	//
#define REG_INVERTER_INFO_R_SLAVE_SOFTWARE_VERSION_1								0x0645	//										// 10 Bytes		// Unsigned Char
//#define REG_INVERTER_INFO_R_SLAVE_SOFTWARE_VERSION_2								0x0646	//
//#define REG_INVERTER_INFO_R_SLAVE_SOFTWARE_VERSION_3								0x0647	//
//#define REG_INVERTER_INFO_R_SLAVE_SOFTWARE_VERSION_4								0x0648	//
//#define REG_INVERTER_INFO_R_SLAVE_SOFTWARE_VERSION_5								0x0649	//
#define REG_INVERTER_INFO_R_SERIAL_NUMBER_1											0x064A	//										// 20 Bytes		// Unsigned Char
//#define REG_INVERTER_INFO_R_SERIAL_NUMBER_2											0x064B	//
//#define REG_INVERTER_INFO_R_SERIAL_NUMBER_3											0x064C	//
//#define REG_INVERTER_INFO_R_SERIAL_NUMBER_4											0x064D	//
//#define REG_INVERTER_INFO_R_SERIAL_NUMBER_5											0x064E	//
//#define REG_INVERTER_INFO_R_SERIAL_NUMBER_6											0x065F	//
//#define REG_INVERTER_INFO_R_SERIAL_NUMBER_7											0x0650	//
//#define REG_INVERTER_INFO_R_SERIAL_NUMBER_8											0x0651	//
//#define REG_INVERTER_INFO_R_SERIAL_NUMBER_9											0x0652	//
//#define REG_INVERTER_INFO_R_SERIAL_NUMBER_10										0x0653	//


// ***********
// Ignore System(Only applicable to HHE MEC)

// ***********
// Ignore Echonet Config (Japan)


// System Information
#define REG_SYSTEM_INFO_RW_SYSTEM_TIME_YEAR_MONTH									0x0740	// 0xYYMM, 0x1109 = 2017/09				// 2 Bytes		// Unsigned Short
#define REG_SYSTEM_INFO_RW_SYSTEM_TIME_DAY_HOUR										0x0741	// 0xDDHH, 0x1109 = 17th Day/9th Hour	// 2 Bytes		// Unsigned Short
#define REG_SYSTEM_INFO_RW_SYSTEM_TIME_MINUTE_SECOND								0x0742	// 0xmmss, 0x1109 = 17th Min/9th Sec	// 2 Bytes		// Unsigned Short

#define REG_SYSTEM_INFO_R_EMS_SN_BYTE_1_2											0x0743	// EMS SN: ASCII 0x414C=='AL'			// 2 Bytes		// Unsigned Short
//#define REG_SYSTEM_INFO_R_EMS_SN_BYTE_3_4											0x0744	// EMS SN: ASCII 0x3132=='12'			// 2 Bytes		// Unsigned Short
//#define REG_SYSTEM_INFO_R_EMS_SN_BYTE_5_6											0x0745	// EMS SN: ASCII 0x3132=='12'			// 2 Bytes		// Unsigned Short
//#define REG_SYSTEM_INFO_R_EMS_SN_BYTE_7_8											0x0746	// EMS SN: ASCII 0x3132=='12'			// 2 Bytes		// Unsigned Short
//#define REG_SYSTEM_INFO_R_EMS_SN_BYTE_9_10											0x0747	// EMS SN: ASCII 0x3132=='12'			// 2 Bytes		// Unsigned Short
//#define REG_SYSTEM_INFO_R_EMS_SN_BYTE_11_12											0x0748	// EMS SN: ASCII 0x3132=='12'			// 2 Bytes		// Unsigned Short
//#define REG_SYSTEM_INFO_R_EMS_SN_BYTE_13_14											0x0749	// EMS SN: ASCII 0x3132=='12'			// 2 Bytes		// Unsigned Short
//#define REG_SYSTEM_INFO_R_EMS_SN_BYTE_15_16											0x074A	// EMS SN: ASCII 0x3132=='12'			// 2 Bytes		// Unsigned Short
#define REG_SYSTEM_INFO_R_EMS_VERSION_HIGH											0x074B	// 										// 2 Bytes		// Unsigned Short
#define REG_SYSTEM_INFO_R_EMS_VERSION_MIDDLE										0x074C	// 										// 2 Bytes		// Unsigned Short
#define REG_SYSTEM_INFO_R_EMS_VERSION_LOW											0x074D	// 										// 2 Bytes		// Unsigned Short
#define REG_SYSTEM_INFO_R_PROTOCOL_VERSION											0x074E	// 										// 2 Bytes		// Unsigned Short


// System Configuration
#define REG_SYSTEM_CONFIG_RW_MAX_FEED_INTO_GRID_PERCENT								0x0800	// 1%/bit								// 2 Bytes		// Unsigned Short
#define REG_SYSTEM_CONFIG_RW_PV_CAPACITY_STORAGE_1									0x0801	// 1W/bit								// 4 Bytes		// Unsigned Integer
//#define REG_SYSTEM_CONFIG_RW_PV_CAPACITY_STORAGE_2									0x0802	// 1W/bit
#define REG_SYSTEM_CONFIG_RW_PV_CAPACITY_OF_GRID_INVERTER_1							0x0803	// 1W/bit								// 4 Bytes		// Unsigned Integer
//#define REG_SYSTEM_CONFIG_RW_PV_CAPACITY_OF_GRID_INVERTER_2						0x0804	// 1W/bit
#define REG_SYSTEM_CONFIG_RW_SYSTEM_MODE											0x0805	// <<SYSTEM MODE LOOKUP>>				// 2 Bytes		// Unsigned Short
#define REG_SYSTEM_CONFIG_RW_METER_CT_SELECT										0x0806	// <<METER CT SELECT LOOKUP>>			// 2 Bytes		// Unsigned Short
#define REG_SYSTEM_CONFIG_RW_BATTERY_READY											0x0807	// <<BATTERY READY LOOKUP>>				// 2 Bytes		// Unsigned Short
#define REG_SYSTEM_CONFIG_RW_IP_METHOD												0x0808	// <<IP METHOD LOOKUP>>					// 2 Bytes		// Unsigned Short
#define REG_SYSTEM_CONFIG_RW_LOCAL_IP_1												0x0809	// 0xC0A80101 192.168.1.1				// 4 Bytes		// Unsigned Int ** Declared short, believe Int
//#define REG_SYSTEM_CONFIG_RW_LOCAL_IP_2												0x080A	// 0xC0A80101 192.168.1.1
#define REG_SYSTEM_CONFIG_RW_SUBNET_MASK_1											0x080B	// 0xFFFFFF01 255.255.255.0				// 4 Bytes		// Unsigned Int ** Declared short, believe Int
//#define REG_SYSTEM_CONFIG_RW_SUBNET_MASK_2											0x080C	// 0xFFFFFF01 255.255.255.0
#define REG_SYSTEM_CONFIG_RW_GATEWAY_1												0x080D	// 0xC0A80101 192.168.1.1				// 4 Bytes		// Unsigned Int ** Declared short, believe Int
//#define REG_SYSTEM_CONFIG_RW_GATEWAY_2												0x080E	// 0xC0A80101 192.168.1.1
#define REG_SYSTEM_CONFIG_RW_MODBUS_ADDRESS											0x080F	// default 0x55							// 2 Bytes		// Unsigned Short
#define REG_SYSTEM_CONFIG_RW_MODBUS_BAUD_RATE										0x0810	// <<MODBUS BAUD RATE LOOKUP>>			// 2 Bytes		// Unsigned Short


// Timing
#define REG_TIMING_RW_TIME_PERIOD_CONTROL_FLAG										0x084F	// <<TIME PERIOD CONTROL FLAG LOOKUP>>	// 2 Bytes		// Unsigned Short
#define REG_TIMING_RW_UPS_RESERVE_SOC												0x0850	// 0.1/bit								// 2 Bytes		// Unsigned Short
#define REG_TIMING_RW_TIME_DISCHARGE_START_TIME_1									0x0851	// 1H/bit								// 2 Bytes		// Unsigned Short
#define REG_TIMING_RW_TIME_DISCHARGE_STOP_TIME_1									0x0852	// 1H/bit								// 2 Bytes		// Unsigned Short
#define REG_TIMING_RW_TIME_DISCHARGE_START_TIME_2									0x0853	// 1H/bit								// 2 Bytes		// Unsigned Short
#define REG_TIMING_RW_TIME_DISCHARGE_STOP_TIME_2									0x0854	// 1H/bit								// 2 Bytes		// Unsigned Short
#define REG_TIMING_RW_CHARGE_CUT_SOC												0x0855	// 0.1/bit								// 2 Bytes		// Unsigned Short
#define REG_TIMING_RW_TIME_CHARGE_START_TIME_1										0x0856	// 1H/bit								// 2 Bytes		// Unsigned Short
#define REG_TIMING_RW_TIME_CHARGE_STOP_TIME_1										0x0857	// 1H/bit								// 2 Bytes		// Unsigned Short
#define REG_TIMING_RW_TIME_CHARGE_START_TIME_2										0x0858	// 1H/bit								// 2 Bytes		// Unsigned Short
#define REG_TIMING_RW_TIME_CHARGE_STOP_TIME_2										0x0859	// 1H/bit								// 2 Bytes		// Unsigned Short

// Dispatch
#define REG_DISPATCH_RW_DISPATCH_START												0x0880	// <<DISPATCH START LOOKUP>>			// 2 Bytes		// Unsigned Short
#define REG_DISPATCH_RW_ACTIVE_POWER_1												0x0881	// 1W/bit Offset: 32000 load<32000		// 4 Bytes		// Int
//#define REG_DISPATCH_RW_ACTIVE_POWER_2												0x0882	// 1W/bit Offset: 32000 load<32000	
#define REG_DISPATCH_RW_REACTIVE_POWER_1											0x0883	// 1Var/bit Offset: 32000 load<32000	// 4 Bytes		// Int
//#define REG_DISPATCH_RW_REACTIVE_POWER_2											0x0884	// 1Var/bit Offset: 32000 load<32000	
#define REG_DISPATCH_RW_DISPATCH_MODE												0x0885	// <<Note7 - DISPATCH MODE LOOKUP>>		// 2 Bytes		// Unsigned Short
#define REG_DISPATCH_RW_DISPATCH_SOC												0x0886	// 0.4%/bit, 95=SOC of 38%				// 2 Bytes		// Unsigned Short
#define REG_DISPATCH_RW_DISPATCH_TIME_1												0x0887	// 1S/bit								// 4 Bytes		// Unsigned Int
//#define REG_DISPATCH_RW_DISPATCH_TIME_2												0x0888	// 1S/bit

// Auxiliary
#define REG_AUXILIARY_W_EMS_DO0														0x08B0	// by-pass control function				// 2 Bytes		// Unsigned Short
#define REG_AUXILIARY_W_EMS_DO1														0x08B1	// System fault output					// 2 Bytes		// Unsigned Short
#define REG_AUXILIARY_R_EMS_DI0														0x08C0	// EPO, BatteryMOS cut off				// 2 Bytes		// Unsigned Short
#define REG_AUXILIARY_R_EMS_DI1														0x08C1	// Reserved								// 2 Bytes		// Unsigned Short

// System - Operational Data
#define REG_SYSTEM_OP_R_PV_INVERTER_ENERGY_1										0x08D0	// 0.1kWh/bit							// 4 Bytes		// Unsigned Int
//#define REG_SYSTEM_OP_R_PV_INVERTER_ENERGY_2										0x08D1	// 0.1kWh/bit
#define REG_SYSTEM_OP_R_SYSTEM_TOTAL_PV_ENERGY_1									0x08D2	// 0.1kWh/bit							// 4 Bytes		// Unsigned Int
//#define REG_SYSTEM_OP_R_SYSTEM_TOTAL_PV_ENERGY_2									0x08D3	// 0.1kWh/bit
#define REG_SYSTEM_OP_R_SYSTEM_FAULT_1												0x08D4	// <<Note6 - SYSTEM ERROR LOOKUP>>		// 4 Bytes		// Unsigned Int
//#define REG_SYSTEM_OP_R_SYSTEM_FAULT_2												0x08D5	// <<Note6 - SYSTEM ERROR LOOKUP>>


// Safety TEST
#define REG_SAFETY_TEST_RW_GRID_REGULATION											0x1000	// <<GRID REGULATION LOOKUP>>			// 2 Bytes		// Unsigned Short


// Custom in Alpha2ESS
#define REG_CUSTOM_LOAD																0xFFFE	// // 1W/bit							// 4 Bytes		// Integer
#define REG_CUSTOM_SYSTEM_DATE_TIME													0xFFFF	// dd/MMM/yyyy HH:mm:ss					// N/A			// Unsigned Char
#define REG_CUSTOM_GRID_CURRENT_A_PHASE												0xFFFD	// 0.1A									// 2 Bytes		// Short
#define REG_CUSTOM_TOTAL_SOLAR_POWER												0xFFFC
// End of Handled Registered







// BATTERY MOS CONTROL LOOKUP
#define BATTERY_MOS_CONTROL_OPEN 0
#define BATTERY_MOS_CONTROL_OPEN_DESC "Open"
#define BATTERY_MOS_CONTROL_CLOSE 1
#define BATTERY_MOS_CONTROL_CLOSE_DESC "Close"

// BATTERY SOC CALIBRATION LOOKUP
#define BATTERY_SOC_CALIBRATION_DISABLE 0
#define BATTERY_SOC_CALIBRATION_DISABLE_DESC "Disable"
#define BATTERY_SOC_CALIBRATION_ENABLE 1
#define BATTERY_SOC_CALIBRATION_ENABLE_DESC "Enable"


// SYSTEM MODE LOOKUP
#define SYSTEM_MODE_AC 1
#define SYSTEM_MODE_AC_DESC "AC mode"
#define SYSTEM_MODE_DC 2
#define SYSTEM_MODE_DC_DESC "DC mode"
#define SYSTEM_MODE_HYBRID 3
#define SYSTEM_MODE_HYBRID_DESC "Hybrid mode"

// METER CT SELECT LOOKUP
#define METER_CT_SELECT_GRID_AND_PV_USE_CT 0
#define METER_CT_SELECT_GRID_AND_PV_USE_CT_DESC "Grid&PV use CT"
#define METER_CT_SELECT_GRID_USE_CT_PV_USE_METER 1
#define METER_CT_SELECT_GRID_USE_CT_PV_USE_METER_DESC "Grid use CT, PV use Meter"
#define METER_CT_SELECT_GRID_USE_METER_PV_USE_CT 2
#define METER_CT_SELECT_GRID_USE_METER_PV_USE_CT_DESC "Grid use meter, PV use CT"
#define METER_CT_SELECT_GRID_AND_PV_USE_METER 3
#define METER_CT_SELECT_GRID_AND_PV_USE_METER_DESC "Grid&PV use meter"


// BATTERY READY LOOKUP
#define BATTERY_READY_OFF 0
#define BATTERY_READY_OFF_DESC "Off"
#define BATTERY_READY_ON 1
#define BATTERY_READY_ON_DESC "On"

// IP METHOD LOOKUP
#define IP_METHOD_DHCP 0
#define IP_METHOD_DHCP_DESC "DHCP"
#define IP_METHOD_STATIC 1
#define IP_METHOD_STATIC_DESC "STATIC"

// MODBUS BAUD RATE LOOKUP
#define MODBUS_BAUD_RATE_9600 0
#define MODBUS_BAUD_RATE_9600_DESC "9600"
#define MODBUS_BAUD_RATE_115200 1
#define MODBUS_BAUD_RATE_115200_DESC "115200 (Only household)"
#define MODBUS_BAUD_RATE_256000 2
#define MODBUS_BAUD_RATE_256000_DESC "256000 (Only household)"
#define MODBUS_BAUD_RATE_19200 3
#define MODBUS_BAUD_RATE_19200_DESC "19200 (Only industry)"

// TIME PERIOD CONTROL FLAG LOOKUP
#define TIME_PERIOD_CONTROL_FLAG_DISABLE 0
#define TIME_PERIOD_CONTROL_FLAG_DISABLE_DESC "Disable Time period control"
#define TIME_PERIOD_CONTROL_FLAG_ENABLE_CHARGE 1
#define TIME_PERIOD_CONTROL_FLAG_ENABLE_CHARGE_DESC "Enable charge Time period control"
#define TIME_PERIOD_CONTROL_FLAG_ENABLE_DISCHARGE 2
#define TIME_PERIOD_CONTROL_FLAG_ENABLE_DISCHARGE_DESC "Enable discharge Time period control"
#define TIME_PERIOD_CONTROL_FLAG_ENABLE 3
#define TIME_PERIOD_CONTROL_FLAG_ENABLE_DESC "Enable Time period control"


// DISPATCH START LOOKUP
#define DISPATCH_START_START 1
#define DISPATCH_START_START_DESC "Start"
#define DISPATCH_START_STOP 0
#define DISPATCH_START_STOP_DESC "Stop"


// Note1 - BATTERY STATUS LOOKUP
#define BATTERY_STATUS_CHARGE0_DISCHARGE0 0
#define BATTERY_STATUS_CHARGE0_DISCHARGE0_DESC "Charge (0) / Discharge (0)"
#define BATTERY_STATUS_CHARGE0_DISCHARGE1 1
#define BATTERY_STATUS_CHARGE0_DISCHARGE1_DESC "Charge (0) / Discharge (1)"
#define BATTERY_STATUS_CHARGE1_DISCHARGE0 256
#define BATTERY_STATUS_CHARGE1_DISCHARGE0_DESC "Charge (1) / Discharge (0)"
#define BATTERY_STATUS_CHARGE1_DISCHARGE1 257
#define BATTERY_STATUS_CHARGE1_DISCHARGE1_DESC "Charge (1) / Discharge (1)"
#define BATTERY_STATUS_CHARGE2_DISCHARGE0 512
#define BATTERY_STATUS_CHARGE2_DISCHARGE0_DESC "Charge (2) / Discharge (0)"
#define BATTERY_STATUS_CHARGE2_DISCHARGE1 513
#define BATTERY_STATUS_CHARGE2_DISCHARGE1_DESC "Charge (2) / Discharge (1)"


// Note2 - BATTERY RELAY STATUS LOOKUP
#define BATTERY_RELAY_STATUS_CHARGE_DISCHARGE_RELAYS_NOT_CONNECTED 0
#define BATTERY_RELAY_STATUS_CHARGE_DISCHARGE_RELAYS_NOT_CONNECTED_DESC "Charge and discharge relays are disconnected"
#define BATTERY_RELAY_STATUS_ONLY_DISCHARGE_RELAY_CLOSED 1
#define BATTERY_RELAY_STATUS_ONLY_DISCHARGE_RELAY_CLOSED_DESC "Only the discharge relay is closed"
#define BATTERY_RELAY_STATUS_ONLY_CHARGE_RELAY_CLOSED 2
#define BATTERY_RELAY_STATUS_ONLY_CHARGE_RELAY_CLOSED_DESC "Only the charging relay is closed"
#define BATTERY_RELAY_STATUS_CHARGE_AND_DISCHARGE_RELAYS_CLOSED 3
#define BATTERY_RELAY_STATUS_CHARGE_AND_DISCHARGE_RELAYS_CLOSED_DESC "Charge and discharge relays are closed"


// Note3 - BATTERY TYPE LOOKUP
#define BATTERY_TYPE_M4860 2
#define BATTERY_TYPE_M4860_DESC "M4860"
#define BATTERY_TYPE_M48100 3
#define BATTERY_TYPE_M48100_DESC "M48100"
#define BATTERY_TYPE_48112_P 13
#define BATTERY_TYPE_48112_P_DESC "48112-P"
#define BATTERY_TYPE_SMILE5_BAT 16
#define BATTERY_TYPE_SMILE5_BAT_DESC "Smile5-BAT"
#define BATTERY_TYPE_M4856_P 24
#define BATTERY_TYPE_M4856_P_DESC "M4856-P"
#define BATTERY_TYPE_SMILE_BAT_10_3P 27
#define BATTERY_TYPE_SMILE_BAT_10_3P_DESC "Smile-BAT-10.3P"
#define BATTERY_TYPE_SMILE_BAT_10_1P 30
#define BATTERY_TYPE_SMILE_BAT_10_1P_DESC "Smile-BAT-10.1P"
#define BATTERY_TYPE_SMILE_BAT_5_8P 33
#define BATTERY_TYPE_SMILE_BAT_5_8P_DESC "Smile-BAT-5.8P"
#define BATTERY_TYPE_SMILE_BAT_5_JP 34
#define BATTERY_TYPE_SMILE_BAT_5_JP_DESC "Smile-BAT-JP"
#define BATTERY_TYPE_SMILE_BAT_13_7P 35
#define BATTERY_TYPE_SMILE_BAT_13_7P_DESC "Smile-BAT-13.7P"

// Note4 - BATTERY ERROR LOOKUP
#define BATTERY_ERROR_BIT_0 ""
#define BATTERY_ERROR_BIT_1 ""
#define BATTERY_ERROR_BIT_2 "Cell Temp Difference"
#define BATTERY_ERROR_BIT_3 "Balancer Fault"
#define BATTERY_ERROR_BIT_4 "Charge Over Current"
#define BATTERY_ERROR_BIT_5 "Balancer Mos Fault"
#define BATTERY_ERROR_BIT_6 "Discharge Over Current"
#define BATTERY_ERROR_BIT_7 "Pole Over Temp"
#define BATTERY_ERROR_BIT_8 "Cell Over Volts"
#define BATTERY_ERROR_BIT_9 "Cell Volt Difference"
#define BATTERY_ERROR_BIT_10 "Discharge Low Temp"
#define BATTERY_ERROR_BIT_11 "Low Volt Shutdown"
#define BATTERY_ERROR_BIT_12 "Cell Low Volts"
#define BATTERY_ERROR_BIT_13 "ISO Comm Fault"
#define BATTERY_ERROR_BIT_14 "LMU SN Repeat"
#define BATTERY_ERROR_BIT_15 "BMU SN Repeat"
#define BATTERY_ERROR_BIT_16 "IR Fault"
#define BATTERY_ERROR_BIT_17 "LMU Comm Fault"
#define BATTERY_ERROR_BIT_18 "Cell Over Temp"
#define BATTERY_ERROR_BIT_19 "BMU Comm Fault"
#define BATTERY_ERROR_BIT_20 "INV Comm Fault"
#define BATTERY_ERROR_BIT_21 "Charge Low Temp"
#define BATTERY_ERROR_BIT_22 "TOPBMU Comm Fault"
#define BATTERY_ERROR_BIT_23 "Volt Detect Fault"
#define BATTERY_ERROR_BIT_24 "Wire Harness Fault"
#define BATTERY_ERROR_BIT_25 "Cluster Cut Fault"
#define BATTERY_ERROR_BIT_26 "Relay Fault"
#define BATTERY_ERROR_BIT_27 "LMU ID Repeat"
#define BATTERY_ERROR_BIT_28 "LMU ID Discontinuous"
#define BATTERY_ERROR_BIT_29 "Current Sensor Fault"
#define BATTERY_ERROR_BIT_30 ""
#define BATTERY_ERROR_BIT_31 "Temp Sensor Fault"


// Note5 - INVERTER OPERATION LOOKUP
#define INVERTER_OPERATION_MODE_WAIT_MODE 0
#define INVERTER_OPERATION_MODE_WAIT_MODE_DESC "Wait Mode"
#define INVERTER_OPERATION_MODE_ONLINE_MODE 1
#define INVERTER_OPERATION_MODE_ONLINE_MODE_DESC "Online Mode"
#define INVERTER_OPERATION_MODE_UPS_MODE 2
#define INVERTER_OPERATION_MODE_UPS_MODE_DESC "UPS Mode"
#define INVERTER_OPERATION_MODE_BYPASS_MODE 3
#define INVERTER_OPERATION_MODE_BYPASS_MODE_DESC "Bypass Mode"
#define INVERTER_OPERATION_MODE_ERROR_MODE 4
#define INVERTER_OPERATION_MODE_ERROR_MODE_DESC "Error Mode"
#define INVERTER_OPERATION_MODE_DC_MODE 5
#define INVERTER_OPERATION_MODE_DC_MODE_DESC "DC Mode"
#define INVERTER_OPERATION_MODE_SELF_TEST_MODE 6
#define INVERTER_OPERATION_MODE_SELF_TEST_MODE_DESC "Self Test Mode"
#define INVERTER_OPERATION_MODE_CHECK_MODE 7
#define INVERTER_OPERATION_MODE_CHECK_MODE_DESC "Check Mode"
#define INVERTER_OPERATION_MODE_UPDATE_MASTER_MODE 8
#define INVERTER_OPERATION_MODE_UPDATE_MASTER_MODE_DESC "Update Master Mode"
#define INVERTER_OPERATION_MODE_UPDATE_SLAVE_MODE 9
#define INVERTER_OPERATION_MODE_UPDATE_SLAVE_MODE_DESC "Update Slave Mode"
#define INVERTER_OPERATION_MODE_UPDATE_ARM_MODE 10
#define INVERTER_OPERATION_MODE_UPDATE_ARM_MODE_DESC "Update ARM Mode"



// Note6 - SYSTEM ERROR LOOKUP
#define SYSTEM_ERROR_AL_BIT_0 "Network Card_Fault"
#define SYSTEM_ERROR_AL_BIT_1 "Rtc_Fault"
#define SYSTEM_ERROR_AL_BIT_2 "E2prom_Fault"
#define SYSTEM_ERROR_AL_BIT_3 "INV_Comms_Error"
#define SYSTEM_ERROR_AL_BIT_4 "Grid_Meter_Lost"
#define SYSTEM_ERROR_AL_BIT_5 "PV_Meter_Lost"
#define SYSTEM_ERROR_AL_BIT_6 "BMS_Lost"
#define SYSTEM_ERROR_AL_BIT_7 "UPS_Battery_Volt_Low"
#define SYSTEM_ERROR_AL_BIT_8 "Backup_Overload"
#define SYSTEM_ERROR_AL_BIT_9 "INV_Slave_Lost"
#define SYSTEM_ERROR_AL_BIT_10 "INV_Master_Lost"
#define SYSTEM_ERROR_AL_BIT_11 "Parallel_Comm_Error"
#define SYSTEM_ERROR_AL_BIT_12 "Parallel_Mode_Differ"
#define SYSTEM_ERROR_AL_BIT_13 "Flash_Fault"
#define SYSTEM_ERROR_AL_BIT_14 "SDRAM error"
#define SYSTEM_ERROR_AL_BIT_15 "Extension CAN error"
#define SYSTEM_ERROR_AL_BIT_16 "inv type not specified"


// Note6 - SYSTEM ERROR LOOKUP
#define SYSTEM_ERROR_AE_BIT_0 "Inverter disconnected"
#define SYSTEM_ERROR_AE_BIT_1 "Net meter separately"
#define SYSTEM_ERROR_AE_BIT_2 "Battery disconnected"
#define SYSTEM_ERROR_AE_BIT_3 "System not set"
#define SYSTEM_ERROR_AE_BIT_4 "PV meter disconnected"
#define SYSTEM_ERROR_AE_BIT_5 "Counter not set"
#define SYSTEM_ERROR_AE_BIT_6 "Incorrect connection direction of the PV meter"
#define SYSTEM_ERROR_AE_BIT_7 "SD not inserted or SD write error"
#define SYSTEM_ERROR_AE_BIT_8 "RTC error"
#define SYSTEM_ERROR_AE_BIT_9 "SDRAM error"
#define SYSTEM_ERROR_AE_BIT_10 "MMC-Error (CH376)"
#define SYSTEM_ERROR_AE_BIT_11 "Network card error"
#define SYSTEM_ERROR_AE_BIT_12 "Extension CAN Error (MCP2515)"
#define SYSTEM_ERROR_AE_BIT_13 "DRED error"
#define SYSTEM_ERROR_AE_BIT_14 "Android LCD separated"
#define SYSTEM_ERROR_AE_BIT_15 "STS_Lost"
#define SYSTEM_ERROR_AE_BIT_16 "STS_Fault"
#define SYSTEM_ERROR_AE_BIT_17 "PV_INV_Lost:n"
#define SYSTEM_ERROR_AE_BIT_18 "DG_PV_Conflict"
#define SYSTEM_ERROR_AE_BIT_19 "PV_INV_Fault:n"
#define SYSTEM_ERROR_AE_BIT_20 "AirConFault"
#define SYSTEM_ERROR_AE_BIT_21 "Fire_Fault"
#define SYSTEM_ERROR_AE_BIT_22 "FireControllerErr"
#define SYSTEM_ERROR_AE_BIT_23 "GC_Fault"
#define SYSTEM_ERROR_AE_BIT_24 "AirConLost"
#define SYSTEM_ERROR_AE_BIT_25 "OverCurr"
#define SYSTEM_ERROR_AE_BIT_26 "PcsModeFault"
#define SYSTEM_ERROR_AE_BIT_27 "BatEnergyLow"



// Note7 - DISPATCH MODE LOOKUP
#define DISPATCH_MODE_BATTERY_ONLY_CHARGED_VIA_PV 1
#define DISPATCH_MODE_BATTERY_ONLY_CHARGED_VIA_PV_DESC "The battery is only charged via PV"
#define DISPATCH_MODE_STATE_OF_CHARGE_CONTROL 2
#define DISPATCH_MODE_STATE_OF_CHARGE_CONTROL_DESC "State of charge control"
#define DISPATCH_MODE_LOAD_FOLLOWING 3
#define DISPATCH_MODE_LOAD_FOLLOWING_DESC "Load Following"
#define DISPATCH_MODE_MAXIMISE_OUTPUT 4
#define DISPATCH_MODE_MAXIMISE_OUTPUT_DESC "Maximize Output"
#define DISPATCH_MODE_NORMAL_MODE 5
#define DISPATCH_MODE_NORMAL_MODE_DESC "Normal mode"
#define DISPATCH_MODE_OPTIMISE_CONSUMPTION 6
#define DISPATCH_MODE_OPTIMISE_CONSUMPTION_DESC "Optimise consumption"
#define DISPATCH_MODE_MAXIMISE_CONSUMPTION 7
#define DISPATCH_MODE_MAXIMISE_CONSUMPTION_DESC "Maximise consumption"
#define DISPATCH_MODE_ECO_MODE 8
#define DISPATCH_MODE_ECO_MODE_DESC "ECO Mode"
#define DISPATCH_MODE_FCAS_MODE 9
#define DISPATCH_MODE_FCAS_MODE_DESC "FCAS-Mode"
#define DISPATCH_MODE_PV_POWER_SETTING 10
#define DISPATCH_MODE_PV_POWER_SETTING_DESC "PV power setting"




// Note8 - GRID REGULATION LOOKUP
#define GRID_REGULATION_AL_0 0
#define GRID_REGULATION_AL_0_DESC "VDE0126"
#define GRID_REGULATION_AL_1 1
#define GRID_REGULATION_AL_1_DESC "ARN4105/11.18"
#define GRID_REGULATION_AL_2 2
#define GRID_REGULATION_AL_2_DESC "AS4777.2"
#define GRID_REGULATION_AL_3 3
#define GRID_REGULATION_AL_3_DESC "G83_2"
#define GRID_REGULATION_AL_4 4
#define GRID_REGULATION_AL_4_DESC "C10/C11"
#define GRID_REGULATION_AL_5 5
#define GRID_REGULATION_AL_5_DESC "TOR D4"
#define GRID_REGULATION_AL_6 6
#define GRID_REGULATION_AL_6_DESC "EN50438_NL"
#define GRID_REGULATION_AL_7 7
#define GRID_REGULATION_AL_7_DESC "EN50438_DK"
#define GRID_REGULATION_AL_8 8
#define GRID_REGULATION_AL_8_DESC "CEB"
#define GRID_REGULATION_AL_9 9
#define GRID_REGULATION_AL_9_DESC "CEI-021"
#define GRID_REGULATION_AL_10 10
#define GRID_REGULATION_AL_10_DESC "NRS097-2-1"
#define GRID_REGULATION_AL_11 11
#define GRID_REGULATION_AL_11_DESC "VDE0126_GREECE"
#define GRID_REGULATION_AL_12 12
#define GRID_REGULATION_AL_12_DESC "UTE_C15_712"
#define GRID_REGULATION_AL_13 13
#define GRID_REGULATION_AL_13_DESC "IEC61727"
#define GRID_REGULATION_AL_14 14
#define GRID_REGULATION_AL_14_DESC "G59_3"
#define GRID_REGULATION_AL_15 15
#define GRID_REGULATION_AL_15_DESC "RD1699"
#define GRID_REGULATION_AL_16 16
#define GRID_REGULATION_AL_16_DESC "G99"
#define GRID_REGULATION_AL_17 17
#define GRID_REGULATION_AL_17_DESC "Philippines_60HZ"
#define GRID_REGULATION_AL_18 18
#define GRID_REGULATION_AL_18_DESC "Tahiti_60HZ"
#define GRID_REGULATION_AL_19 19
#define GRID_REGULATION_AL_19_DESC "AS4777.2-SA"
#define GRID_REGULATION_AL_20 20
#define GRID_REGULATION_AL_20_DESC "G98"
#define GRID_REGULATION_AL_21 21
#define GRID_REGULATION_AL_21_DESC "EN50549"
#define GRID_REGULATION_AL_22 22
#define GRID_REGULATION_AL_22_DESC "PEA"
#define GRID_REGULATION_AL_23 23
#define GRID_REGULATION_AL_23_DESC "MEA"
#define GRID_REGULATION_AL_24 24
#define GRID_REGULATION_AL_24_DESC "BISI"
#define GRID_REGULATION_AL_25 25
#define GRID_REGULATION_AL_25_DESC "JET-GR Series"
#define GRID_REGULATION_AL_26 26
#define GRID_REGULATION_AL_26_DESC "JET-GR Series"
#define GRID_REGULATION_AL_27 27
#define GRID_REGULATION_AL_27_DESC "Taiwan"
#define GRID_REGULATION_AL_28 28
#define GRID_REGULATION_AL_28_DESC "DEFAULT_50HZ"
#define GRID_REGULATION_AL_29 29
#define GRID_REGULATION_AL_29_DESC "DEFAULT_60HZ"
#define GRID_REGULATION_AL_30 30
#define GRID_REGULATION_AL_30_DESC "WAREHOUSE"
#define GRID_REGULATION_AL_31 31
#define GRID_REGULATION_AL_31_DESC "AS4777.2-NZ"
#define GRID_REGULATION_AL_32 32
#define GRID_REGULATION_AL_32_DESC "Korea"
#define GRID_REGULATION_AL_33 33
#define GRID_REGULATION_AL_33_DESC "G98/G99-IE"
#define GRID_REGULATION_AL_34 34
#define GRID_REGULATION_AL_34_DESC "34EN50549-PL"
#define GRID_REGULATION_AL_35 35
#define GRID_REGULATION_AL_35_DESC "UL 1741"
#define GRID_REGULATION_AL_36 36
#define GRID_REGULATION_AL_36_DESC "UL1741-Rule 21"
#define GRID_REGULATION_AL_37 37
#define GRID_REGULATION_AL_37_DESC "UL1741-Hawaiian"



// MQTT Subscriptions
enum mqttSubscriptions
{
	readHandledRegister,
	readHandledRegisterAll,
	readRawRegister,
	writeRawSingleRegister,
	writeRawDataRegister,
	setCharge,
	setDischarge,
	setNormal,
	unknown
};
#define MQTT_SUB_REQUEST_READ_HANDLED_REGISTER "/request/read/register/handled"
#define MQTT_SUB_REQUEST_READ_RAW_REGISTER "/request/read/register/raw"
#define MQTT_SUB_REQUEST_WRITE_RAW_SINGLE_REGISTER "/request/write/register/raw/single"
#define MQTT_SUB_REQUEST_WRITE_RAW_DATA_REGISTER "/request/write/register/raw/data"
#define MQTT_SUB_REQUEST_SET_CHARGE "/request/set/charge"
#define MQTT_SUB_REQUEST_SET_DISCHARGE "/request/set/discharge"
#define MQTT_SUB_REQUEST_SET_NORMAL "/request/set/normal"
#define MQTT_SUB_REQUEST_READ_HANDLED_REGISTER_ALL "/request/read/register/handled/all"

// MQTT Responses
#define MQTT_MES_RESPONSE_READ_HANDLED_REGISTER "/response/read/register/handled"
#define MQTT_MES_RESPONSE_READ_RAW_REGISTER "/response/read/register/raw"
#define MQTT_MES_RESPONSE_WRITE_RAW_SINGLE_REGISTER "/response/write/register/raw/single"
#define MQTT_MES_RESPONSE_WRITE_RAW_DATA_REGISTER "/response/write/register/raw/data"
#define MQTT_SUB_RESPONSE_SET_CHARGE "/response/set/charge"
#define MQTT_SUB_RESPONSE_SET_DISCHARGE "/response/set/discharge"
#define MQTT_SUB_RESPONSE_SET_NORMAL "/response/set/normal"
#define MQTT_SUB_RESPONSE_READ_HANDLED_REGISTER_ALL "/response/read/register/handled/all"


#define MQTT_MES_STATE_SECOND_TEN "/state/second/ten"
#define MQTT_MES_STATE_MINUTE_ONE "/state/minute/one"
#define MQTT_MES_STATE_MINUTE_FIVE "/state/minute/five"
#define MQTT_MES_STATE_HOUR_ONE "/state/hour/one"
#define MQTT_MES_STATE_DAY_ONE "/state/day/one"







// Frame and Function Codes
#define MAX_FRAME_SIZE_ZERO_INDEXED 63
#define MIN_FRAME_SIZE_ZERO_INDEXED 4
#define MAX_FRAME_SIZE_RESPONSE_WRITE_SUCCESS_ZERO_INDEXED 7

#define MODBUS_FN_READDATAREGISTER 0x03
#define MODBUS_FN_WRITEDATAREGISTER 0x10
#define MODBUS_FN_WRITESINGLEREGISTER 0x06

#define FRAME_POSITION_SLAVE_ID 0
#define FRAME_POSITION_FUNCTION_CODE 1



// Ensure we stick to fixed values by forcing from a selection of values for data type returned
enum modbusReturnDataType
{
	notDefined,
	unsignedInt,
	signedInt,
	unsignedShort,
	signedShort,
	character
};
#define MODBUS_RETURN_DATA_TYPE_NOT_DEFINED_DESC "notDefined"
#define MODBUS_RETURN_DATA_TYPE_UNSIGNED_INT_DESC "unsignedInt"
#define MODBUS_RETURN_DATA_TYPE_SIGNED_INT_DESC "signedInt"
#define MODBUS_RETURN_DATA_TYPE_UNSIGNED_SHORT_DESC "unsignedShort"
#define MODBUS_RETURN_DATA_TYPE_SIGNED_SHORT_DESC "signedShort"
#define MODBUS_RETURN_DATA_TYPE_CHARACTER_DESC "character"




// Ensure we stick to fixed values by forcing from a selection of values for a Modbus request & response
enum modbusRequestAndResponseStatusValues
{
	preProcessing,
	notHandledRegister,
	invalidFrame,
	responseTooShort,
	noResponse,
	noMQTTPayload,
	invalidMQTTPayload,
	writeSingleRegisterSuccess,
	writeDataRegisterSuccess,
	readDataRegisterSuccess,
	slaveError,
	setDischargeSuccess,
	setChargeSuccess,
	setNormalSuccess,
	payloadExceededCapacity,
	addedToPayload,
	notValidIncomingTopic
};
#define MODBUS_REQUEST_AND_RESPONSE_PREPROCESSING_MQTT_DESC "preProcessing"
#define MODBUS_REQUEST_AND_RESPONSE_NOT_HANDLED_REGISTER_MQTT_DESC "notHandledRegister"
#define MODBUS_REQUEST_AND_RESPONSE_INVALID_FRAME_MQTT_DESC "invalidFrame"
#define MODBUS_REQUEST_AND_RESPONSE_RESPONSE_TOO_SHORT_MQTT_DESC "responseTooShort"
#define MODBUS_REQUEST_AND_RESPONSE_NO_RESPONSE_MQTT_DESC "noResponse"
#define MODBUS_REQUEST_AND_RESPONSE_NO_MQTT_PAYLOAD_MQTT_DESC "noMQTTPayload"
#define MODBUS_REQUEST_AND_RESPONSE_INVALID_MQTT_PAYLOAD_MQTT_DESC "invalidMQTTPayload"
#define MODBUS_REQUEST_AND_RESPONSE_WRITE_SINGLE_REGISTER_SUCCESS_MQTT_DESC "writeSingleRegisterSuccess"
#define MODBUS_REQUEST_AND_RESPONSE_WRITE_DATA_REGISTER_SUCCESS_MQTT_DESC "writeDataRegisterSuccess"
#define MODBUS_REQUEST_AND_RESPONSE_READ_DATA_REGISTER_SUCCESS_MQTT_DESC "readDataRegisterSuccess"
#define MODBUS_REQUEST_AND_RESPONSE_ERROR_MQTT_DESC "slaveError"
#define MODBUS_REQUEST_AND_RESPONSE_SET_DISCHARGE_SUCCESS_MQTT_DESC "setDishargeSuccess"
#define MODBUS_REQUEST_AND_RESPONSE_SET_CHARGE_SUCCESS_MQTT_DESC "setChargeSuccess"
#define MODBUS_REQUEST_AND_RESPONSE_SET_NORMAL_SUCCESS_MQTT_DESC "setNormalSuccess"
#define MODBUS_REQUEST_AND_RESPONSE_PAYLOAD_EXCEEDED_CAPACITY_MQTT_DESC "payloadExceededCapacity"
#define MODBUS_REQUEST_AND_RESPONSE_ADDED_TO_PAYLOAD_MQTT_DESC "addedToPayload"
#define MODBUS_REQUEST_AND_RESPONSE_NOT_VALID_INCOMING_TOPIC_MQTT_DESC "notValidIncomingTopic"


#define MODBUS_REQUEST_AND_RESPONSE_PREPROCESSING_DISPLAY_DESC "PRE-PROC"
#define MODBUS_REQUEST_AND_RESPONSE_NOT_HANDLED_REGISTER_DISPLAY_DESC "NOT-HANDL"
#define MODBUS_REQUEST_AND_RESPONSE_INVALID_FRAME_DISPLAY_DESC "RSP-BADCRC"
#define MODBUS_REQUEST_AND_RESPONSE_RESPONSE_TOO_SHORT_DISPLAY_DESC "RSP-SHORT"
#define MODBUS_REQUEST_AND_RESPONSE_NO_RESPONSE_DISPLAY_DESC "NO-RSP"
#define MODBUS_REQUEST_AND_RESPONSE_NO_DISPLAY_PAYLOAD_DISPLAY_DESC "NO-PAYLOAD"
#define MODBUS_REQUEST_AND_RESPONSE_INVALID_DISPLAY_PAYLOAD_DISPLAY_DESC "INV-PAYLOA"
#define MODBUS_REQUEST_AND_RESPONSE_WRITE_SINGLE_REGISTER_SUCCESS_DISPLAY_DESC "W-SR-SUC"
#define MODBUS_REQUEST_AND_RESPONSE_WRITE_DATA_REGISTER_SUCCESS_DISPLAY_DESC "W-DR-SUC"
#define MODBUS_REQUEST_AND_RESPONSE_READ_DATA_REGISTER_SUCCESS_DISPLAY_DESC "R-DR-SUC"
#define MODBUS_REQUEST_AND_RESPONSE_ERROR_DISPLAY_DESC "SLA-ERROR"
#define MODBUS_REQUEST_AND_RESPONSE_SET_DISCHARGE_SUCCESS_DISPLAY_DESC "SET-DI-SUC"
#define MODBUS_REQUEST_AND_RESPONSE_SET_CHARGE_SUCCESS_DISPLAY_DESC "SET-CH-SUC"
#define MODBUS_REQUEST_AND_RESPONSE_SET_NORMAL_SUCCESS_DISPLAY_DESC "SET-NO-SUC"
#define MODBUS_REQUEST_AND_RESPONSE_PAYLOAD_EXCEEDED_CAPACITY_DISPLAY_DESC "PAYLOAD-ER"
#define MODBUS_REQUEST_AND_RESPONSE_ADDED_TO_PAYLOAD_DISPLAY_DESC "ADDED-PAYL"
#define MODBUS_REQUEST_AND_RESPONSE_NOT_VALID_INCOMING_TOPIC_DISPLAY_DESC "notValidIncomingTopic"

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

	char statusMqttMessage[MAX_MQTT_STATUS_LENGTH] = MODBUS_REQUEST_AND_RESPONSE_PREPROCESSING_MQTT_DESC;
	char displayMessage[OLED_CHARACTER_WIDTH] = MODBUS_REQUEST_AND_RESPONSE_PREPROCESSING_DISPLAY_DESC;

	// These variables will be set by the sending process
	char mqttName[MAX_MQTT_NAME_LENGTH] = "";
	uint8_t registerCount = 0;
	modbusReturnDataType returnDataType = modbusReturnDataType::notDefined;
	char returnDataTypeDesc[MAX_DATA_TYPE_DESC_LENGTH] = MODBUS_RETURN_DATA_TYPE_NOT_DEFINED_DESC;
	bool hasLookup = false;

	// And one of these will be set by the receiving process
	uint32_t unsignedIntValue = 0;
	int32_t signedIntValue = 0;
	uint16_t unsignedShortValue = 0;
	int16_t signedShortValue = 0;
	char characterValue[MAX_CHARACTER_VALUE_LENGTH] = "";
	char dataValueFormatted[MAX_FORMATTED_DATA_VALUE_LENGTH] = "";
};


struct mqttState
{
	uint16_t registerAddress;
	char mqttName[MAX_MQTT_NAME_LENGTH];
};




#define DEBUG
//#define DEBUG_LEVEL2 // For serial flooding action
//#define DEBUG_OUTPUT_TX_RX
#endif
