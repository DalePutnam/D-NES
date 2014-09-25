/*
 * logbuffer.h
 *
 *  Created on: Sep 14, 2014
 *      Author: Dale
 */

#ifndef LOGBUFFER_H_
#define LOGBUFFER_H_

#include <streambuf>
#include <vector>

class EmulatorWorker;

class LogBuffer : public std::streambuf
{
	EmulatorWorker& sink; // The sink for the stream
	std::vector<char> buffer; // Actual buffer

	// Called when buffer is full
	std::streambuf::int_type overflow(std::streambuf::int_type ch);
	int sync(); // Or whenever the buffer needs to be emptied

	bool writeToTextBuffer(); // Write out to the sink

	LogBuffer(const LogBuffer& log); // Prevent copying
	LogBuffer& operator=(const LogBuffer& log); // Prevent Assignment

public:
	explicit LogBuffer(EmulatorWorker& sink, std::size_t bufferSize = 256);

};



#endif /* LOGBUFFER_H_ */
