/*
 * log_buffer.cc
 *
 *  Created on: Sep 14, 2014
 *      Author: Dale
 */


#include "log_buffer.h"
#include "emulator_worker.h"

std::streambuf::int_type LogBuffer::overflow(std::streambuf::int_type ch)
{
	if (ch != traits_type::eof())
	{
		*std::streambuf::pptr() = ch; // add new char to last spot in buffer
		std::streambuf::pbump(1);

		// flush buffer
		if (writeToTextBuffer())
		{
			return ch;
		}
	}

	return traits_type::eof();
}

int LogBuffer::sync()
{
	return writeToTextBuffer() ? 0 : -1; // Flush buffer
}

bool LogBuffer::writeToTextBuffer()
{
	sink.writeOutLog(pbase(), pptr()); // Write to sink

	// Update buffer pointers
	std::ptrdiff_t n = pptr() - pbase();
	pbump(-n);

	return true;
}

LogBuffer::LogBuffer(EmulatorWorker& sink, std::size_t bufferSize)
	: sink(sink),
	  buffer(bufferSize + 1)
{
	// Initialize buffer pointers
	char* base = &buffer.front();
	std::streambuf::setp(base, base + buffer.size() - 1);
}
