#pragma once

#include "Globals.h"

class Key_press_event: public Event_management::Event
{
    public:
    Input_key key;

    Key_press_event(const Input_key key_in)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Keyboard_button_pressed),
        key(key_in){}
};

class Key_hold_event : public Event_management::Event
{
public:
    Input_key key;

    Key_hold_event(const Input_key key_in)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Keyboard_button_hold),
        key(key_in) {}
};

class Key_release_event : public Event_management::Event
{
public:
    Input_key key;

    Key_release_event(const Input_key key_in)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Keyboard_button_released),
        key(key_in) {}
};