/*
 * main.cc
 *
 *  Created on: Mar 15, 2014
 *      Author: Dale
 */

#include <ctime>
#include <string>
#include <iostream>

#include "cpu.h"
#include "memory.h"

using namespace std;

int main(int argc, char* argv[])
{
	// If no ROM file specified then exit
	if (argc < 2)
	{
		cout << "No ROM specified" << endl;
	}
	else
	{
		string filename(argv[1]);
		Cart* cart = new Cart(filename);
		//RAM* ram = new RAM(filename); // Initialize RAM (a certain section of the main memory is devoted to the .nes file)
		CPU* cpu = new CPU(cart); // Initialize CPU

		clock_t t1, t2, diff;
		t1 = clock(); // Record starting time

		cpu->Run(-1);

		t2 = clock(); // Recored ending time
		diff = t2 - t1; // Recored running time

		// Print running time in milliseconds
		float milliseconds = diff / (CLOCKS_PER_SEC / 1000);

		cout << "CPU ran for " << milliseconds << "ms" << endl;

		// Deallocate memory
		delete cpu;
		delete cart;
	}

	return 0;
}


