#pragma once

#include "The_event_manager.h"
#include "Input_Manager.h"
#include <vector>
#include "UI/Widget.h"
#include "UI/Button.h"
#include "UI/Text_panel.h"

class UI_manager
{
private:
    std::vector<Widget*> widgets;

    float mouse_ndc_x = 0.0f;
    float mouse_ndc_y = 0.0f;
    bool  mouse_clicked = false;

    Event_management::Event_receiver_shared mouse_move_receiver;
    Event_management::Event_receiver_shared mouse_click_receiver;

    unsigned int screen_width = 800;
    unsigned int screen_height = 600;

public:

    UI_manager() = default;

    void init(Input_Manager* input_manager, unsigned int width, unsigned int height)
    {
        screen_width = width;
        screen_height = height;

        mouse_move_receiver = Event_management::make_receiver([this](const Event_management::Event& e)
            {
                if (e.type == Event_management::Event_type::Mouse_moved)
                {
                    const auto& mouse = dynamic_cast<const Mouse_move_event&>(e);
                    mouse_ndc_x = (float)(mouse.mouse_x / screen_width) * 2.0f - 1.0f;
                    mouse_ndc_y = 1.0f - (float)(mouse.mouse_y / screen_height) * 2.0f;
                }
            });

        mouse_click_receiver = Event_management::make_receiver([this](const Event_management::Event& e)
            {
                if (e.type == Event_management::Event_type::Mouse_button_pressed)
                {
                    const auto& mouse = dynamic_cast<const Mouse_button_press_event&>(e);
                    if (mouse.key.code == GLFW_MOUSE_BUTTON_LEFT)
                        mouse_clicked = true;
                }
                else if (e.type == Event_management::Event_type::Mouse_button_released)
                {
                    const auto& mouse = dynamic_cast<const Mouse_button_release_event&>(e);
                    if (mouse.key.code == GLFW_MOUSE_BUTTON_LEFT)
                        mouse_clicked = false;
                }
            });

        input_manager->subscribe("Mouse_input", Event_management::Event_type::Mouse_moved, mouse_move_receiver);
        input_manager->subscribe("Mouse_input", Event_management::Event_type::Mouse_button_pressed, mouse_click_receiver);
        input_manager->subscribe("Mouse_input", Event_management::Event_type::Mouse_button_released, mouse_click_receiver);
    }

    void on_resize(unsigned int width, unsigned int height)
    {
        screen_width = width;
        screen_height = height;
    }

    void add_widget(Widget* widget)
    {
        widgets.push_back(widget);
    }

    void update()
    {
        for (Widget* widget : widgets)
        {
            if (widget && widget->visible)
                widget->update(mouse_ndc_x, mouse_ndc_y, mouse_clicked);
        }
    }

    void render()
    {
        for (Widget* widget : widgets)
        {
            if (widget && widget->visible)
                widget->render();
        }
    }

    bool is_hovered(float mx, float my) const
    {
        for (Widget* widget : widgets)
        {
            if (widget && widget->visible && widget->is_hovered(mx, my))
                return true;
        }
        return false;
    }

    bool is_hovered_ndc() const
    {
        for (Widget* widget : widgets)
        {
            if (widget && widget->visible && widget->is_hovered(mouse_ndc_x, mouse_ndc_y))
                return true;
        }
        return false;
    }
};