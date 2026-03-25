//
// Created by altay2510tr on 3/13/26.
//
#pragma once
#include "Globals.h"

class Mouse_move_event: public Event_management::Event
{
public:
    double mouse_x_offset, mouse_y_offset;
    Mouse_move_event(const double mouse_x_offset, const double mouse_y_offset)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Mouse_moved),
        mouse_x_offset(mouse_x_offset), mouse_y_offset(mouse_y_offset)
    {}

    void execute() override {}
};

class Mouse_button_press_event : public Event_management::Event
{
public:
    Input_key key;
    Mouse_button_press_event(const Input_key key)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Mouse_button_pressed), key(key)
    {
    }

    void execute() override {}
};

class Mouse_button_hold_event : public Event_management::Event
{
public:
    Input_key key;
    Mouse_button_hold_event(const Input_key key)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Mouse_button_hold), key(key)
    {
    }

    void execute() override {}
};

class Mouse_button_release_event : public Event_management::Event
{
public:
    Input_key key;
    Mouse_button_release_event(const Input_key key)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Mouse_button_released), key(key)
    {
    }

    void execute() override {}
};

class Mouse_scroll_event : public Event_management::Event
{
public:
    double x_offset, y_offset;
    Mouse_scroll_event(const double x_offset, const double y_offset)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Mouse_scrolled),
        x_offset(x_offset), y_offset(y_offset)
    {
    }
    void execute() override {}
};