//
// Created by altay2510tr on 3/13/26.
//
#pragma once
#include "Globals.h"

class Mouse_move_event: public Event_management::Event
{
public:
    double mouse_x_offset, mouse_y_offset;
    double mouse_x = 0, mouse_y = 0;
    Mouse_move_event(const double mouse_x_offset_in, const double mouse_y_offset_in,
        const double mouse_x_in, const double mouse_y_in)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Mouse_moved),
        mouse_x_offset(mouse_x_offset_in), mouse_y_offset(mouse_y_offset_in), mouse_x(mouse_x_in), mouse_y(mouse_y_in)
    {}
};

class Mouse_button_press_event : public Event_management::Event
{
public:
    Input_key key;
    double mouse_x = 0, mouse_y = 0;
    Mouse_button_press_event(const Input_key key_in, const double mouse_x_in, const double mouse_y_in)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Mouse_button_pressed),
        key(key_in), mouse_x(mouse_x_in), mouse_y(mouse_y_in)
    {}
};

class Mouse_button_hold_event : public Event_management::Event
{
public:
    Input_key key;
    double mouse_x = 0, mouse_y = 0;
    Mouse_button_hold_event(const Input_key key_in, const double mouse_x_in, const double mouse_y_in)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Mouse_button_hold), key(key_in),
        mouse_x(mouse_x_in), mouse_y(mouse_y_in)
    {}
};

class Mouse_button_release_event : public Event_management::Event
{
public:
    Input_key key;
    double mouse_x = 0, mouse_y = 0;
    Mouse_button_release_event(const Input_key key_in, const double mouse_x_in, const double mouse_y_in)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Mouse_button_released), key(key_in),
		mouse_x(mouse_x_in), mouse_y(mouse_y_in)
    {}
};

class Mouse_scroll_event : public Event_management::Event
{
public:
    double x_offset, y_offset;
    double mouse_x = 0, mouse_y = 0;
    Mouse_scroll_event(const double x_offset_in, const double y_offset_in, const double mouse_x_in, const double mouse_y_in)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Mouse_scrolled),
        x_offset(x_offset_in), y_offset(y_offset_in), mouse_x(mouse_x_in), mouse_y(mouse_y_in)
    {}
};