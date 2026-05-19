#pragma once

#include "Globals.h"

//TODO: add params

struct Window_resize_event : public Event_management::Event
{
	int new_width;
	int new_height;
	float new_aspect_ratio;
	Window_resize_event(int width, int height)
		: Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_resized),
		new_width(width), new_height(height), new_aspect_ratio(static_cast<float>(width) / static_cast<float>(height))
	{}
};	

struct Window_framebuffer_resize_event : public Event_management::Event
{
	int new_width;
	int new_height;
	float new_aspect_ratio;
	Window_framebuffer_resize_event(int width, int height)
		: Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_framebuffer_resized),
		new_width(width), new_height(height), new_aspect_ratio(static_cast<float>(width) / static_cast<float>(height))
	{}
};

struct Window_close_event : public Event_management::Event
{
    Window_close_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_closed)
    {}
};

struct Window_open_event : public Event_management::Event
{
    Window_open_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_opened)
    {}
};

struct Window_focus_event : public Event_management::Event
{
    Window_focus_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_focused)
    {}
};

struct Window_unfocus_event : public Event_management::Event
{
    Window_unfocus_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_unfocused)
    {}
};

struct Window_iconify_event : public Event_management::Event
{

    Window_iconify_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_iconified)
    {}
};

struct Window_uniconify_event : public Event_management::Event
{

    Window_uniconify_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_uniconified)
    {}
};

struct Window_maximize_event : public Event_management::Event
{

    Window_maximize_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_maximized)
    {}
};

struct Window_restore_event : public Event_management::Event
{

    Window_restore_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_restored)
    {}
};

struct Window_move_event : public Event_management::Event
{
    int x, y;

    Window_move_event(int x_in, int y_in)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_moved),
        x(x_in), y(y_in)
    {}
};

struct Window_content_scale_event : public Event_management::Event
{
    float x_scale, y_scale;

    Window_content_scale_event(float xs, float ys)
        : Event(Event_management::Event_timing::Queued, Event_management::Event_type::Window_content_scaled),
        x_scale(xs), y_scale(ys) 
    {}
};

struct Window_refresh_event : public Event_management::Event
{
    Window_refresh_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_refresh)
    {}
};