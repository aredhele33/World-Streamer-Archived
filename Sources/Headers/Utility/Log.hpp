#pragma once

#include <iostream>

template<typename ...Args>
void LOG_DBG(Args&& ...args)
{
	std::cout << "[Open World MMFS][DBG] - ";
	(std::cout << ... << args);
	std::cout << std::endl;
}

template<typename ...Args>
void LOG_WRN(Args&& ...args)
{
	std::cout << "[Open World MMFS][WRN] - ";
	(std::cout << ... << args);
	std::cout << std::endl;
}

template<typename ...Args>
void LOG_ERR(Args&& ...args)
{
	std::cerr << "[Open World MMFS][ERR] - ";
	(std::cerr << ... << args);
	std::cerr << std::endl;
}
