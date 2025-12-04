#include "pch.h"

void Logging(std::string msg) {
	std::ofstream myfile;
	myfile.open("ModkitLimitAdjusterEnhanced.log", std::ios::app);
	myfile << msg << "\n";
	myfile.close();
}

void ClearLog()
{
	std::ofstream myfile;
	myfile.open("ModkitLimitAdjusterEnhanced.log");
	myfile << "";
	myfile.close();
}