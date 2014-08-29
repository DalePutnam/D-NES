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

#include "nes.h"

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
		NES nes(filename);

		chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();

		nes.Start();

		chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
		chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);

		cout << "NES ran for " << time_span.count() << "s" << endl;

	}

	return 0;
}


