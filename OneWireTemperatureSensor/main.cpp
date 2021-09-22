#include <iostream>
#include <string>
#include "W1TempSensor.h"

std::string deviceTypeToString(const W1TempSensor::DeviceType deviceType);

int main(void)
{	
	InitLib();
	std::cout << "Count devices: " << W1TempSensor::getCountDevice() << ".\n";
	std::cout << "List devices:\n";
	auto deviceList{ W1TempSensor::getDevices() };
	for (size_t index = 0; index < deviceList.size(); index++)
	{
		std::cout << "  " << index + 1 << ". Device: " << deviceTypeToString(deviceList[index].familyCode) << ", serial number: " << deviceList[index].serialNumber << ". ";
		W1TempSensor tempSensor{ deviceList[index] };
		std::cout << "Temperature: " << tempSensor.getTemperature() << " degrees Celsius, resolution temperature conversion: " << tempSensor.getResolution() << " bit;\n";
	}
	std::cout << "\nThe program has finished executing. Press Enter to continue ..." << std::endl;
	std::cin.get();
	return 0;
}

std::string deviceTypeToString(const W1TempSensor::DeviceType deviceType)
{
	using deviceType_t = W1TempSensor::DeviceType;
	switch (deviceType)
	{
	case deviceType_t::DS18S20:
		return "DS18S20";
	case deviceType_t::DS1822:
		return "DS1822";
	case deviceType_t::DS18B20:
		return "DS18B20";
	case deviceType_t::DS1825:
		return "DS1825";
	case deviceType_t::DS28EA00:
		return "DS28EA00";
	default:
		return "Unknown device!";
	}
}