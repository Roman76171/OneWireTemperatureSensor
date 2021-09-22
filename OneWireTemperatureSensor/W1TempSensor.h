#pragma once
#ifndef W1TempSensor

#include <sstream>
#include <string>
#include <array>
#include <vector>

void InitLib();

class W1TempSensor
{
public:
	enum class DeviceAction
	{
		ADD,
		REMOVE,
	};

	enum class PullupSetting
	{
		ENABLE,
		DISABLE,
		AUTO,
	};

	enum class DeviceType
	{
		DS18S20,
		DS1822,
		DS18B20,
		DS1825,
		DS28EA00,
		COUNT,
	};

	struct Device
	{
		DeviceType familyCode;
		std::string serialNumber;
	};

	using deviceList_t = std::vector<Device>;

	//Gets a list of temperature sensors with alarm. Analogue of the command "Search Alarms" for DS18B20. Root rights may be required.
	//Return type: vector<Device>
	static deviceList_t alarmSearch();

	//Gets data about the number of connected devices type temperature sensor.
	static int getCountDevice();

	//Gets a list of all connected temperature sensors.
	//Return type: vector<Device>
	static deviceList_t getDevices();

	//Gets 5V strong pullup setting.
	static PullupSetting getPullup();

	//Manually adding or removing a temperature sensor. Important: by default, devices are added and removed on their own at the driver level. Root rights may be required.
	//It makes the most sense when using manual search, rather than automatic. Note: by default, automatic search is used, and the addition of temperature sensors.
	static void manualDeviceControl(const Device device, const DeviceAction deviceAction);

	//Sets 5V strong pullup. May need root permission to change value.
	static void setPullup(PullupSetting pullupSetting);	
	
	//Performing actions without being tied to a specific device. Only valid for systems with one sensor.
    //Important: if there are no sensors, an exception is thrown, if there are more than one sensors, the first one from the list of devices is selected.
	W1TempSensor();

	//Performing actions associated with a specific device. A safer option.
	W1TempSensor(const Device device);

	//Gets data about the device with which it is interacting.
	Device getDevice();

	//Gets data with temperature limits, when exceeded, the device goes into alarm mode.
	std::array<int, 2> getMaxMinTemp();

	//Gets the current temperature resolution.
	int getResolution();

	//Gets the current temerature.
	double getTemperature();

	//Gets the current power mode of the device.
	//Return value:
	//   True - the device is connected to an external power source;
	//   False - the device is connected according to the scheme with parasitic power supply.
	bool hasDeviceExternalPower();

	//Restores the state from EEPROM to SRAM as if the device had just been connected. May need root permission.
	void restoreEepromDevice();	

	//Writes status from SRAM to EEPROM. May need root permission.
	//Note: when connecting a parasitic power supply, a number of requirements for connecting to the power supply must be met.
	void saveToEepromDevice();

	//Allows you to select a specific device.
	void setDevice(const Device device);

	//Sets the temperature limits, when exceeded, the device goes into alarm mode. May need root permission to change values.
	void setMaxMinTemp(int minTemp, int maxTemp);

	//Sets new temperature resolution. May need root permission to change value.
	void setResolution(const int resolution);

private:
	using ramDevice_t = std::array<int, 9>;

	//Checks if all temperature sensors connected to the OneWire bus are connected from an external power source.
	static bool hasAllDeviceExternalPower();

	//Reads data from a file: workFolder/fileName. Data is separated symbol NewLine.
	static std::stringstream readFromFile(const std::string& workFolder, const std::string& fileName);

	//Writes data from a file: workFolder/fileName. Data is separated symbol NewLine.
	static void writeToFile(const std::string& workFolder, const std::string& fileName, std::stringstream& data);

	//Saves to the device's SRAM.
	ramDevice_t saveToRamDevice();

	Device m_device;
};

#endif // !W1TempSensor