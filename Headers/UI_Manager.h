#pragma once

#include "The_event_manager.h"
#include "Input_Manager.h"
#include <vector>
#include "UI/Widget.h"
#include "UI/Button.h"
#include "UI/Text_panel.h"

class UI_manager
{
public:
    struct Config
    {
        std::string channel_name = "UI_input";
        unsigned int screen_width = 1280;
        unsigned int screen_height = 720;
    };

private:
    Config config;

    std::vector<Widget*> widgets;

    float mouse_ndc_x = 0.0f;
    float mouse_ndc_y = 0.0f;
    bool  mouse_clicked = false;

    Event_management::Event_receiver_shared mouse_move_receiver;
    Event_management::Event_receiver_shared mouse_click_receiver;
    Event_management::Event_receiver_shared mouse_release_receiver;

    Input_Manager* input_manager = nullptr;

    /// <summary>
    ///     Checks whether any visible widget is currently hovered by the mouse.
    /// </summary>
    /// <returns>True if at least one widget is hovered.</returns>
    bool any_widget_hovered() const
    {
        for (Widget* w : widgets)
            if (w && w->visible && w->is_hovered(mouse_ndc_x, mouse_ndc_y))
                return true;
        return false;
    }

public:

    UI_manager() = default;

    /// <summary>
    ///     Initializes the UI manager and registers mouse input event listeners.
    /// </summary>
    /// <remarks>
    ///     Creates the configured input channel, tracks mouse movement/click state,
    ///     and subscribes to mouse-related input events.
    /// </remarks>
    /// <param name="input_mgr">[in] Input manager used for event subscription.</param>
    /// <param name="config_in">[in] UI manager configuration settings.</param>
    void init(Input_Manager* input_mgr, const Config& config_in = Config())
    {
        input_manager = input_mgr;
        config = config_in;

        input_manager->create_channel(config.channel_name);

        mouse_move_receiver = Event_management::make_receiver([this](const Event_management::Event& e)
            {
                if (e.type != Event_management::Event_type::Mouse_moved) return;
                const auto& mouse = dynamic_cast<const Mouse_move_event&>(e);
                mouse_ndc_x = (float)(mouse.mouse_x / (double)config.screen_width) * 2.0f - 1.0f;
                mouse_ndc_y = 1.0f - (float)(mouse.mouse_y / (double)config.screen_height) * 2.0f;
            });

        mouse_click_receiver = Event_management::make_receiver([this](const Event_management::Event& e)
            {
                if (e.type != Event_management::Event_type::Mouse_button_pressed) return;
                const auto& mouse = dynamic_cast<const Mouse_button_press_event&>(e);
                if (mouse.key.code != GLFW_MOUSE_BUTTON_LEFT) return;

                mouse_clicked = true;
            });

        mouse_release_receiver = Event_management::make_receiver([this](const Event_management::Event& e)
            {
                if (e.type != Event_management::Event_type::Mouse_button_released) return;
                const auto& mouse = dynamic_cast<const Mouse_button_release_event&>(e);
                if (mouse.key.code == GLFW_MOUSE_BUTTON_LEFT)
                    mouse_clicked = false;
            });

        input_manager->subscribe("Mouse_input", Event_management::Event_type::Mouse_moved, mouse_move_receiver);
        input_manager->subscribe("Mouse_input", Event_management::Event_type::Mouse_button_pressed, mouse_click_receiver);
        input_manager->subscribe("Mouse_input", Event_management::Event_type::Mouse_button_released, mouse_release_receiver);
    }

    /// <summary>
    ///     Updates stored screen dimensions after a window resize.
    /// </summary>
    /// <param name="width">[in] New screen width.</param>
    /// <param name="height">[in] New screen height.</param>
    void on_resize(unsigned int width, unsigned int height)
    {
        config.screen_width = width;
        config.screen_height = height;
    }

    /// <summary>
    ///     Subscribes a receiver to an event type on the UI manager channel.
    /// </summary>
    /// <param name="event_type">[in] Event type to subscribe to.</param>
    /// <param name="receiver">[in] Shared receiver callback object.</param>
    void subscribe(const Event_management::Event_type event_type,
        const Event_management::Event_receiver_shared& receiver)
    {
        input_manager->subscribe(config.channel_name, event_type, receiver);
    }

    /// <summary>
    ///     Adds a widget to the UI container.
    /// </summary>
    /// <param name="widget">[in] Pointer to the widget to add.</param>
    void add_widget(Widget* widget)
    {
        widgets.push_back(widget);
    }

    /// <summary>
    ///     Updates all visible UI widgets using current mouse state.
    /// </summary>
    /// <remarks>
    ///     Forwards normalized mouse coordinates and click state to each widget.
    /// </remarks>
    void update()
    {
        for (Widget* widget : widgets)
            if (widget && widget->visible)
                widget->update(mouse_ndc_x, mouse_ndc_y, mouse_clicked);
    }

    /// <summary>
    ///     Renders all visible UI widgets.
    /// </summary>
    /// <remarks>
    ///     Iterates through widget list and calls render on each active widget.
    /// </remarks>
    void render()
    {
        for (Widget* widget : widgets)
            if (widget && widget->visible)
                widget->render();
    }

    /// <summary>
    ///     Checks whether any UI widget is hovered in NDC space.
    /// </summary>
    /// <returns>True if any widget is currently hovered.</returns>
    bool is_hovered_ndc() const
    {
        return any_widget_hovered();
    }

    /// <summary>
    ///     Checks whether any visible widget is hovered at the given mouse position.
    /// </summary>
    /// <param name="mx">[in] Mouse X position.</param>
    /// <param name="my">[in] Mouse Y position.</param>
    /// <returns>True if any widget is hovered.</returns>
    bool is_hovered(float mx, float my) const
    {
        for (Widget* w : widgets)
            if (w && w->visible && w->is_hovered(mx, my))
                return true;
        return false;
    }
};