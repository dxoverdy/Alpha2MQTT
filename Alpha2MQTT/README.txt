Alpha2MQTT (A2M) is a remote control interface for AlphaESS inverters which support Modbus via RS485.

It allows full remote control of the inverter and can report various information based on repeating schedules of every 10 seconds, every 1 minute, every 5 minutes, every hour and every day.

Communication with Alpha2MQTT is via MQTT and can be driven by home integration solutions such as Home Assistant, Node-Red and anything else which is MQTT compatible.

It's designed to run on an ESP8266 microcontroller with a TTL to RS485 module such as the MAX3485.

Alpha2MQTT honours AlphaESS Modbus documentation:
https://www.alpha-ess.de/images/downloads/handbuecher/AlphaESS-Handbuch_SMILE_ModBus_RTU_TCP_V2.pdf
https://www.alpha-ess.de/images/downloads/handbuecher/AlphaESS_Register_Parameter_List.pdf
It is only available in German however Google offers PDF translation which I have used to good effect for English.

Supported devices are:
SMILE5, SMILE-B3, SMILE-T10, Storion T30.
The default baud rates 9600, 9600, 9600 and 19200 respectively.
The default slave address is 0x55.

Separately, users have confirmed the following other devices work
SMILE-B3-PLUS using a default baud rate of 9600.


Useful Terms:
=============
MQTT
====
Message Queueing Telemetry Transport
https://en.wikipedia.org/wiki/MQTT
Do some research on MQTT if this is new to you.  Mosquitto is an easy, lightweight broker which can be installed on a raft of devices and computers.

Register
========
A register is an address on the inverter which holds a piece of information.  For example, address 0x0102 (or 0102H as written in the documentation) is the address which holds the battery state of charge (in percent.)  A register can be read only, write only or both read and write.  Registers are two bytes long (16 bit) or are four bytes long (32 bit) and these are distinguished in the Data format column of the AlphaESS Modbus documentation.  A two byte register is referred to as a single register as it has a single memory address, a four byte register is referred to as a double register as it has two memory addresses.



Configuration:
==============
Configure Alpha2MQTT by opening up Definitions.h and verifying/customising the following definitions for your need:
- Set your inverter (delete the //) between lines 25-28 and ensure the rest are commented out by adding //.
- Set your WiFi Access Point name on line 32.
- Set your WiFi password on line 33.
- Set your MQTT broker server on line 36.
- Set your MQTT broker port on line 37.  (Default is 1883)
- Set your MQTT username on line 38 if you use security (you should) or leave it blank.
- Set your MQTT password on line 39 if you use security (again, you should) or leave it blank.
- Set your Alpha2MQTT device name on line 43.  This is the device name presented on your network and is also how MQTT topics begin.  This document assumes Alpha2MQTT.
- Set your AlphaESS inverter's slave id on line 46.  By default this is 0x55 and shouldn't need changing unless you've changed it via Modbus or via inverters which have an integrated display.  Don't change it.
- Set your maximum payload size on line 54.  ESP8266's work well with 4096 bytes which is enough for 70 to 80 registers on any schedule or request.
- Set whether the device should auto restart every so many hours.  This is for specific routers only.  Uncomment line 70 if you want to use this feature
- Set the number of hours for an automatic restart on line 71

States:
=======
To receive information from any of the repeating schedules, subscribe your MQTT client to:

Alpha2MQTT/state/second/ten
Alpha2MQTT/state/minute/one
Alpha2MQTT/state/minute/five
Alpha2MQTT/state/hour/one
Alpha2MQTT/state/day/one

Each of the schedules can be customised to report a number* / any combination of register values.  Alpha2MQTT has been developed this way to give flexibility.  For example, there is probably no need to report grid voltage every ten seconds, instead once every five minutes and in doing so we are limiting network traffic over MQTT and onward processing in software such as Home Assistant.

*See maximum payload size above.

You can customise the schedules by modifying Alpha2MQTT.ino.  Search for 'Schedules' and add or remove registers as you see fit from each schedule.  The list of supported registers begins on line 85 in Definitions.h.  A register name which contains _R_ is read only, one which contains _RW_ is read/write, and one which contains _W_ is write only.

An example response for any subscribed state is a JSON of name/value pairs which are separated by commas, for example:
{
    "REG_BATTERY_HOME_R_BATTERY_POWER": 2845,
    "REG_INVERTER_HOME_R_VOLTAGE_L1": 238.4
}


Advanced Read Registers
=======================
Appreciating that some people may want to take inverter values in raw form with extra information, Alpha2MQTT supports request and responses for individual registers.  It does this by offering two ways, handled and raw.  A handled register and raw register is essentially the same request to the inverter, however when requesting via the handled route, checks, calculations and balances are done in Alpha2MQTT and the response includes both raw and formatted (as per Modbus documentation) data and information.  For example, where the Modbus documentation indicated a number should undergo manipulation to return something of value, i.e. frequency which needs to be multiplied by 0.01 to return Hz, then a handled read request will return the raw data, as well as the formatted data which underwent calculations.  A handled request for the EMS serial number (ALxxxxxxxxxxxxxxx) will return just that, rather than a series of numbers which need manipulation by you.


Handled Read:
=============
Publish MQTT messages to:
Alpha2MQTT/request/read/register/handled
With the following JSON
{
    "registerAddress": "0x0010"
}
where
registerAddress is the hex address of the register as per the documentation, and where the register is a handled register in Definitions.h

Alpha2MQTT will do the rest and will return the response via the following topic which you can subscribe to:
Alpha2MQTT/response/read/register/handled

An example response for the above request could be:
{
    "responseStatus": "readDataRegisterSuccess",
    "registerAddress": "0x0010",
    "functionCode": 3,
    "registerName": "REG_GRID_METER_R_TOTAL_ENERGY_FEED_TO_GRID_1",
    "dataType": "unsignedInt",
    "dataValue": 123515,
    "formattedDataValue": 12351.50,
    "rawDataSize": 4,
    "rawData": [0,1,226,123],
    "end": "true"
}

* If the register returns a character dataType then dataValue and formattedDataValue will be in double quotes.
** If the register is a lookup, i.e. 0x1000 (REG_SAFETY_TEST_RW_GRID_REGULATION) Grid_Regulation, formattedDataValue will bring back the appropriate textual lookup, i.e dataValue of 8, formattedDataValue of "CEB".
*** 0x0743 (REG_SYSTEM_INFO_R_EMS_SN_BYTE_1_2) EMS SN byte1-2 is the only register which undergoes custom processing in Alpha2MQTT different to spec.  It returns the full 15 character ALxxxxxxxxxxxxxxx serial number in characterValue in one go, and the remaining EMS SN byte-x-y registers are not implemented as they are essentially pointless.
**** There is a custom handled register address of 0xFFFF (REG_CUSTOM_SYSTEM_DATE_TIME) which returns the full system date/time in UK dd/MMM/yyyy HH:mm:ss format in formattedDataValue.
***** There is a custom handled register address of 0xFFFE (REG_CUSTOM_LOAD) which returns current consumption, also known as load.  It returns in watts.




Raw Read:
=========
Alpha2MQTT supports over 200 registers via the handled route, however it does not cater for registers in the Safety TEST, ATE TEST, CT calibration and Battery - INDUSTRY series categories (with the exception of 0x1000 Grid_Regulation.)  This is because there are many more hundreds of registers in these categories, they are rather niche and on my inverter (SMILE B3) most I cannot query and test.  As such, by providing a raw read functionality Alpha2MQTT can expose any of these registers to advanced users and it will return the raw data bytes for onward processing as you see fit.

Publish MQTT messages to:
Alpha2MQTT/request/read/register/raw
With the following JSON
{
    "registerAddress": "0x0743",
    "dataBytes": 2
}
where
registerAddress is the hex address of the register as per the documentation.
dataBytes is the number of bytes to request, as per the Data format column in the Alpha documentation.  Usually follow the documentation, however for registers such as serial numbers / date times you can actually pass more and the Alpha system will duly return more.  For example, the documentation for EMS SN byte1-2 (0x0743) is two bytes
however you can request 16.
{
    "registerAddress": "0x0743",
    "dataBytes": 16
}

Alpha2MQTT will do the rest and will return the response via the following topic which you can subscribe to:
Alpha2MQTT/response/read/register/raw

An example response for the above request could be:
{
    "responseStatus": "readDataRegisterSuccess",
    "registerAddress": "0x0743",
    "functionCode": 3,
    "rawDataSize": 16,
    "rawData": [65,76,55,48,48,49,48,50,49,48,54,48,51,50,49,0],
    "end": "true"
}





Writing:
=======
The AlphaESS documentation mentions two methods to write to registers, "Write Single Register" and "Write Data Register."  Alpha2MQTT supports both.  That said, when I started developing Alpha2MQTT I presumed Write Single Register was an easier way to write a two-byte register, requiring only a value, however my inverter never responds to any Write Single Register request.  The documentation on how to make a request and obtain the response is below, however it is only provided as guidance.  Write Data Register works with any two or four byte register and so for me, for my Smile B3 at least, is the go-to function.



Write Raw Single Register:
==========================
Publish MQTT messages to:
Alpha2MQTT/request/write/register/raw/single
With the following JSON
{
    "registerAddress": "0x084F",
    "value": 1
}
where
registerAddress is the hex address of the register as per the documentation.
value is the base 10 integer you wish to write to the register.  For example, the above will write 1 to the Time period control flag register and Enable charge time period control and disable discharge time period control.

Alpha2MQTT will do the rest and will return the response via the following topic which you can subscribe to:
Alpha2MQTT/response/write/register/raw/single

An example response for the above request could be:
{
    "responseStatus": "writeSingleRegisterSuccess",
    "registerAddress": "0x084F",
    "functionCode": 6,
    "rawDataSize": 4,
    "rawData": [8,79,0,1]
}

In reality, you will probably only ever get the following back:
{
    "responseStatus": "noResponse",
    "registerAddress": "0x084F"
}
In raw data, the Alpha documentation says byte one is the high byte of the register address, byte two is the low byte of the register address, byte three is the high byte of the value and byte four is the low byte of the value.
Again, for me, this function does nothing and I only provide the above as guidance as to how it should work.  Instead, I recommend just using Write Raw Data Register as documented below.



Write Raw Data Register:
========================
Publish MQTT messages to:
Alpha2MQTT/request/write/register/raw/data
With the following JSON
{
    "registerAddress": "0x0881",
    "dataBytes": 4,
    "value": 30000
}
where
registerAddress is the hex address of the register as per the documentation.
dataBytes is the number of bytes to write as appropriate for the register in question as per the Data Format column.
value is the base 10 integer you wish to write to the register.  For example, the above will write 30000 to the Dispatch active power register to essentially set battery charging to 2000W.

* If the register has documentation which suggests values are in hex, keep in mind they need to be written in a single base 10 value (standard numbers you count with) regardless of whether you are writing to a single two-byte register or a double four-byte register.  And all hex values are read out in base 10 bytes, each between 0 and 255.  What does this mean exactly?  Take register 0x080F, the Modbus address.  The documentation refers to the value being 0x55 in hex.  This is true, however this will be read out in rawData as 0, 85.  If you wanted to change the Modbus address to 0x56, you would use a value of 86.  This would be read out in rawData as 0, 86.
** There is a hex starter for ten at the bottom of this document.


Alpha2MQTT will do the rest and will return the response via the following topic which you can subscribe to:
Alpha2MQTT/response/write/register/raw/data

An example response for the above request could be:
{
    "responseStatus": "writeDataRegisterSuccess",
    "registerAddress": "0x0881",
    "functionCode": 16,
    "rawDataSize": 4,
    "rawData": [8,129,0,2],
    "end": "true"
}
In raw data, the Alpha documentation says byte one is the high byte of the register address, byte two is the low byte of the register address, byte three is the high byte of the number of registers and byte four is the low byte of the number of registers.  As this was a four byte register, in reality this is a double register, hence 2 in the last byte.


You can technically be smart and write two registers at once if they are next to each other, but things are complex enough as they are so I advise you don't.
If you take registerAddress of 0x0851 (Discharge Start Time 1,) it is next to registerAddress 0x0852 (Discharge Stop Time 1) and they are two bytes each.  So technically you can get away with writing a single value in to four dataBytes.
If done individually:
{
    "registerAddress": "0x0851",
    "value":1,
    "dataBytes":2
}
{
    "registerAddress": "0x0852",
    "value":4,
    "dataBytes":2
}
These two instructions will set start time 1 to 01:00 and stop time 1 to 04:00 respectively.

But combined:
{
    "registerAddress": "0x0851",
    "value":65540,
    "dataBytes":4
}

Why 65540?

                1AM             4AM
00000000 00000001 00000000 00000100 = 65540 in base 10 decimal


** WARNING **
Double and triple check the dataBytes you send corresponds to the register in the Alpha Documentation.  Writing more or less may have unexpected outcomes!
Double and triple check the value you send.  Writing an invalid or incompatible value may have unexpected outcomes!
I have written to most of the R/W registers where I have had an interest in doing so, and it appears AlphaESS have developed this to be pretty robust where it just regards junk requests, however please, employ discretion at all times!




Slave Errors:
=============
If the request and response was successful, but the inverter failed to carry out the task for some reason, it will return a slave error.  The response to any request above will be of the following format if so:
{
    "responseStatus": "slaveError",
    "functionCode": 16,
    "slaveErrorCode": 321
    "end": "true"
}
where
slaveErrorCode is an unknown number.  Note that slave errors are not covered in the AlphaESS documentation, and not having received a slave error response I cannot give guidance as to what would cause errors, or what potential slaveErrorCodes could be.



JSON Definitions:
==========================
responseStatus - Denotes the outcome of the request and can be one of:
- preProcessing - Alpha2MQTT wasn't able to progress beyond pre-processing stage.
- notHandledRegister - The register address in the request wasn't one within Alpha2MQTT's list of handled registers.
- invalidFrame - The data which was returned by the inverter failed CRC checks and so was discarded.  Frequent occurrence of this could indicate wiring/circuitry issues.
- responseTooShort - The data which was returned by the inverter was too short to be considered and so was discarded.  Frequent occurence of this could indicate wiring/circuitry issues.
- noResponse - The inverter didn't respond at all to the request.
- noMQTTPayload - No JSON was passed to the request.
- invalidMQTTPayload - JSON was passed to the request, but was of an invalid format.
- writeSingleRegisterSuccess - The Write Single Register process was successful.
- writeDataRegisterSuccess - The Write Data Register process was successful.
- readDataRegisterSuccess - The Read Data Register process was successful.
- slaveError - The inverter responded with an error code to the request.
- setChargeSuccess - The inverter is now charging from the grid.
- setDischargeSuccess - The inverter is now discharging to the grid.


dataType - Denotes the kind of data which is returned - and hence how it should be treated if onward processed and can be one of:
- notDefined - Alpha2MQTT doesn't know at this stage.
- unsignedInt - A four byte (32 bit) number which has a possible range between 0 and 65535
- unsignedShort - A two byte (16 bit) number which has a possible range between 0 and 4294967295
- signedInt - A two byte (16 bit) number which has a possible range between -2147483648 to 2147483647
- signedShort - A two byte (16 bit) number which has a possible range between 32768 to +32767
- character - A variable length character string which could contain text such as serial numbers and dates.

registerAddress - The address of the requested register, in 0x hex format (i.e. 0x0126) but surrounded in double quotes.  This comes back in responses so you can be confident a particular response is for a particular request.

functionCode - The function code of the operation, defined by AlphaESS as:
- Read Data Register 0x03 (3)
- Write Single Register 0x6 (6)
- Write Data Register 0x10 (16)
- Slave Error Read Data Register 0x80 + 0x03 = 0x83 ((128 + 3) = 131)
- Slave Error Write Single Register 0x80 + 0x06 = 0x86 ((128 + 6) = 134)
- Slave Error Write Data Register 0x80 + 0x10 = 0x90 ((128 + 16) = 144)


registerName - The name of the register internally defined in Alpha2MQTT as per the handled register list in Definitions.h.  For example, REG_BATTERY_HOME_R_BATTERY_POWER is Alpha2MQTT's name for register 0x0126.

dataValue - The raw value from the register in decimal form.  Quoted if textual, unquoted if numerical.

formattedDataValue - The value from dataValue having undergone calculations as per AlphaESS Modbus documentation.  Quoted if textual, unquoted if numerical.

slaveErrorCode - The actual error code returned by the inverter.  There is no supporting documentation for this and I have never had a slave error response.

rawDataSize - The number of bytes of data returned by the inverter.

rawData - An array of utf8_t bytes (base 10) of the actual data response from the inverter.  If reading a 2 byte register there will be two bytes, high byte and low byte.  If reading a 4 byte register there will be four bytes, high byte and low byte, high byte and low byte.

rawData example
{
    "responseStatus": "readDataRegisterSuccess",
    "registerAddress": "0x0881",
    "functionCode": 3,
    "registerName": "REG_DISPATCH_RW_ACTIVE_POWER_1",
    "dataType": "signedInt",
    "dataValue": 32000,
    "formattedDataValue": 32000,
    "rawDataSize": 4,
    "rawData": [0,0,125,0],
    "end": "true"
}
0        0        125      0
00000000 00000000 01111101 00000000 = 32000 base 10 decimal.






Force Charge From Grid
======================
As we now have nifty functionality to command the inverter as we see fit, Alpha2MQTT has some MQTT topics to send messages to to instruct it to charge from the grid.  All this is doing is essentially batching up some appropriate Write Data Register commands and exposing them as a single MQTT topic for ease of use.

Publish MQTT messages to:
Alpha2MQTT/request/set/charge
With the following JSON
{
    "watts": 2500,
    "socPercent": 100,
    "duration": 14400
}
where
watts is the power at which you want to charge the batteries with.  You can specify a number greater than the inverter/battery supports, the system will limit accordingly.  They provide this functionality in case you don't want to hammer your batteries at full power.  Note that it will charge at this rate utilising PV power too.  As such, if you set watts to 2500 and PV is generating 1000, it will pull 1500 from the grid.  As I suspect this will be largely used during the night when electricity rates are cheaper (such as Octopus Go / Economy 7) then this is largely irrelevant.

socPercent is where you want charging to stop.  If you want to stop charging at 80% as you believe the weather tomorrow will do the rest, set this to 80.  Otherwise, usually it'll be 100.

duration is how long in seconds to put the inverter in this mode.  Charging from the grid uses 'Dispatch' mode where an external device ala Alpha2MQTT can, for a specified time period, instruct what it does, however I believe for safety Alpha require you to specify how long it'll be in dispatch mode so it can always end up back in the inverter's control.  As such, just put a duration in seconds for how long usually it takes to charge the battery.  If you set to 14400 (4 hours) that'll cover off most domestic systems.  Ultimately it's a calculation based on kWh of battery capacity divided by the rate you want to charge plus some tolerance, say 5%.  For my 11.47kWh B3 which can charge at circa 3000W I am going to be utilising 3000 as the watts and duration as (11000 / 2800 (allowing for usable capacity / actual max charge)) + 5% for 3.92 hours + 5% = 4.125 hours = 15000 seconds.  After the 15000 seconds control will be released and the battery will be able to discharge to the load of the house.  You can alternatively write 0 to register 0x0880 to turn off dispatch mode manually using Write Data Register when you have read an SOC of 100 via register 0x0102 using the Read Handled Register functionality of Alpha2MQTT via some periodically driven automated process in Node Red or Home Assistant.


Alpha2MQTT will do the rest and will return the response via the following topic which you can subscribe to:
Alpha2MQTT/response/set/charge

An example response for the above request could be:
{
    "responseStatus": "setChargeSuccess",
    "failureDetail": ""
}
where
failureDetail will document at what point in the dispatch process the failure occurred.





Force Discharge To Grid
=======================
As we now have nifty functionality to command the inverter as we see fit, Alpha2MQTT has some MQTT topics to send messages to to instruct it to discharge to the grid.  All this is doing is essentially batching up some appropriate Write Data Register commands and exposing them as a single MQTT topic for ease of use.

Publish MQTT messages to:
Alpha2MQTT/request/set/discharge
With the following JSON
{
    "watts": 2500,
    "socPercent": 10,
    "duration": 1800
}
where
watts is the power at which you want to discharge the batteries with.  You can specify a number greater than the inverter/battery supports, the system will limit accordingly.  They provide this functionality in case you don't want to hammer your batteries at full power.

socPercent is where you want discharging to stop.  If you want to stop discharging at 80% as you want to retain the bulk of your stored power, set this to 80.

duration is how long in seconds to put the inverter in this mode.  Discharging to the grid uses 'Dispatch' mode so carries the same caveat as described above in Force Charge From Grid.  I suspect this mode will be leveraged for certain times of day where payments per kWh can be quite rewarding.  I suspect setting a duration in the order of an hour or two will do for the bulk, however I would fully expect Dispatch mode to be terminated by writing 0 to register 0x0880 manually using Write Data Register at the moment you want charging to stop.  It is up to you.


Alpha2MQTT will do the rest and will return the response via the following topic which you can subscribe to:
Alpha2MQTT/response/set/discharge

An example response for the above request could be:
{
    "responseStatus": "setDishargeSuccess",
    "failureDetail": ""
}
where
failureDetail will document at what point in the dispatch process the failure occurred.


Force Back To Normal
====================
And if you want to come out of Force Charge From Grid / Force Discharge To Grid / Any other Dispatch mode, Alpha2MQTT has a similar topic you can leverage.  This topic essentially does a Write to register 0x0880 with value 0, to stop dispatch.

Publish MQTT messages to:
Alpha2MQTT/request/set/normal
With the following JSON
{
    "void": 0
}
where
void is nothing, it just ensures there is some JSON to parse.

Alpha2MQTT will do the rest and will return the response via the following topic which you can subscribe to:
Alpha2MQTT/response/set/normal

An example response for the above request could be:
{
    "responseStatus": "setNormalSuccess",
    "failureDetail": ""
}
where
failureDetail will document at what point in the dispatch process the failure occurred.





Read All Handled Registers
==========================
Finally, an option similar to state, however is done on a request/response basis, is to request batches of handled registers in Alpha2MQTT.  This is intensive and so is only recommended to be pulled when absolutely necessary.  Due to memory limitations as previously explained, it is recommended you pull in batches of 70.

Publish MQTT messages to:
Alpha2MQTT/request/read/register/handled/all
With the following JSON
{
    "start": 0,
    "end": 70
}
where
start is the index of the first register in the handled register list to get.  They start at zero and end around 200.
end is the index of the last register in the handled register list to get.  You don't need to worry about specifying an exact end position, Alpha2MQTT will stop at the last one.

It is recommended you pull 0-70, 71,140, and so on, until you have obtained them all.

Alpha2MQTT will do the rest and will return the response via the following topic which you can subscribe to:
Alpha2MQTT/response/read/register/handled/all

An example response for the above request will be in the likes of:
{
    "REG_BATTERY_HOME_R_BATTERY_POWER": 2845,
    ... repeat 40 or so times ...
    "REG_INVERTER_HOME_R_VOLTAGE_L1": 238.4
}



If at any point you request too much data for the MQTT buffer, you will receive the following payload back:
{
  "mqttError": "Length of payload exceeds X bytes.  Length would be Y bytes."
}
where
X is the number of bytes available in the buffer (max payload size - 1).
Y is the number of bytes ended up being requested.







A quick hex / binary / base 10 starter for 10:
==============================================
One byte = 8 bits
Two bytes = 16 bits
Four bytes = 32 bits

A two byte number in binary can be expressed as so:

0     0     0     0     0     0     0     0       -       0     0     0     0     0     0     0     0

Put an imaginary number above every bit, starting from the right, starting at one and doubling each time

32768 16384 8192  4096  2048  1024  512   256     -       128   64    32    16    8     4     2     1
0     0     0     0     0     0     0     0       -       0     0     0     0     0     0     0     0

Each bit you set to one means you can add those numbers together to get a numeric answer you are used to

32768 16384 8192  4096  2048  1024  512   256     -       128   64    32    16    8     4     2     1
0     0     1     0     0     0     0     0       -       0     0     1     0     0     0     0     1     = 8225

The left block of 8 bits is the high byte, the right block of 8 bits is the low byte.
To convert the binary number into hex, you split the numbers into blocks of four, and put imaginary numbers again above, each block of four bits containing 1, 2, 4 and 8, again from the right:
8     4     2     1  -  8     4     2     1       -       8     4     2     1   -   8     4     2     1
0     0     1     0  -  0     0     0     0       -       0     0     1     0   -   0     0     0     1     = 8225
And add up the bits from each block in the same way
2 - 0 - 2 - 1
So 8225 is 0x2021 in Hex.
Hex numbers go from 0 to 15, with A for 10, B for 11, C for 12, D for 13, E for 14 and F for 15

Using the Modbus address from above, 0x0055
8     4     2     1     8     4     2     1               8     4     2     1     8     4     2     1
0     0     0     0     0     0     0     0       -       1     0     0     0     0     1     0     1     = 85
0                                                         8                       5
0x0085
Individual bytes returned in rawData will be 0, 85 as the following will have happened for each byte (given a the four blocks of four bits will be combined into two blocks of eight):

If an example required writing 0xFF to a register, we'd do the following:
We'd know that F is 15, and we have four of them, so we will arrange our blocks of four like so:
8     4     2     1     8     4     2     1               8     4     2     1       8     4     2     1
1     1     1     1  -  1     1     1     1       -       1     1     1     1   -   1     1     1     1     = 0xFFFF

Put the standard binary numbers above:

32768 16384 8192  4096  2048  1024  512   256             128   64    32    16    8     4     2     1
1     1     1     1     1     1     1     1       -       1     1     1     1     1     1     1     1       = 65535

So 65535 is what will be used in value.

Individual bytes returned in rawData will be 255,255
128 64  32  16  8   4   2   1    - 128 64  32  16  8   4   2   1
1   1   1   1   1   1   1   1      1   1   1   1   1   1   1   1 = 255, 255

The same process is employed for four bytes, the numbers just keep doubling and you end up with two sets of high and low bytes, in order of high byte 1, low byte 1, high byte 2, low byte 2




(c) Daniel Young 2022 daniel@mydan.com

Thanks to Colin McGerty's (colin@mcgerty.co.uk) work on Sofar2MQTT which brought about my intrigue and on which some of Alpha2MQTT's program logic and circuitry is based.
Thanks to the AlphaESS DE Team (https://www.alpha-ess.de/) for latest Modbus documentation and email support.
calcCRC by angelo.compagnucci@gmail.com and jpmzometa@gmail.com