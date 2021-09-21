#pragma once
#ifndef W1TempSensor

#include <sstream>
#include <string>
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

	//Gets data about the number of connected devices type temperature sensor.
	static int getCountDevice();

	//Gets a list of all connected temperature sensors.
	//Return type: vector<string> 
	static deviceList_t getDevices();

	//Gets 5V strong pullup setting.
	static PullupSetting getPullup();

	//Manually adding or removing a temperature sensor. Important: by default, devices are added and removed on their own at the driver level. Root rights may be required.
	static void manualDeviceControl(const Device device, const DeviceAction deviceAction);

	//Sets 5V strong pullup. May need root permission to change value.
	static void setPullup(const PullupSetting pullupSetting);	
	
	W1TempSensor();

	W1TempSensor(const Device device);

	void setDevice(const Device device);

private:	
	//Checks if all temperature sensors connected to the OneWire bus are connected from an external power source.
	static bool hasAllDeviceExternalPower();

	//Reads data from a file: workFolder/fileName. Data is separated by '\n'.
	static std::stringstream readFromFile(const std::string& workFolder, const std::string& fileName);

	//Writes data from a file: workFolder/fileName. Data is separated by '\n'.
	static void writeToFile(const std::string& workFolder, const std::string& fileName, std::stringstream& data);

	Device m_device;
};

#endif // !W1TempSensor
