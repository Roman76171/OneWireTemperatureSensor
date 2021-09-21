#include <iostream>
#include <fstream>
#include <dirent.h>
#include "W1TempSensor.h"

int main(void)
{
	InitLib();
	W1TempSensor::setPullup(W1TempSensor::PullupSetting::DISABLE);
	std::cout << std::string("r");
	/*std::ifstream fin{"r1.txt"};
	while (fin)
	{
		std::string str1{};
		std::string str2{};
		std::getline(fin, str1, '-');
		std::getline(fin, str2);
		std::cout << str1 << str2 << std::endl;
	}
	fin.close();*/
	/*const std::string basePath{ "/sys/bus/w1/devices" };

	std::ifstream fin{ basePath + '/' + "w1_bus_master1" + '/' + "w1_master_slave_count"};
	if (!fin.is_open())
	{
		std::cerr << "Can't open file!" << std::endl;
	}
	std::string str{};
	std::getline(fin, str);
	fin.close();
	std::cout << "File string: " << str << std::endl;*/
	getchar();
	return 0;
}