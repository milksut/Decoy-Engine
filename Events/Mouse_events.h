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
    Mouse_move_event(const double mouse_x_offset, const double mouse_y_offset, const double mouse_x, const double mouse_y)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Mouse_moved),
        mouse_x_offset(mouse_x_offset), mouse_y_offset(mouse_y_offset), mouse_x(mouse_x), mouse_y(mouse_y)
    {}

    void execute() override {}
};

class Mouse_button_press_event : public Event_management::Event
{
public:
    Input_key key;
    double mouse_x = 0, mouse_y = 0;
    Mouse_button_press_event(const Input_key key, const double mouse_x, const double mouse_y)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Mouse_button_pressed),
        key(key), mouse_x(mouse_x), mouse_y(mouse_y)
    {
    }

    void execute() override {}
};

class Mouse_button_hold_event : public Event_management::Event
{
public:
    Input_key key;
    double mouse_x = 0, mouse_y = 0;
    Mouse_button_hold_event(const Input_key key, const double mouse_x, const double mouse_y)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Mouse_button_hold), key(key),
        mouse_x(mouse_x), mouse_y(mouse_y)
    {
    }

    void execute() override {}
};

class Mouse_button_release_event : public Event_management::Event
{
public:
    Input_key key;
    double mouse_x = 0, mouse_y = 0;
    Mouse_button_release_event(const Input_key key, const double mouse_x, const double mouse_y)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Mouse_button_released), key(key),
		mouse_x(mouse_x), mouse_y(mouse_y)
    {
    }

    void execute() override {}
};

class Mouse_scroll_event : public Event_management::Event
{
public:
    double x_offset, y_offset;
    double mouse_x = 0, mouse_y = 0;
    Mouse_scroll_event(const double x_offset, const double y_offset, const double mouse_x, const double mouse_y)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Mouse_scrolled),
        x_offset(x_offset), y_offset(y_offset), mouse_x(mouse_x), mouse_y(mouse_y)
    {
    }
    void execute() override {}
};