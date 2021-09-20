#include <iostream>
#include <fstream>

int main(void)
{
	const std::string basePath{ "/sys/bus/w1/devices" };

	std::ifstream fin{ basePath + '/' + "w1_bus_master1" + '/' + "w1_master_slave_count"};
	if (!fin.is_open())
	{
		std::cerr << "Can't open file!" << std::endl;
	}
	std::string str{};
	std::getline(fin, str);
	fin.close();
	std::cout << "File string: " << str << std::endl;
	getchar();
	return 0;
}