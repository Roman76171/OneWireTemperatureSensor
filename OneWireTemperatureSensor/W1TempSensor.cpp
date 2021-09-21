#include <iostream>
#include <fstream>
#include <dirent.h>
#include <cstring>
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
//Return type: vector<string> 
W1TempSensor::deviceList_t W1TempSensor::getDevices()
{
	const std::string fileSlaveDevices{ "w1_master_slaves" };

	std::stringstream in{ readFromFile(BASE_FOLDER + '/' + MASTER_FOLDER, fileSlaveDevices) };
	deviceList_t deviceList{};
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
void W1TempSensor::setPullup(const PullupSetting pullupSetting)
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
		hasAllDeviceExternalPower() ? (out << 1) : (out << 0);
	}
	out.flush();
	writeToFile(BASE_FOLDER + '/' + MASTER_FOLDER, filePullup, out);
	PullupSetting pullupSettingNew{ getPullup() };
	if (pullupSetting != pullupSettingNew)
	{
		throw std::runtime_error("Failed to change pullup setting!");
	}
}

W1TempSensor::W1TempSensor() : m_device{}
{

}

W1TempSensor::W1TempSensor(const Device device) : m_device{ device }
{

}

void W1TempSensor::setDevice(const Device device)
{
	m_device = device;
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

//Writes data from a file: workFolder/fileName. Data is separated by '\n'.
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