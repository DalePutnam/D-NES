/*
 * main.cc
 *
 *  Created on: Mar 15, 2014
 *      Author: Dale
 */

#include <ctime>
#include <string>
#include <iostream>
#include <chrono>
#include <ratio>

#include "cpu.h"
#include "memory.h"
#include "cart_factory.h"

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
		CartFactory factory;

		string filename(argv[1]);
		Cart* cart = factory.getCartridge(filename);
		CPU* cpu = new CPU(cart); // Initialize CPU

		chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();

		cpu->Run(-1);

		chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
		chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);

		cout << "CPU ran for " << time_span.count() << "s" << endl;

		// Deallocate memory
		delete cpu;
		delete cart;
	}

	return 0;
}


