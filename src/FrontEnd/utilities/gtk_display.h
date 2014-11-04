/*
 * gtk_display.h
 *
 *  Created on: Oct 19, 2014
 *      Author: Dale
 */

#ifndef SRC_FRONTEND_UTILITIES_GTK_DISPLAY_H_
#define SRC_FRONTEND_UTILITIES_GTK_DISPLAY_H_

#include <gtkmm/image.h>
#include <glibmm/threads.h>
#include "../../Emulator/Interfaces/display.h"

class EmulatorWorker;

class GtkDisplay : public Display
{
	EmulatorWorker& worker;

	int width;
	int height;
	int pixelCount;
	guint8* pixelArray0;
	guint8* pixelArray1;

	Glib::Threads::Mutex frame0Mutex; // Frame Mutual Exclusion
	Glib::Threads::Mutex frame1Mutex; // Frame Mutual Exclusion

	Glib::RefPtr<Gdk::Pixbuf> buffer0;
	Glib::RefPtr<Gdk::Pixbuf> buffer1;
	bool even;

public:
	GtkDisplay(int width, int height, EmulatorWorker& worker);
	void NextPixel(unsigned int pixel);
	Glib::RefPtr<Gdk::Pixbuf>& GetFrame();
	~GtkDisplay();
};


#endif /* SRC_FRONTEND_UTILITIES_GTK_DISPLAY_H_ */
