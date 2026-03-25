//
// Created by altay2510tr on 3/13/26.
//
#pragma once

#include "Globals.h"

class Key_press_event: public Event_management::Event
{
    public:
    Input_key key;

    Key_press_event(const Input_key key)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Keyboard_button_pressed), key(key)
    {
    }

    void execute() override {}
};

class Key_hold_event : public Event_management::Event
{
public:
    Input_key key;

    Key_hold_event(const Input_key key)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Keyboard_button_hold), key(key)
    {
    }

    void execute() override {}
};

class Key_release_event : public Event_management::Event
{
public:
    Input_key key;

    Key_release_event(const Input_key key)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Keyboard_button_released), key(key)
    {
    }

    void execute() override {}
};