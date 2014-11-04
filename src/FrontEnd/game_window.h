/*
 * game_window.h
 *
 *  Created on: Oct 19, 2014
 *      Author: Dale
 */

#ifndef SRC_FRONTEND_GAME_WINDOW_H_
#define SRC_FRONTEND_GAME_WINDOW_H_

#include <gtkmm/window.h>
#include <gtkmm/image.h>

class GameWindow : public Gtk::Window
{
	Glib::RefPtr<Gdk::Pixbuf> buffer;
	Gtk::Image& frame;
	int width;
	int height;

public:
	GameWindow(int width, int height);
	void UpdateFrame(Glib::RefPtr<Gdk::Pixbuf>& buffer);

	~GameWindow();
};

#endif /* SRC_FRONTEND_GAME_WINDOW_H_ */
