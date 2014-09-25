/*
 * log_window.cc
 *
 *  Created on: Sep 14, 2014
 *      Author: Dale
 */

#include <gtkmm/textview.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/box.h>

#include "log_window.h"

void LogWindow::onTextChanged(Gtk::Allocation& allocation)
{
	// Set scroll position to bottom of text
	Gtk::ScrolledWindow* scrolled = 0;
	builder->get_widget("logScrolledWindow", scrolled);
	Glib::RefPtr<Gtk::Adjustment> adjustment = scrolled->get_vadjustment();
	adjustment->set_value(adjustment->get_upper() - adjustment->get_page_size());
}

LogWindow::LogWindow() :
		tag(Gtk::TextBuffer::Tag::create()),
		tagTable(Gtk::TextBuffer::TagTable::create()),
		buffer(Gtk::TextBuffer::create(tagTable)),
		iterator(buffer->begin()),
#ifdef DEBUG
		builder(Gtk::Builder::create_from_file("D:/Source/D-NES/src/FrontEnd/glade/LogWindow.glade")) // Get glade from file
#else
		builder(Gtk::Builder::create_from_resource("/glade/LogWindow.glade")) // Get glade from resource
#endif
{
	set_default_size(836, 600);
	set_title("Log");

	// Set font and size
	tag->property_font() = "Courier 12";
	buffer->get_tag_table()->add(tag);

	// add buffer to text view
	Gtk::TextView* view = 0;
	builder->get_widget("logTextView", view);
	view->set_buffer(buffer);

	// Connect all signals
	view->signal_size_allocate().connect(sigc::mem_fun(*this, &LogWindow::onTextChanged));

	// Add glade contents to window
	Gtk::Box* vbox = 0;
	builder->get_widget("logBox", vbox);
	add(*vbox);

	vbox->show_all(); // show window
}

void LogWindow::updateBuffer(std::string logChunk)
{
	Glib::RefPtr<Gtk::TextBuffer::Mark> mark = buffer->create_mark(iterator); // Mark place in buffer
	iterator = buffer->insert(iterator, logChunk); // Insert new text
	buffer->apply_tag(tag, mark->get_iter(), iterator); // Apply font to new text
}


