#pragma once

#include "Globals.h"
#include "Window_manager.h"
#include "The_event_manager.h"
#include "Layer.h"
#include "LayerStack.h"


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
    Config                      m_config;
    Event_manager               m_event_manager;
    Window_Manager              m_window;
    Decoy::LayerStack           m_layer_stack;

    bool                        m_running = true;
    double                      m_last_frame_time = 0.0;

    Event_management::Event_receiver_shared m_close_receiver;
    Event_management::Event_receiver_shared m_framebuffer_receiver;

public:
    Event_manager& get_event_manager() { return m_event_manager; }
    Window_Manager& get_window() { return m_window; }
    Input_Manager* get_input_manager() { return m_window.get_input_manager(); }

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

    }

    void push_layer(Decoy::Layer* layer)
    {
        m_layer_stack.pushLayer(layer);
    }

    void push_overlay(Decoy::Layer* overlay)
    {
        m_layer_stack.pushOverlay(overlay);
    }

    void run()
    {
        m_last_frame_time = glfwGetTime();

        while (m_running)
        {
            double now = glfwGetTime();
            float  dt = (float)(now - m_last_frame_time);
            m_last_frame_time = now;

            for (Decoy::Layer* layer : m_layer_stack)
                layer->onUpdate(dt);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            for (Decoy::Layer* layer : m_layer_stack)
                layer->onRender();

            get_input_manager()->Poll_keys();
            m_window.Tick(); // swap buffers + glfwPollEvents
        }
    }
};