/*
 * name_table_viewer.h
 *
 *  Created on: Oct 21, 2014
 *      Author: Dale
 */

#ifndef SRC_FRONTEND_NAME_TABLE_VIEWER_H_
#define SRC_FRONTEND_NAME_TABLE_VIEWER_H_

#include <gtkmm/window.h>
#include <gtkmm/image.h>
#include <gtkmm/builder.h>
#include "utilities/emulator_worker.h"

class NameTableViewer : public Gtk::Window
{
	EmulatorWorker& worker;
	Glib::RefPtr<Gtk::Builder> builder; // Glade Builder
	unsigned char pixelArray0[184320];
	unsigned char pixelArray1[184320];
	unsigned char pixelArray2[184320];
	unsigned char pixelArray3[184320];
	unsigned char pixelArray4[49152];
	unsigned char pixelArray5[49152];
	unsigned char pixelArray6[3072];
	unsigned char pixelArray7[3072];
	unsigned char pixelArray8[3072];
	unsigned char pixelArray9[3072];
	unsigned char pixelArray10[3072];
	unsigned char pixelArray11[3072];
	unsigned char pixelArray12[3072];
	unsigned char pixelArray13[3072];
	Glib::RefPtr<Gdk::Pixbuf> buffer0;
	Glib::RefPtr<Gdk::Pixbuf> buffer1;
	Glib::RefPtr<Gdk::Pixbuf> buffer2;
	Glib::RefPtr<Gdk::Pixbuf> buffer3;
	Glib::RefPtr<Gdk::Pixbuf> buffer4;
	Glib::RefPtr<Gdk::Pixbuf> buffer5;
	Glib::RefPtr<Gdk::Pixbuf> buffer6;
	Glib::RefPtr<Gdk::Pixbuf> buffer7;
	Glib::RefPtr<Gdk::Pixbuf> buffer8;
	Glib::RefPtr<Gdk::Pixbuf> buffer9;
	Glib::RefPtr<Gdk::Pixbuf> buffer10;
	Glib::RefPtr<Gdk::Pixbuf> buffer11;
	Glib::RefPtr<Gdk::Pixbuf> buffer12;
	Glib::RefPtr<Gdk::Pixbuf> buffer13;

	void Update();

public:
	NameTableViewer(EmulatorWorker& worker);
};



#endif /* SRC_FRONTEND_NAME_TABLE_VIEWER_H_ */
