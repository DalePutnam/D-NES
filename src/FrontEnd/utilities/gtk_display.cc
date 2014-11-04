/*
 * gtk_display.cc
 *
 *  Created on: Oct 19, 2014
 *      Author: Dale
 */

#ifndef SRC_FRONTEND_UTILITIES_GTK_DISPLAY_CC_
#define SRC_FRONTEND_UTILITIES_GTK_DISPLAY_CC_

#include "gtk_display.h"
#include "emulator_worker.h"

GtkDisplay::GtkDisplay(int width, int height, EmulatorWorker& worker)
	: worker(worker),
	  width(width),
	  height(height),
	  pixelCount(0),
	  pixelArray0(new guint8[width*height*3]),
	  pixelArray1(new guint8[width*height*3]),
	  even(true)
{}

void GtkDisplay::NextPixel(unsigned int pixel)
{
	guint8 red = static_cast<guint8>((pixel & 0xFF0000) >> 16);
	guint8 green = static_cast<guint8>((pixel & 0x00FF00) >> 8);
	guint8 blue = static_cast<guint8>((pixel & 0x0000FF));

	if (even)
	{
		pixelArray0[(pixelCount*3)] = red;
		pixelArray0[(pixelCount*3) + 1] = green;
		pixelArray0[(pixelCount*3) + 2] = blue;
	}
	else
	{
		pixelArray1[(pixelCount*3)] = red;
		pixelArray1[(pixelCount*3) + 1] = green;
		pixelArray1[(pixelCount*3) + 2] = blue;
	}
	++pixelCount;

	if (pixelCount == width * height)
	{
		if (even)
		{
			Glib::Threads::Mutex::Lock lock(frame0Mutex);
			buffer0 = Gdk::Pixbuf::create_from_data(pixelArray0, Gdk::Colorspace::COLORSPACE_RGB, false, 8, width, height, width*3);
		}
		else
		{
			Glib::Threads::Mutex::Lock lock(frame1Mutex);
			buffer1 = Gdk::Pixbuf::create_from_data(pixelArray1, Gdk::Colorspace::COLORSPACE_RGB, false, 8, width, height, width*3);
		}
		pixelCount = 0;

		worker.updateFrame();
	}
}

Glib::RefPtr<Gdk::Pixbuf>& GtkDisplay::GetFrame()
{
	if (even)
	{
		Glib::Threads::Mutex::Lock lock(frame0Mutex);
		even = !even;
		return buffer0;
	}
	else
	{
		Glib::Threads::Mutex::Lock lock(frame1Mutex);
		even = !even;
		return buffer1;
	}
}

GtkDisplay::~GtkDisplay()
{
	delete [] pixelArray0;
	delete [] pixelArray1;
}


#endif /* SRC_FRONTEND_UTILITIES_GTK_DISPLAY_CC_ */
