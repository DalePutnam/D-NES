/*
 * name_table_viewer.cc
 *
 *  Created on: Oct 21, 2014
 *      Author: Dale
 */

#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include "name_table_viewer.h"

void NameTableViewer::Update()
{
	worker.pauseWorker();

	worker.getNameTable(0, pixelArray0);
	buffer0 = Gdk::Pixbuf::create_from_data(pixelArray0, Gdk::Colorspace::COLORSPACE_RGB, false, 8, 256, 240, 256*3);
	worker.getNameTable(1, pixelArray1);
	buffer1 = Gdk::Pixbuf::create_from_data(pixelArray1, Gdk::Colorspace::COLORSPACE_RGB, false, 8, 256, 240, 256*3);
	worker.getNameTable(2, pixelArray2);
	buffer2 = Gdk::Pixbuf::create_from_data(pixelArray2, Gdk::Colorspace::COLORSPACE_RGB, false, 8, 256, 240, 256*3);
	worker.getNameTable(3, pixelArray3);
	buffer3 = Gdk::Pixbuf::create_from_data(pixelArray3, Gdk::Colorspace::COLORSPACE_RGB, false, 8, 256, 240, 256*3);
	worker.getPatternTable(0, pixelArray4);
	buffer4 = Gdk::Pixbuf::create_from_data(pixelArray4, Gdk::Colorspace::COLORSPACE_RGB, false, 8, 128, 128, 128*3);
	worker.getPatternTable(1, pixelArray5);
	buffer5 = Gdk::Pixbuf::create_from_data(pixelArray5, Gdk::Colorspace::COLORSPACE_RGB, false, 8, 128, 128, 128*3);
	worker.getPalette(0, pixelArray6);
	buffer6 = Gdk::Pixbuf::create_from_data(pixelArray6, Gdk::Colorspace::COLORSPACE_RGB, false, 8, 64, 16, 64*3);
	worker.getPalette(1, pixelArray7);
	buffer7 = Gdk::Pixbuf::create_from_data(pixelArray7, Gdk::Colorspace::COLORSPACE_RGB, false, 8, 64, 16, 64*3);
	worker.getPalette(2, pixelArray8);
	buffer8 = Gdk::Pixbuf::create_from_data(pixelArray8, Gdk::Colorspace::COLORSPACE_RGB, false, 8, 64, 16, 64*3);
	worker.getPalette(3, pixelArray9);
	buffer9 = Gdk::Pixbuf::create_from_data(pixelArray9, Gdk::Colorspace::COLORSPACE_RGB, false, 8, 64, 16, 64*3);
	worker.getPalette(4, pixelArray10);
	buffer10 = Gdk::Pixbuf::create_from_data(pixelArray10, Gdk::Colorspace::COLORSPACE_RGB, false, 8, 64, 16, 64*3);
	worker.getPalette(5, pixelArray11);
	buffer11 = Gdk::Pixbuf::create_from_data(pixelArray11, Gdk::Colorspace::COLORSPACE_RGB, false, 8, 64, 16, 64*3);
	worker.getPalette(6, pixelArray12);
	buffer12 = Gdk::Pixbuf::create_from_data(pixelArray12, Gdk::Colorspace::COLORSPACE_RGB, false, 8, 64, 16, 64*3);
	worker.getPalette(7, pixelArray13);
	buffer13 = Gdk::Pixbuf::create_from_data(pixelArray13, Gdk::Colorspace::COLORSPACE_RGB, false, 8, 64, 16, 64*3);

	Gtk::Image* image = 0;
	builder->get_widget("NameTable0", image);
	image->set(buffer0);
	builder->get_widget("NameTable1", image);
	image->set(buffer1);
	builder->get_widget("NameTable2", image);
	image->set(buffer2);
	builder->get_widget("NameTable3", image);
	image->set(buffer3);
	builder->get_widget("patternTable0", image);
	image->set(buffer4);
	builder->get_widget("patternTable1", image);
	image->set(buffer5);
	builder->get_widget("bgPalette0", image);
	image->set(buffer6);
	builder->get_widget("bgPalette1", image);
	image->set(buffer7);
	builder->get_widget("bgPalette2", image);
	image->set(buffer8);
	builder->get_widget("bgPalette3", image);
	image->set(buffer9);
	builder->get_widget("spPalette0", image);
	image->set(buffer10);
	builder->get_widget("spPalette1", image);
	image->set(buffer11);
	builder->get_widget("spPalette2", image);
	image->set(buffer12);
	builder->get_widget("spPalette3", image);
	image->set(buffer13);

	worker.resumeWorker();
}

NameTableViewer::NameTableViewer(EmulatorWorker& worker)
	: worker(worker),
	  builder(Gtk::Builder::create_from_file("D:/Source/D-NES/src/FrontEnd/glade/NameTableViewer.glade"))
{
	set_title("NameTableViewer");
	set_resizable(false);

	Gtk::Button* button = 0;
	builder->get_widget("updateButton", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &NameTableViewer::Update));

	Gtk::Box* vbox = 0;
	builder->get_widget("mainBox", vbox);
	add(*vbox);

	Update();

	vbox->show_all();
}

