/*
 * game_window.cc
 *
 *  Created on: Oct 19, 2014
 *      Author: Dale
 */

#include "game_window.h"

GameWindow::GameWindow(int width, int height)
	: buffer(Gdk::Pixbuf::create(Gdk::Colorspace::COLORSPACE_RGB, false, 8, 256, 240)),
	  frame(*new Gtk::Image(buffer)),
	  width(width),
	  height(height)
{
	set_default_size(width, height);
	set_resizable(false);
	set_title("Emulator");

	add(frame);
	show_all();
}

void GameWindow::UpdateFrame(Glib::RefPtr<Gdk::Pixbuf>& buffer)
{
	this->buffer = buffer;
	frame.set(this->buffer);
}

GameWindow::~GameWindow()
{
	delete &frame;
}


