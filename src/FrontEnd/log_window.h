/*
 * log_window.h
 *
 *  Created on: Sep 14, 2014
 *      Author: Dale
 */

#ifndef LOG_WINDOW_H_
#define LOG_WINDOW_H_

#include <gtkmm/window.h>
#include <gtkmm/builder.h>
#include <gtkmm/textbuffer.h>

class LogWindow : public Gtk::Window
{
	Glib::RefPtr<Gtk::TextBuffer::Tag> tag; // Font Size, etc
	Glib::RefPtr<Gtk::TextBuffer::TagTable> tagTable; // Contains all tags
	Glib::RefPtr<Gtk::TextBuffer> buffer; // Buffer containing text to display
	Gtk::TextBuffer::iterator iterator;

	Glib::RefPtr<Gtk::Builder> builder; // Glade Builder

	// Signal Handlers
	void onTextChanged(Gtk::Allocation& allocation);

public:
	LogWindow();
	void updateBuffer(std::string logChunk); // Add to buffer contents
};

#endif /* LOG_WINDOW_H_ */
