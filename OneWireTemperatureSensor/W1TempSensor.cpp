#include <iostream>
#include <fstream>
#include <dirent.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <cmath>
#include "W1TempSensor.h"

using fileList_t = std::vector<std::string>;

const std::string BASE_FOLDER{ "/sys/bus/w1/devices" };
static std::string MASTER_FOLDER{ "w1_bus_masterX" };

fileList_t findFile(const std::string& fileNamePatter, const std::string& searchFolder=".");
W1TempSensor::DeviceType toDeviceType(const std::string& deviceType);
std::string DeviceTypeToString(const W1TempSensor::DeviceType deviceType);

///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////

void InitLib()
{
	MASTER_FOLDER = "w1_bus_master1";
}

///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////

//Gets a list of temperature sensors with alarm. Analogue of the command "Search Alarms" for DS18B20.
//Return type: vector<Device>
W1TempSensor::deviceList_t W1TempSensor::alarmSearch()
{
	const std::string fileTemperatureConversionAllDevice{ "therm_bulk_read" };

	std::stringstream stream{};
	stream << "trigger";
	writeToFile(BASE_FOLDER + '/' + MASTER_FOLDER, fileTemperatureConversionAllDevice, stream);
	int statusOperation{ 1 };
	do
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(750));
		stream = readFromFile(BASE_FOLDER + '/' + MASTER_FOLDER, fileTemperatureConversionAllDevice);		
		stream >> statusOperation;
	} while (statusOperation == -1);
	deviceList_t devicesAlarmSignal{};
	for (auto device : getDevices())
	{
		W1TempSensor tempSensor{ device };
		std::array<int, 2> tempAlarms{ tempSensor.getMaxMinTemp() };
		int temp{ static_cast<int>(round(tempSensor.getTemperature())) };
		if (tempAlarms[0] > temp || tempAlarms[1] < temp)
		{
			devicesAlarmSignal.push_back(device);
		}
	}
	return devicesAlarmSignal;
}

//Returns data about the number of connected devices type temperature sensor.
int W1TempSensor::getCountDevice()
{
	const std::string fileSlaveDeviceCount{ "w1_master_slave_count" };
	
	std::stringstream in{ readFromFile(BASE_FOLDER + '/' + MASTER_FOLDER, fileSlaveDeviceCount) };
	int countDevice{ 0 };
	in >> countDevice;
	return countDevice;
}

//Returns a list of all connected temperature sensors.
//Return type: vector<Device> 
W1TempSensor::deviceList_t W1TempSensor::getDevices()
{
	const std::string fileSlaveDevices{ "w1_master_slaves" };

	std::stringstream in{ readFromFile(BASE_FOLDER + '/' + MASTER_FOLDER, fileSlaveDevices) };
	deviceList_t deviceList{};
	if (getCountDevice() == 0)
	{
		return deviceList;
	}
	while (!in.eof())
	{
		std::string str{};
		std::getline(in, str);
		if (str.length() > 0)
		{
			std::istringstream istr{ str };
			std::string strDeviceType{};
			std::getline(istr, strDeviceType, '-');
			std::string strSerialNumber{};
			std::getline(istr, strSerialNumber);
			Device device{ toDeviceType(strDeviceType), strSerialNumber };
			deviceList.push_back(std::move(device));
		}
	}
	return deviceList;
}

//Manually adding or removing a temperature sensor. Important: by default, devices are added and removed on their own at the driver level. Root rights may be required.
void W1TempSensor::manualDeviceControl(const Device device, const DeviceAction deviceAction)
{
	const std::string fileManuallyAddDevice{ "w1_master_add" };
	const std::string fileManuallyRemoveDevice{ "w1_master_remove" };

	std::string deviceName{ DeviceTypeToString(device.familyCode) + '-' + device.serialNumber };
	std::stringstream out{ deviceName };
	if (deviceAction == DeviceAction::ADD)
	{
		writeToFile(BASE_FOLDER + '/' + MASTER_FOLDER, fileManuallyAddDevice, out);
	}
	else if (deviceAction == DeviceAction::REMOVE)
	{
		writeToFile(BASE_FOLDER + '/' + MASTER_FOLDER, fileManuallyRemoveDevice, out);
	}
}

//Gets 5V strong pullup setting.
W1TempSensor::PullupSetting W1TempSensor::getPullup()
{
	const std::string filePullup{ "w1_master_pullup" };

	std::stringstream in{ readFromFile(BASE_FOLDER + '/' + MASTER_FOLDER, filePullup) };
	int pullupSetting{ 1 };
	in >> pullupSetting;
	return (pullupSetting ? PullupSetting::DISABLE : PullupSetting::ENABLE);
}

//Sets 5V strong pullup. May need root permission to change value.
//Param: 
//   ENABLE -- 5V strong pullup enabled;
//   DISABLE -- 5V strong pullup 0 disabled;
//   AUTO -- Interrogates all temperature sensors connected to the OneWire bus, and if at least one sensor is detected in the "parasitic power" mode, enabled 5V strong pullup.
void W1TempSensor::setPullup(PullupSetting pullupSetting)
{
	const std::string filePullup{ "w1_master_pullup" };

	std::stringstream out{};
	if (pullupSetting == PullupSetting::ENABLE)
	{
		out << 0;
	}
	else if (pullupSetting == PullupSetting::DISABLE)
	{
		out << 1;
	}
	else if (pullupSetting == PullupSetting::AUTO)
	{
		if (hasAllDeviceExternalPower())
		{
			out << 1;
			pullupSetting = PullupSetting::DISABLE;
		}
		else
		{
			out << 0;
			pullupSetting = PullupSetting::ENABLE;
		}
	}
	writeToFile(BASE_FOLDER + '/' + MASTER_FOLDER, filePullup, out);
	PullupSetting pullupSettingNew{ getPullup() };
	if (pullupSetting != pullupSettingNew)
	{
		throw std::runtime_error("Failed to change pullup setting!");
	}
}

//Performing actions without being tied to a specific device. Only valid for systems with one sensor.
//Important: if there are no sensors, an exception is thrown, if there are more than one sensors, the first one from the list of devices is selected.
W1TempSensor::W1TempSensor() : m_device{}
{
	try
	{
		deviceList_t deviceList{ getDevices() };
		if (deviceList.size() == 0)
		{
			throw std::runtime_error("No devices available!");
		}
		m_device = deviceList[0];
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(exception.what());
	}
}

//Performing actions associated with a specific device. A safer option.
W1TempSensor::W1TempSensor(const Device device) : m_device{ device }
{

}

//Gets data about the device with which it is interacting.
W1TempSensor::Device W1TempSensor::getDevice()
{
	return m_device;
}

//Gets data with temperature limits, when exceeded, the device goes into alarm mode.
std::array<int, 2> W1TempSensor::getMaxMinTemp()
{
	const std::string fileMinMaxTemp{ "alarms" };
	const std::string deviceFolder{ DeviceTypeToString(m_device.familyCode) + '-' + m_device.serialNumber };

	std::stringstream in{ readFromFile(BASE_FOLDER + '/' + deviceFolder, fileMinMaxTemp) };
	std::array<int, 2> alarmTemp{};
	in >> alarmTemp[0] >> alarmTemp[1];
	return alarmTemp;
}

//Gets the current temperature resolution.
int W1TempSensor::getResolution()
{
	const std::string fileResolutionDevice{ "resolution" };
	const std::string deviceFolder{ DeviceTypeToString(m_device.familyCode) + '-' + m_device.serialNumber };

	std::stringstream in{ readFromFile(BASE_FOLDER + '/' + deviceFolder, fileResolutionDevice) };
	int resolution{ 12 };
	in >> resolution;
	return resolution;
}

//Gets the current temerature.
double W1TempSensor::getTemperature()
{
	const std::string fileTemperature{ "temperature" };
	const std::string deviceFolder{ DeviceTypeToString(m_device.familyCode) + '-' + m_device.serialNumber };

	std::stringstream in{ readFromFile(BASE_FOLDER + '/' + deviceFolder, fileTemperature) };
	int milliTemperature{ -56 * 1000 };
	in >> milliTemperature;
	double temperature{ static_cast<double>(milliTemperature) / 1000 };
	return temperature;
}

// Gets the current power mode of the device.
//Return value:
//   True - the device is connected to an external power source;
//   False - the device is connected according to the scheme with parasitic power supply.
bool W1TempSensor::hasDeviceExternalPower()
{
	const std::string fileDevicePowerStatus{ "ext_power" };
	const std::string deviceFolder{ DeviceTypeToString(m_device.familyCode) + '-' + m_device.serialNumber };

	std::stringstream in{ readFromFile(BASE_FOLDER + '/' + deviceFolder, fileDevicePowerStatus) };
	int powerStatus{ 1 };
	in >> powerStatus;
	return static_cast<bool>(powerStatus);
}

//Restores the state from EEPROM to SRAM as if the device had just been connected.
void W1TempSensor::restoreEepromDevice()
{
	const std::string fileEeprom{ "eeprom" };
	const std::string deviceFolder{ DeviceTypeToString(m_device.familyCode) + '-' + m_device.serialNumber };

	std::stringstream out{};
	out << "restore";
	writeToFile(BASE_FOLDER + '/' + deviceFolder, fileEeprom, out);
}

//Writes status from SRAM to EEPROM.
//Note: when connecting a parasitic power supply, a number of requirements for connecting to the power supply must be met.
void W1TempSensor::saveToEepromDevice()
{
	const std::string fileEeprom{ "eeprom" };
	const std::string deviceFolder{ DeviceTypeToString(m_device.familyCode) + '-' + m_device.serialNumber };

	std::stringstream out{};
	out << "save";
	writeToFile(BASE_FOLDER + '/' + deviceFolder, fileEeprom, out);
}

//Allows you to select a specific device.
void W1TempSensor::setDevice(const Device device)
{
	m_device = device;
}

//Sets the temperature limits, when exceeded, the device goes into alarm mode.
void W1TempSensor::setMaxMinTemp(int minTemp, int maxTemp)
{
	const std::string fileMinMaxTemp{ "alarms" };
	const std::string deviceFolder{ DeviceTypeToString(m_device.familyCode) + '-' + m_device.serialNumber };

	if (minTemp > maxTemp)
	{
		int tmp{ maxTemp };
		maxTemp = minTemp;
		minTemp = tmp;
	}
	if (minTemp < -55 || maxTemp > 125)
	{
		throw std::runtime_error("Permissible temperature values from -55 to 125.");
	}
	std::stringstream out{};
	out << minTemp << ' ' << maxTemp;
	writeToFile(BASE_FOLDER + '/' + deviceFolder, fileMinMaxTemp, out);
	auto maxMinTempNew{ getMaxMinTemp() };
	if (maxMinTempNew[0] != minTemp && maxMinTempNew[1] != maxTemp)
	{
		throw std::runtime_error("Failed to change maximum and minimum values!");
	}
	saveToRamDevice();
}

//Sets new temperature resolution.
void W1TempSensor::setResolution(const int resolution)
{
	const std::string fileResolutionDevice{ "resolution" };
	const std::string deviceFolder{ DeviceTypeToString(m_device.familyCode) + '-' + m_device.serialNumber };

	if (resolution < 9 || resolution > 12)
	{
		throw std::runtime_error("Permissible resolution values from 9 to 12.");
	}
	std::stringstream out{};
	out << resolution;
	writeToFile(BASE_FOLDER + '/' + deviceFolder, fileResolutionDevice, out);
	int resolutionNew{ getResolution() };
	if (resolution != resolutionNew)
	{
		throw std::runtime_error("Failed to change resolution!");
	}
	saveToRamDevice();
}

///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////

//Checks if all temperature sensors connected to the OneWire bus are connected from an external power source.
//Return value:
//   True -- Are all temperature sensors connected to an external power source;
//   False -- At least one temperature sensor is connected according to a parasitic power supply scheme.
bool W1TempSensor::hasAllDeviceExternalPower()
{
	const std::string fileDevicePowerStatus{ "ext_power" };

	deviceList_t deviceList{ getDevices() };
	for (auto device : deviceList)
	{
		const std::string deviceFolder{ DeviceTypeToString(device.familyCode) + '-' + device.serialNumber };

		std::stringstream in{ readFromFile(BASE_FOLDER + '/' + deviceFolder, fileDevicePowerStatus) };
		int powerStatus{ 0 };
		in >> powerStatus;
		if (!powerStatus)
		{
			return false;
		}
	}
	return true;
}

//Reads data from a file: workFolder/fileName. Data is separated by '\n'.
std::stringstream W1TempSensor::readFromFile(const std::string& workFolder, const std::string& fileName)
{
	std::ifstream fin{};
	fin.open(workFolder + '/' + fileName);
	if (!fin.is_open())
	{
		throw std::runtime_error("Can't open file: " + fileName + " in folder " + workFolder + " !");
	}
	std::stringstream sstr{};
	while (!fin.eof())
	{
		std::string str{};
		std::getline(fin, str);
		sstr << str << '\n';
	}
	fin.close();
	return sstr;
}

//Writes data from a file: workFolder/fileName. Data is separated by "\n".
void W1TempSensor::writeToFile(const std::string& workFolder, const std::string& fileName, std::stringstream& data)
{
	std::ofstream fout{};
	fout.open(workFolder + '/' + fileName, std::ios::trunc);
	if (!fout.is_open())
	{
		throw std::runtime_error("Can't open file: " + fileName + " in folder " + workFolder + " !");
	}
	while (!data.eof())
	{
		std::string str{};
		std::getline(data, str);
		fout << str << '\n';
	}
	fout.flush();
	fout.close();
}

//Saves to the device's SRAM.
W1TempSensor::ramDevice_t W1TempSensor::saveToRamDevice()
{
	const std::string fileSlave{ "w1_slave" };
	const std::string deviceFolder{ DeviceTypeToString(m_device.familyCode) + '-' + m_device.serialNumber };

	std::stringstream stream{ readFromFile(BASE_FOLDER + '/' + deviceFolder, fileSlave) };
	ramDevice_t ram{};
	for (size_t i = 0; i < 9; i++)
	{
		std::string str{};
		std::getline(stream, str, ' ');
		ram[i] = std::stoi(str, 0, 16);
	}
	return ram;
}

///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////

fileList_t findFile(const std::string& fileNamePatter, const std::string& searchFolder)
{
	DIR* folder{ opendir(searchFolder.c_str()) };
	fileList_t folderList{};
	if (folder)
	{		
		dirent* file{ readdir(folder) };
		while (file)
		{
			if ((file->d_name != "..") && (strstr(file->d_name, fileNamePatter.c_str())))
			{
				folderList.push_back(file->d_name);
			}
			file = readdir(folder);
		}
		closedir(folder);
	}
	else
	{
		throw std::runtime_error("Can't open directory: " + searchFolder + " !");
	}
	return folderList;
}

W1TempSensor::DeviceType toDeviceType(const std::string& deviceType)
{
	using device_t = W1TempSensor::DeviceType;
	if (deviceType == "10")
	{
		return device_t::DS18S20;
	}
	else if (deviceType == "22")
	{
		return device_t::DS1822;
	}
	else if (deviceType == "28")
	{
		return device_t::DS18B20;
	}
	else if (deviceType == "3B")
	{
		return device_t::DS1825;
	}
	else if (deviceType == "42")
	{
		return device_t::DS28EA00;
	}
	else
	{
		throw std::runtime_error("Unknown device!");
	}
}

std::string DeviceTypeToString(const W1TempSensor::DeviceType deviceType)
{
	using device_t = W1TempSensor::DeviceType;
	switch (deviceType)
	{
	case device_t::DS18S20:
		return "10";
	case device_t::DS1822:
		return "22";
	case device_t::DS18B20:
		return "28";
	case device_t::DS1825:
		return "3B";
	case device_t::DS28EA00:
		return "42";
	}
}