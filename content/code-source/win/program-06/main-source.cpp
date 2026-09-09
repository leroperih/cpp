
// Data: d29/m03/y2026

#include <iostream>
#include <windows.h>
#include <thread>
#include <chrono>










int main()
{

	while (true)
	{

		POINT pt;
		GetCursorPos(&pt);

		std::cout << "x=" << std::showpos << std::internal << std::setw(5) << std::setfill('0') << pt.x;
		std::cout << std::endl;
		std::cout << "y=" << std::showpos << std::internal << std::setw(5) << std::setfill('0') << pt.y;

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		system("cls");

	}

	return 0;

}