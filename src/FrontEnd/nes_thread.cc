#include "nes_thread.h"
#include "main_window.h"

wxDEFINE_EVENT(wxEVT_COMMAND_NESTHREAD_UPDATE, wxThreadEvent);
//wxDEFINE_EVENT(wxEVT_COMMAND_NESTHREAD_COMPLETED, wxThreadEvent);

wxThread::ExitCode NESThread::Entry()
{
	if (!TestDestroy())
	{
		nes.Start();
	}

	return static_cast<wxThread::ExitCode>(0);
}

NESThread::NESThread(MainWindow* handler, std::string& filename)
	: wxThread(wxTHREAD_JOINABLE),
	handler(handler),
	nes(*new NES(filename, *this)),
	nameTable0(0),
	nameTable1(0),
	nameTable2(0),
	nameTable3(0),
	patternTable0(0),
	patternTable1(0),
	palette0(0),
	palette1(0),
	palette2(0),
	palette3(0),
	palette4(0),
	palette5(0),
	palette6(0),
	palette7(0),
	width(256),
	height(240),
	pixelCount(0),
	pixelArray(new unsigned char[width*height*3])
{
}

NESThread::~NESThread()
{
	delete &nes;

	if (nameTable0) delete[] nameTable0;
	if (nameTable1) delete[] nameTable1;
	if (nameTable2) delete[] nameTable2;
	if (nameTable3) delete[] nameTable3;

	if (patternTable0) delete[] patternTable0;
	if (patternTable1) delete[] patternTable1;

	if (palette0) delete[] palette0;
	if (palette1) delete[] palette1;
	if (palette2) delete[] palette2;
	if (palette3) delete[] palette3;
	if (palette4) delete[] palette4;
	if (palette5) delete[] palette5;
	if (palette6) delete[] palette6;
	if (palette7) delete[] palette7;

	delete[] pixelArray;
}

void NESThread::EmulatorResume()
{
	if (nes.IsPaused()) nes.Resume();
}

void NESThread::EmulatorPause()
{
	if (!nes.IsPaused()) nes.Pause();
}

void NESThread::Stop()
{
	nes.Stop();
}

void NESThread::NextPixel(unsigned int pixel)
{
	while (frameLocked);

	unsigned char red = static_cast<unsigned char>((pixel & 0xFF0000) >> 16);
	unsigned char green = static_cast<unsigned char>((pixel & 0x00FF00) >> 8);
	unsigned char blue = static_cast<unsigned char>(pixel & 0x0000FF);

	pixelArray[pixelCount * 3] = red;
	pixelArray[(pixelCount * 3) + 1] = green;
	pixelArray[(pixelCount * 3) + 2] = blue;
	++pixelCount;

	if (pixelCount == width * height)
	{
		frameLocked = true;
		wxQueueEvent(handler, new wxThreadEvent(wxEVT_COMMAND_NESTHREAD_UPDATE));
		pixelCount = 0;
	}
}

unsigned char* NESThread::GetFrame()
{
	return pixelArray;
}

void NESThread::UnlockFrame()
{
	frameLocked = false;
}

unsigned char* NESThread::GetNameTable(int tableID)
{
	switch (tableID)
	{
	case 0:
		if (!nameTable0) nameTable0 = new unsigned char[184320];
		nes.GetNameTable(tableID, nameTable0);
		return nameTable0;
	case 1:
		if (!nameTable1) nameTable1 = new unsigned char[184320];
		nes.GetNameTable(tableID, nameTable1);
		return nameTable1;
	case 2:
		if (!nameTable2) nameTable2 = new unsigned char[184320];
		nes.GetNameTable(tableID, nameTable2);
		return nameTable2;
	default:
		if (!nameTable3) nameTable3 = new unsigned char[184320];
		nes.GetNameTable(tableID, nameTable3);
		return nameTable3;
	}
}

unsigned char* NESThread::GetPatternTable(int tableID, int paletteID)
{
	switch (tableID)
	{
	case 0:
		if (!patternTable0) patternTable0 = new unsigned char[49152];
		nes.GetPatternTable(tableID, paletteID, patternTable0);
		return patternTable0;
	default:
		if (!patternTable1) patternTable1 = new unsigned char[49152];
		nes.GetPatternTable(tableID, paletteID, patternTable1);
		return patternTable1;
	}
}

unsigned char* NESThread::GetPalette(int tableID)
{
	switch (tableID)
	{
	case 0:
		if (!palette0) palette0 = new unsigned char[3072];
		nes.GetPalette(tableID, palette0);
		return palette0;
	case 1:
		if (!palette1) palette1 = new unsigned char[3072];
		nes.GetPalette(tableID, palette1);
		return palette1;
	case 2:
		if (!palette2) palette2 = new unsigned char[3072];
		nes.GetPalette(tableID, palette2);
		return palette2;
	case 3:
		if (!palette3) palette3 = new unsigned char[3072];
		nes.GetPalette(tableID, palette3);
		return palette3;
	case 4:
		if (!palette4) palette4 = new unsigned char[3072];
		nes.GetPalette(tableID, palette4);
		return palette4;
	case 5:
		if (!palette5) palette5 = new unsigned char[3072];
		nes.GetPalette(tableID, palette5);
		return palette5;
	case 6:
		if (!palette6) palette6 = new unsigned char[3072];
		nes.GetPalette(tableID, palette6);
		return palette6;
	default:
		if (!palette7) palette7 = new unsigned char[3072];
		nes.GetPalette(tableID, palette7);
		return palette7;

	}
}