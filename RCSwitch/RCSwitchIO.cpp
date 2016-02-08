/*
This library is a Windows Universal port of the original RCSwitch for Arduino library by Suat Özgur.
It was ported by Michael Osthege (2016) and released under the same license (GNU Lesser General Pulic License)
The original license information can be found below.



RCSwitch - Arduino libary for remote control outlet switches
Copyright (c) 2011 Suat Özgür.  All right reserved.

Contributors:
- Andre Koehler / info(at)tomate-online(dot)de
- Gordeev Andrey Vladimirovich / gordeev(at)openpyro(dot)com
- Skineffect / http://forum.ardumote.com/viewtopic.php?f=2&t=46
- Dominik Fischer / dom_fischer(at)web(dot)de
- Frank Oltmanns / <first name>.<last name>(at)gmail(dot)com
- Andreas Steinel / A.<lastname>(at)gmail(dot)com

Project home: http://code.google.com/p/rc-switch/

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "pch.h"
#include "RCSwitchIO.h"

using namespace RCSwitch;
using namespace Platform;



RCSwitch::RCSwitchIO::RCSwitchIO(int transmitPinNumber, int receivePinNumber)
{
	//public properties/fields
	this->TransmitPinNumber = transmitPinNumber;
	this->ReceivePinNumber = receivePinNumber;
	this->PulseLength = 350;
	this->RepeatTransmit = 10;
	this->Protocol = 1;
	QueryPerformanceFrequency((LARGE_INTEGER*)(&ticksPerMicrosecond));
	ticksPerMicrosecond = ticksPerMicrosecond / 1000000;


	//initialization of GPIO stuff
	if (Windows::Foundation::Metadata::ApiInformation::IsTypePresent("Windows.Devices.Gpio.GpioController"))//check if GPIO is available on this device
	{
		this->gpioController = GpioController::GetDefault();
		if (gpioController != nullptr)
			IsGPIOInitialized = true;
	}
	if (IsGPIOInitialized)
	{
		//open the transmitter pin
		transmitPin = gpioController->OpenPin(TransmitPinNumber);
		if (transmitPin != nullptr)
		{
			IsTransmitAvailable = true;
			transmitPin->SetDriveMode(GpioPinDriveMode::Output);
		}
		//receive signals
		if (receivePinNumber != -1)
		{
			enableReceive();
		}
	}
}



#pragma region From Switch to transmit
bool RCSwitchIO::Switch(String^ group, String^ device, bool on)
{
	if (!IsTransmitAvailable)
		return false;
	//deactivate the interrupt
	bool disabled_Receive = false;
	if (this->ReceivePinNumber != -1) {
		this->disableReceive();
		disabled_Receive = true;//remember to activate it afterwards
	}
	//
	char* group_c = StringToChar(group);
	char* device_c = StringToChar(device);
	this->sendTriState(this->getCodeWordA(group_c, device_c, on));
	//enable interrupt
	if (disabled_Receive) {
		this->enableReceive();
	}
	//
	return true;
}

/**
* Switch a remote switch on (Type B with two rotary/sliding switches)
*
* @param nAddressCode  Number of the switch group (1..4)
* @param nChannelCode  Number of the switch itself (1..4)
*/
bool RCSwitchIO::Switch(int nAddressCode, int nChannelCode, bool on) {
	if (!IsTransmitAvailable)
		return false;
	//deactivate the interrupt
	bool disabled_Receive = false;
	if (this->ReceivePinNumber != -1) {
		this->disableReceive();
		disabled_Receive = true;//remember to activate it afterwards
	}
	//send
	this->sendTriState(this->getCodeWordB(nAddressCode, nChannelCode, on));
	//enable interrupt
	if (disabled_Receive) {
		this->enableReceive();
	}
	//
	return true;
}

char * RCSwitch::RCSwitchIO::StringToChar(String ^ str_String)
{
	//adapted from http://answers.flyppdevportal.com/categories/metro/nativecode.aspx?ID=c39b3060-0c2d-40b7-aa8c-035f1fad441c
	std::wstring wstr(str_String->Data());
	int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
	char* str_char = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), str_char, len, NULL, NULL);
	str_char[len] = '\0';
	return str_char;
}

/**
* Returns a char[13], representing the Code Word to be send.
*
* getCodeWordA(char*, char*)
*
*/
char* RCSwitchIO::getCodeWordA(char* sGroup, char* sDevice, bool bOn) {
	static char sDipSwitches[13];
	int i = 0;
	int j = 0;

	for (i = 0; i < 5; i++) {
		if (sGroup[i] == '0') {
			sDipSwitches[j++] = 'F';
		}
		else {
			sDipSwitches[j++] = '0';
		}
	}

	for (i = 0; i < 5; i++) {
		if (sDevice[i] == '0') {
			sDipSwitches[j++] = 'F';
		}
		else {
			sDipSwitches[j++] = '0';
		}
	}

	if (bOn) {
		sDipSwitches[j++] = '0';
		sDipSwitches[j++] = 'F';
	}
	else {
		sDipSwitches[j++] = 'F';
		sDipSwitches[j++] = '0';
	}

	sDipSwitches[j] = '\0';
	//OutputDebugStringA(sDipSwitches);
	return sDipSwitches;
}

/**
* Returns a char[13], representing the Code Word to be send.
* A Code Word consists of 9 address bits, 3 data bits and one sync bit but in our case only the first 8 address bits and the last 2 data bits were used.
* A Code Bit can have 4 different states: "F" (floating), "0" (low), "1" (high), "S" (synchronous bit)
*
* +-------------------------------+--------------------------------+-----------------------------------------+-----------------------------------------+----------------------+------------+
* | 4 bits address (switch group) | 4 bits address (switch number) | 1 bit address (not used, so never mind) | 1 bit address (not used, so never mind) | 2 data bits (on|off) | 1 sync bit |
* | 1=0FFF 2=F0FF 3=FF0F 4=FFF0   | 1=0FFF 2=F0FF 3=FF0F 4=FFF0    | F                                       | F                                       | on=FF off=F0         | S          |
* +-------------------------------+--------------------------------+-----------------------------------------+-----------------------------------------+----------------------+------------+
*
* @param nAddressCode  Number of the switch group (1..4)
* @param nChannelCode  Number of the switch itself (1..4)
* @param bStatus       Wether to switch on (true) or off (false)
*
* @return char[13]
*/
char* RCSwitchIO::getCodeWordB(int nAddressCode, int nChannelCode, bool bStatus) {
	int nReturnPos = 0;
	static char sReturn[13];

	char* code[5] = { "FFFF", "0FFF", "F0FF", "FF0F", "FFF0" };
	if (nAddressCode < 1 || nAddressCode > 4 || nChannelCode < 1 || nChannelCode > 4) {
		return '\0';
	}
	for (int i = 0; i<4; i++) {
		sReturn[nReturnPos++] = code[nAddressCode][i];
	}

	for (int i = 0; i<4; i++) {
		sReturn[nReturnPos++] = code[nChannelCode][i];
	}

	sReturn[nReturnPos++] = 'F';
	sReturn[nReturnPos++] = 'F';
	sReturn[nReturnPos++] = 'F';

	if (bStatus) {
		sReturn[nReturnPos++] = 'F';
	}
	else {
		sReturn[nReturnPos++] = '0';
	}

	sReturn[nReturnPos] = '\0';

	return sReturn;
}


/**
* @param sCodeWord   /^[10FS]*$/  -> see getCodeWord
*/
void RCSwitchIO::sendTriState(char* sCodeWord) {
	for (int nRepeat = 0; nRepeat<RepeatTransmit; nRepeat++) {
		int i = 0;
		while (sCodeWord[i] != '\0') {
			switch (sCodeWord[i]) {
			case '0':
				this->sendT0();
				break;
			case 'F':
				this->sendTF();
				break;
			case '1':
				this->sendT1();
				break;
			}
			i++;
		}
		this->sendSync();
	}
}

/**
* Sends a Tri-State "0" Bit
*            _     _
* Waveform: | |___| |___
*/
void RCSwitchIO::sendT0() {
	this->transmit(1, 3);
	this->transmit(1, 3);
}

/**
* Sends a Tri-State "1" Bit
*            ___   ___
* Waveform: |   |_|   |_
*/
void RCSwitchIO::sendT1() {
	this->transmit(3, 1);
	this->transmit(3, 1);
}

/**
* Sends a Tri-State "F" Bit
*            _     ___
* Waveform: | |___|   |_
*/
void RCSwitchIO::sendTF() {
	this->transmit(1, 3);
	this->transmit(3, 1);
}

/**
* Sends a "0" Bit
*                       _
* Waveform Protocol 1: | |___
*                       _
* Waveform Protocol 2: | |__
*/
void RCSwitchIO::send0() {
	if (this->Protocol == 1) {
		this->transmit(1, 3);
	}
	else if (this->Protocol == 2) {
		this->transmit(1, 2);
	}
	else if (this->Protocol == 3) {
		this->transmit(4, 11);
	}
}

/**
* Sends a "1" Bit
*                       ___
* Waveform Protocol 1: |   |_
*                       __
* Waveform Protocol 2: |  |_
*/
void RCSwitchIO::send1() {
	if (this->Protocol == 1) {
		this->transmit(3, 1);
	}
	else if (this->Protocol == 2) {
		this->transmit(2, 1);
	}
	else if (this->Protocol == 3) {
		this->transmit(9, 6);
	}
}

/**
* Sends a "Sync" Bit
*                       _
* Waveform Protocol 1: | |_______________________________
*                       _
* Waveform Protocol 2: | |__________
*/
void RCSwitchIO::sendSync() {

	if (this->Protocol == 1) {
		this->transmit(1, 31);
	}
	else if (this->Protocol == 2) {
		this->transmit(1, 10);
	}
	else if (this->Protocol == 3) {
		this->transmit(1, 71);
	}
}


void RCSwitchIO::delayMicroseconds(int microseconds) {
	// implementation of a microsecond delay
	//std::this_thread::sleep_for(std::chrono::microseconds(us));	// this does not work

	//the following implementation from http://www.edaboard.com/thread71656.html works perfectly!

	__int64 timeRightNow;
	__int64 timeStart;
	
	__int64 timeToWait = (double)ticksPerMicrosecond * (double)microseconds;

	QueryPerformanceCounter((LARGE_INTEGER*)(&timeStart));

	timeRightNow = timeStart;

	while ((timeRightNow - timeStart) < timeToWait)
	{
		QueryPerformanceCounter((LARGE_INTEGER*)(&timeRightNow));
	};
}

void RCSwitchIO::transmit(int nHighPulses, int nLowPulses) {
	if (this->TransmitPinNumber != -1 && transmitPin != nullptr) {
		//debugCounter += nHighPulses + nLowPulses;

		transmitPin->Write(GpioPinValue::High);
		//OnValueChanged(nullptr, nullptr);
		delayMicroseconds(this->PulseLength * nHighPulses);

		transmitPin->Write(GpioPinValue::Low);
		//OnValueChanged(nullptr, nullptr);
		delayMicroseconds(this->PulseLength * nLowPulses);
	}
}
#pragma endregion

#pragma region Enabling and disabling the receive interrupt
void RCSwitchIO::enableReceive() {
	if (this->ReceivePinNumber != -1) {
		//nReceivedValue = NULL;
		//nReceivedBitlength = NULL;
		//attachInterrupt(this->nReceiverInterrupt, handleInterrupt, CHANGE);

		// the Windows 10 IoT implementation is follows:
		if (gpioController != nullptr)
		{
			receivePin = gpioController->OpenPin(ReceivePinNumber);
			receivePin->SetDriveMode(GpioPinDriveMode::Input);
			receivePin->ValueChanged += ref new Windows::Foundation::TypedEventHandler<Windows::Devices::Gpio::GpioPin ^, Windows::Devices::Gpio::GpioPinValueChangedEventArgs ^>(this, &RCSwitchIO::OnValueChanged);
		}
	}
}

/**
* Disable receiving data
*/
void RCSwitchIO::disableReceive() {
	//detachInterrupt(this->nReceiverInterrupt);
	receivePin = nullptr;
}

#pragma endregion

#pragma region Interpreting the Receive Interrupts
void RCSwitchIO::OnValueChanged(GpioPin ^sender, GpioPinValueChangedEventArgs ^args)
{
	static unsigned int changeCount;
	static unsigned int repeatCount;

	
	QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);//get the current time point
	if (lastTime == 0)
		lastTime = currentTime;
	//TODO: problem is probably related to the time measurement
	double difference = (double)(currentTime - lastTime);
	double duration = difference / ticksPerMicrosecond;//calculate the microseconds since lastTime

	if (duration > 5000 && duration > timings[0] - 200 && duration < timings[0] + 200) {
		repeatCount++;
		changeCount--;
		if (repeatCount == 2) {
			if (receiveProtocol1(changeCount) == false) {
				if (receiveProtocol2(changeCount) == false) {
					if (receiveProtocol3(changeCount) == false) {
						//failed
					}
				}
			}
			repeatCount = 0;
		}
		changeCount = 0;
	}
	else if (duration > 5000)//no flank for 5 ms
	{
		changeCount = 0;
	}

	if (changeCount >= RCSWITCH_MAX_CHANGES) {
		changeCount = 0;
		repeatCount = 0;
	}
	timings[changeCount++] = duration;
	lastTime = currentTime;
}


/**
*
*/
bool RCSwitchIO::receiveProtocol1(unsigned int changeCount) {

	unsigned long code = 0;
	unsigned long delay = timings[0] / 31;
	unsigned long delayTolerance = delay * nReceiveTolerance * 0.01;

	for (int i = 1; i<changeCount; i = i + 2) {

		if (timings[i] > delay - delayTolerance && timings[i] < delay + delayTolerance && timings[i + 1] > delay * 3 - delayTolerance && timings[i + 1] < delay * 3 + delayTolerance) {
			code = code << 1;
		}
		else if (timings[i] > delay * 3 - delayTolerance && timings[i] < delay * 3 + delayTolerance && timings[i + 1] > delay - delayTolerance && timings[i + 1] < delay + delayTolerance) {
			code += 1;
			code = code << 1;
		}
		else {
			// Failed
			i = changeCount;
			code = 0;
		}
	}
	code = code >> 1;
	if (changeCount > 6) {    // ignore < 4bit values as there are no devices sending 4bit values => noise
		unsigned long nReceivedValue = code;
		unsigned int nReceivedBitlength = changeCount / 2;
		unsigned int nReceivedDelay = delay;
		unsigned int nReceivedProtocol = 1;

		String^ rcode = getReceivedCode(nReceivedValue, nReceivedBitlength);
		Signal^ signal = ref new Signal(nReceivedProtocol, nReceivedDelay, nReceivedBitlength, rcode);
		OnSignalReceived(this, signal);
	}

	if (code == 0)
		return false;
	else//if (code != 0)
		return true;
}

bool RCSwitchIO::receiveProtocol2(unsigned int changeCount) {

	unsigned long code = 0;
	unsigned long delay = timings[0] / 10;
	unsigned long delayTolerance = delay * nReceiveTolerance * 0.01;

	for (int i = 1; i<changeCount; i = i + 2) {

		if (timings[i] > delay - delayTolerance && timings[i] < delay + delayTolerance && timings[i + 1] > delay * 2 - delayTolerance && timings[i + 1] < delay * 2 + delayTolerance) {
			code = code << 1;
		}
		else if (timings[i] > delay * 2 - delayTolerance && timings[i] < delay * 2 + delayTolerance && timings[i + 1] > delay - delayTolerance && timings[i + 1] < delay + delayTolerance) {
			code += 1;
			code = code << 1;
		}
		else {
			// Failed
			i = changeCount;
			code = 0;
		}
	}
	code = code >> 1;
	if (changeCount > 6) {    // ignore < 4bit values as there are no devices sending 4bit values => noise
		unsigned long nReceivedValue = code;
		unsigned int nReceivedBitlength = changeCount / 2;
		unsigned int nReceivedDelay = delay;
		unsigned int nReceivedProtocol = 2;

		String^ rcode = getReceivedCode(nReceivedValue, nReceivedBitlength);
		Signal^ signal = ref new Signal(nReceivedProtocol, nReceivedDelay, nReceivedBitlength, rcode);
		OnSignalReceived(this, signal);
	}

	if (code == 0)
		return false;
	else//if (code != 0)
		return true;
}

/** Protocol 3 is used by BL35P02.
*
*/
bool RCSwitchIO::receiveProtocol3(unsigned int changeCount) {

	unsigned long code = 0;
	unsigned long delay = timings[0] / PROTOCOL3_SYNC_FACTOR;
	unsigned long delayTolerance = delay * nReceiveTolerance * 0.01;

	for (int i = 1; i<changeCount; i = i + 2) {

		if (timings[i]   > delay*PROTOCOL3_0_HIGH_CYCLES - delayTolerance
			&& timings[i]   < delay*PROTOCOL3_0_HIGH_CYCLES + delayTolerance
			&& timings[i + 1] > delay*PROTOCOL3_0_LOW_CYCLES - delayTolerance
			&& timings[i + 1] < delay*PROTOCOL3_0_LOW_CYCLES + delayTolerance) {
			code = code << 1;
		}
		else if (timings[i]   > delay*PROTOCOL3_1_HIGH_CYCLES - delayTolerance
			&& timings[i]   < delay*PROTOCOL3_1_HIGH_CYCLES + delayTolerance
			&& timings[i + 1] > delay*PROTOCOL3_1_LOW_CYCLES - delayTolerance
			&& timings[i + 1] < delay*PROTOCOL3_1_LOW_CYCLES + delayTolerance) {
			code += 1;
			code = code << 1;
		}
		else {
			// Failed
			i = changeCount;
			code = 0;
		}
	}
	code = code >> 1;
	if (changeCount > 6) {    // ignore < 4bit values as there are no devices sending 4bit values => noise
		unsigned long nReceivedValue = code;
		unsigned int nReceivedBitlength = changeCount / 2;
		unsigned int nReceivedDelay = delay;
		unsigned int nReceivedProtocol = 3;

		String^ rcode = getReceivedCode(nReceivedValue, nReceivedBitlength);
		Signal^ signal = ref new Signal(nReceivedProtocol, nReceivedDelay, nReceivedBitlength, rcode);
		OnSignalReceived(this, signal);
	}

	if (code == 0)
		return false;
	else//if (code != 0)
		return true;
}

#pragma endregion



String^ RCSwitchIO::getReceivedCode(unsigned long receivedValue, unsigned int receivedBitlength) {
	char* bin = dec2binWzerofill(receivedValue, receivedBitlength);
	char* tristate = bin2tristate(bin);
	//char* tristate = bin2tristate(dec2binWzerofill(nReceivedValue, nReceivedBitlength));
	std::string s_str = std::string(tristate);
	std::wstring wid_str = std::wstring(s_str.begin(), s_str.end());
	const wchar_t* w_char = wid_str.c_str();
	Platform::String^ p_string = ref new Platform::String(w_char);
	return p_string;
}

char* RCSwitchIO::dec2binWzerofill(unsigned long Dec, unsigned int bitLength) {
	char bin[64];
	unsigned int i = 0;

	while (Dec > 0) {
		bin[32 + i++] = (Dec & 1 > 0) ? '1' : '0';
		Dec = Dec >> 1;
	}

	for (unsigned int j = 0; j< bitLength; j++) {
		if (j >= bitLength - i) {
			bin[j] = bin[31 + i - (j - (bitLength - i))];
		}
		else {
			bin[j] = '0';
		}
	}
	bin[bitLength] = '\0';

	return bin;
}


char* RCSwitchIO::bin2tristate(char* bin) {
	char returnValue[50];
	int pos = 0;
	int pos2 = 0;
	while (bin[pos] != '\0' && bin[pos + 1] != '\0') {
		if (bin[pos] == '0' && bin[pos + 1] == '0') {
			returnValue[pos2] = '0';
		}
		else if (bin[pos] == '1' && bin[pos + 1] == '1') {
			returnValue[pos2] = '1';
		}
		else if (bin[pos] == '0' && bin[pos + 1] == '1') {
			returnValue[pos2] = 'F';
		}
		else {
			return "not applicable";
		}
		pos = pos + 2;
		pos2++;
	}
	returnValue[pos2] = '\0';
	return returnValue;
}


String^ StringFromAscIIChars(char* chars)
{
	size_t newsize = strlen(chars) + 1;
	wchar_t * wcstring = new wchar_t[newsize];
	size_t convertedChars = 0;
	mbstowcs_s(&convertedChars, wcstring, newsize, chars, _TRUNCATE);
	String^ str = ref new Platform::String(wcstring);
	delete[] wcstring;
	return str;
}