#pragma once

#include "Globals.h"
#include "Window_manager.h"
#include "The_event_manager.h"
#include "Layer_manager.h"

class App
{
public:
    struct Config
    {
        Window_Manager::Config window_config;
        bool enable_vsync = false;
        double target_fps = 144.0;
    };

private:
    Config         m_config;
    Event_manager  m_event_manager;
    Window_Manager m_window;
    Layer_manager  m_layer_manager;

    bool   m_running = true;
    double m_last_frame_time = 0.0;

    Event_management::Event_receiver_shared m_close_receiver;

public:
    Event_manager& get_event_manager() { return m_event_manager; }
    Window_Manager& get_window() { return m_window; }
    Input_Manager* get_input_manager() { return m_window.get_input_manager(); }
    Layer_manager& get_layer_manager() { return m_layer_manager; }

    explicit App(const Config& cfg = Config())
        : m_config(cfg)
        , m_window(m_event_manager, cfg.window_config)
    {
        m_window.set_vsync(cfg.enable_vsync);

        m_close_receiver = Event_management::make_receiver([this](const Event_management::Event& e)
            {
                if (e.type == Event_management::Event_type::Window_closed)
                    m_running = false;
            });
        m_window.subscribe(Event_management::Event_type::Window_closed, m_close_receiver);

        glfwSetTime(0.0);
    }

    ~App()
    {
        m_layer_manager.clear_all_layers();
    }

    int push_layer(std::unique_ptr<Layer> layer, int index = -1)
    {
        return m_layer_manager.try_add_layer(std::move(layer), index);
    }

    void remove_layer(const std::string& name)
    {
        m_layer_manager.remove_layer(name);
    }

    Layer* get_layer(const std::string& name)
    {
        return m_layer_manager.get_layer(name);
    }

    void run()
    {
        m_last_frame_time = glfwGetTime();

        while (m_running)
        {
            m_layer_manager.update_all_layers();

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            m_layer_manager.render_all_layers();

            get_input_manager()->Poll_keys();
            m_window.Tick();
        }
    }
};