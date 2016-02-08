/*
This library is a Windows Universal port of the original RCSwitch for Arduino library by Suat Özgur.
It was ported by Michael Osthege (2016,  thecakedev(at)hotmail(dot)com) and released under the same license (GNU Lesser General Pulic License)
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



using namespace Platform;
using namespace Windows::Devices::Gpio;
using namespace Windows::Foundation;

namespace RCSwitch {

	public ref class Signal sealed
	{
	public:
		property unsigned int Protocol { public: unsigned int get() { return _Protocol; } private: void set(unsigned int value) { _Protocol = value; } }
		property unsigned int Delay { public: unsigned int get() { return _Delay; } private: void set(unsigned int value) { _Delay = value; } }
		property unsigned int BitLength { public: unsigned int get() { return _BitLength; } private: void set(unsigned int value) { _BitLength = value; } }
		//property unsigned long Value { public: unsigned long get() { return _Value; } private: void set(unsigned long value) { _Value = value; } }
		property Platform::String^ Code { public: String^ get() { return _Code; } private: void set(String^ value) { _Code = value; } }

		Signal(unsigned int protocol, unsigned int delay, unsigned int bitlength, String^ code) {
			this->Protocol = protocol;
			this->BitLength = bitlength;
			this->Delay = delay;
			this->Code = code;
		}
	private:
		unsigned int _Protocol;
		unsigned int _Delay;
		unsigned int _BitLength;
		//unsigned long _Value;
		Platform::String^ _Code;
	};

	public delegate void SignalReceivedHandler(Platform::Object^ sender, Signal^ signal);

	public ref class RCSwitchIO sealed
	{
	public:
		//read
		property bool IsGPIOInitialized { public: bool get() { return _IsGPIOInitialized; } private: void set(bool value) { _IsGPIOInitialized = value; } }
		property bool IsTransmitAvailable { public: bool get() { return _IsTransmitAvailable; } private: void set(bool value) { _IsTransmitAvailable = value; } }
		property int TransmitPinNumber { public: int get() { return _TransmitPinNumber; } private: void set(int value) { _TransmitPinNumber = value; } }
		property int ReceivePinNumber { public: int get() { return _ReceivePinNumber; } private: void set(int value) { _ReceivePinNumber = value; } }
		//readwrite
		property int PulseLength { public: int get() { return _PulseLength; } void set(int value) { _PulseLength = value; } }
		property int RepeatTransmit { public: int get() { return _RepeatTransmit; } void set(int value) { _RepeatTransmit = value; } }
		property int Protocol { public: int get() { return _Protocol; } void set(int value) { _Protocol = value; } }
		//events
		event SignalReceivedHandler^ OnSignalReceived;


		//constructor
		RCSwitchIO(int transmitPin, int receivePin);
		//methods
		bool Switch(String ^ group, String ^ device, bool on);
		bool Switch(int nAddressCode, int nChannelCode, bool on);
		
		

	private:
		int _TransmitPinNumber;
		int _ReceivePinNumber;
		int _PulseLength;
		int _RepeatTransmit;
		int _Protocol;
		bool _IsGPIOInitialized;
		bool _IsTransmitAvailable;
		//the following three variables may only be accessed if GPIO is available || Windows::Foundation::Metadata::ApiInformation::IsTypePresent("Windows::Devices::Gpio::GpioController")
		GpioController^ gpioController;
		GpioPin^ transmitPin;
		GpioPin^ receivePin;
		//receive defines
		#define RCSWITCH_MAX_CHANGES 67
		#define PROTOCOL3_SYNC_FACTOR   71
		#define PROTOCOL3_0_HIGH_CYCLES  4
		#define PROTOCOL3_0_LOW_CYCLES  11
		#define PROTOCOL3_1_HIGH_CYCLES  9
		#define PROTOCOL3_1_LOW_CYCLES   6
		//receive values
		__int64 ticksPerMicrosecond;
		LONGLONG currentTime;
		LONGLONG lastTime;
		
		int nReceiveTolerance = 60;
		unsigned int timings[RCSWITCH_MAX_CHANGES];

		//transmitting
		char* StringToChar(String^ str);
		char* getCodeWordA(char* sGroup, char* sDevice, bool bOn);
		char* getCodeWordB(int nAddressCode, int nChannelCode, bool bStatus);
		void sendTriState(char* Code);

		void sendT0();
		void sendT1();
		void sendTF();
		void send0();
		void send1();
		void sendSync();
		void delayMicroseconds(int us);
		void transmit(int nHighPulses, int nLowPulses);

		void enableReceive();

		void disableReceive();

		//receiving
		void OnValueChanged(GpioPin ^sender, GpioPinValueChangedEventArgs ^args);

		bool receiveProtocol1(unsigned int changeCount);
		bool receiveProtocol2(unsigned int changeCount);
		bool receiveProtocol3(unsigned int changeCount);

		String^ getReceivedCode(unsigned long receivedValue, unsigned int receivedBitlength);
		char* dec2binWzerofill(unsigned long dec, unsigned int length);
		char* bin2tristate(char* bin);
		String^ StringFromAscIIChars(char* chars);
	};
}

