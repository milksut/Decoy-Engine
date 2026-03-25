//
// Created by altay2510tr on 3/25/26.
//
#pragma once

#include "Globals.h"

class Combo_button_press_event : public Event_management::Event
{
public:
    Key_combo combo;

    Combo_button_press_event(const Key_combo combo)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Combo_button_pressed), combo(combo)
    {
    }

    void execute() override {}
};

class Combo_button_hold_event : public Event_management::Event
{
public:
    Key_combo combo;

    Combo_button_hold_event(const Key_combo combo)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Combo_button_hold), combo(combo)
    {
    }

    void execute() override {}
};

class Combo_button_release_event : public Event_management::Event
{
public:
    Key_combo combo;

    Combo_button_release_event(const Key_combo combo)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Combo_button_released), combo(combo)
    {
    }

    void execute() override {}
};