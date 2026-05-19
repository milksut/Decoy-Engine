#pragma once

#include "Globals.h"
#include "Events/Window_events.h"
#include "Input_Manager.h"

class Window_Manager
{
public:
    //Config------------------------------------------------------------------
    struct Config
    {
        int         width = 1280;
        int         height = 720;
        int         pos_x = 0;
		int         pos_y = 0;
        float       aspect_ratio = (float)width / (float)height;
        std::string title = "Application";
        std::string channel_name = "Window_main";
        bool        resizable = true;
        bool        decorated = true;           // title-bar + borders
        bool        maximized = false;
        GLFWwindow* share_context = nullptr;       // pass another window's handle to share GL context
    };
    //-----------------------------------------------------------------------
private:
    GLFWwindow* window = nullptr;
    std::optional<Input_Manager> input_manager;
    static inline int instance_count = 0;

    //probably no need to be private, but changing things on this will not effect the window, so you should use functions instead
	Config config = Config();

    Window_Manager(const Window_Manager&) = delete;// Delete copy constructor

    Window_Manager& operator=(const Window_Manager&) = delete;// Delete copy assignment operator

	//Callbacks-----------------------------------------------------------------
    void framebuffer_resize_callback(GLFWwindow* /*window*/, int width, int height)
    {
        glViewport(0, 0, width, height);

        input_manager->throw_event(config.channel_name, std::make_unique<Window_framebuffer_resize_event>(width, height));
    }
    void window_resize_callback(GLFWwindow* /*window*/, int width, int height)
    {
		config.width = width;
		config.height = height;
		config.aspect_ratio = static_cast<float>(width) / static_cast<float>(height);

        input_manager->throw_event(config.channel_name, std::make_unique<Window_resize_event>(width, height));
	}
    void mouse_move_callback(GLFWwindow* /*window*/, double x_pos, double y_pos)
    {
		double &mouse_x_ref = input_manager->mouse_x, &mouse_y_ref = input_manager->mouse_y;
        const double mouse_x_offset = (x_pos - mouse_x_ref) * input_manager->mouse_sensitivity;
        const double mouse_y_offset = (y_pos - mouse_y_ref) * input_manager->mouse_sensitivity;

        mouse_x_ref = x_pos;
        mouse_y_ref = y_pos;

        input_manager->throw_event(Input_channel_names[Mouse_input], std::make_unique<Mouse_move_event>(
            mouse_x_offset, mouse_y_offset, mouse_x_ref, mouse_y_ref));
    }
    void mouse_scroll_callback(GLFWwindow* /*window*/, double x_offset, double y_offset)
    {
        input_manager->throw_event(Input_channel_names[Mouse_input],
            std::make_unique<Mouse_scroll_event>(x_offset, y_offset, input_manager->mouse_x, input_manager->mouse_y));
    }
    void window_close_callback(GLFWwindow* /*window*/)
    {
		input_manager->throw_event(config.channel_name, std::make_unique<Window_close_event>());
	}
    void window_focus_callback(GLFWwindow* /*window*/, int focused)
    {
        if (focused)
            input_manager->throw_event(config.channel_name, std::make_unique<Window_focus_event>());
        else
            input_manager->throw_event(config.channel_name, std::make_unique<Window_unfocus_event>());
	}
    void window_iconify_callback(GLFWwindow* /*window*/, int iconified)
    {
        if (iconified)
            input_manager->throw_event(config.channel_name, std::make_unique<Window_iconify_event>());
        else
            input_manager->throw_event(config.channel_name, std::make_unique<Window_uniconify_event>());
    }
    void window_maximize_callback(GLFWwindow* /*window*/, int maximized)
    {
        if (maximized)
            input_manager->throw_event(config.channel_name, std::make_unique<Window_maximize_event>());
        else
            input_manager->throw_event(config.channel_name, std::make_unique<Window_restore_event>());
	}
    void window_move_callback(GLFWwindow* /*window*/, int x_pos, int y_pos)
    {
        config.pos_x = x_pos;
        config.pos_y = y_pos;
        input_manager->throw_event(config.channel_name, std::make_unique<Window_move_event>(x_pos, y_pos));
    }
    void window_content_scale_callback(GLFWwindow* /*window*/, float x_scale, float y_scale)
    {
        input_manager->throw_event(config.channel_name, std::make_unique<Window_content_scale_event>(x_scale, y_scale));
	}
    void window_refresh_callback(GLFWwindow* /*window*/)
    {
        input_manager->throw_event(config.channel_name, std::make_unique<Window_refresh_event>());
	}
    //--------------------------------------------------------------------------

    void create_window(Event_manager& event_manager)
    {
        if (window)
        {
            LOG_WARNING("Window_Manager - Window already exists, skipping creation.");
			return;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_VERSION_MAJOR);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_VERSION_MINOR);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, config.decorated ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_MAXIMIZED, config.maximized ? GLFW_TRUE : GLFW_FALSE);

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        window = glfwCreateWindow(config.width, config.height, config.title.c_str(), nullptr, config.share_context);
        if (!window)
        {
            LOG_FATAL("Window_Manager - Failed to create GLFW window \"%s\"!", config.title.c_str());
            throw std::runtime_error("Window_Manager - glfwCreateWindow() failed");
        }

        glfwMakeContextCurrent(window);

        // Only load GL on the first window — subsequent windows share the same context
        if (instance_count == 1)
        {
            if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
            {
                LOG_FATAL("Window_Manager - Failed to initialise GLAD!");
                throw std::runtime_error("Window_Manager - gladLoadGLLoader() failed");
            }
        }

        glfwSetWindowUserPointer(window, this);
		input_manager.emplace(event_manager, window);
        input_manager->create_channel(config.channel_name);
        get_mouse_position(input_manager->mouse_x, input_manager->mouse_y);

		//Register callbacks----------------------------------------------------------------
        glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int width, int height)
            {
                auto self = static_cast<Window_Manager*>(glfwGetWindowUserPointer(w));
                self->framebuffer_resize_callback(w, width, height);
            });

        glfwSetWindowSizeCallback(window, [](GLFWwindow* w, int width, int height)
            {
                auto self = static_cast<Window_Manager*>(glfwGetWindowUserPointer(w));
                self->window_resize_callback(w, width, height);
            });

        glfwSetCursorPosCallback(window, [](GLFWwindow* w, double x, double y)
            {
                auto self = static_cast<Window_Manager*>(glfwGetWindowUserPointer(w));
                self->mouse_move_callback(w, x, y);
            });

        glfwSetScrollCallback(window, [](GLFWwindow* w, double xoff, double yoff)
            {
                auto self = static_cast<Window_Manager*>(glfwGetWindowUserPointer(w));
                self->mouse_scroll_callback(w, xoff, yoff);
            });

        glfwSetWindowCloseCallback(window, [](GLFWwindow* w)
            {
                auto self = static_cast<Window_Manager*>(glfwGetWindowUserPointer(w));
                self->window_close_callback(w);
            });

        glfwSetWindowFocusCallback(window, [](GLFWwindow* w, int focused)
            {
                auto self = static_cast<Window_Manager*>(glfwGetWindowUserPointer(w));
                self->window_focus_callback(w, focused);
            });

        glfwSetWindowIconifyCallback(window, [](GLFWwindow* w, int iconified)
            {
                auto self = static_cast<Window_Manager*>(glfwGetWindowUserPointer(w));
                self->window_iconify_callback(w, iconified);
            });

        glfwSetWindowMaximizeCallback(window, [](GLFWwindow* w, int maximized)
            {
                auto self = static_cast<Window_Manager*>(glfwGetWindowUserPointer(w));
                self->window_maximize_callback(w, maximized);
            });

        glfwSetWindowPosCallback(window, [](GLFWwindow* w, int x, int y)
            {
                auto self = static_cast<Window_Manager*>(glfwGetWindowUserPointer(w));
                self->window_move_callback(w, x, y);
            });

        glfwSetWindowContentScaleCallback(window, [](GLFWwindow* w, float xs, float ys)
            {
                auto self = static_cast<Window_Manager*>(glfwGetWindowUserPointer(w));
                self->window_content_scale_callback(w, xs, ys);
            });

        glfwSetWindowRefreshCallback(window, [](GLFWwindow* w)
            {
                auto self = static_cast<Window_Manager*>(glfwGetWindowUserPointer(w));
                self->window_refresh_callback(w);
            });
        //----------------------------------------------------------------------------------


        LOG_INFO("Window_Manager - Window \"%s\" created (%dx%d)", config.title.c_str(), config.width, config.height);
    }
    
    
public:
    Input_Manager* get_input_manager()
    {
        return input_manager ? &(*input_manager) : nullptr;
    }
    GLFWwindow* get_handle() const { return window; }

    void subscribe(const Event_management::Event_type event_type, const Event_management::Event_receiver_shared& receiver)
    {
        input_manager->subscribe(config.channel_name, event_type, receiver);
    }

    void get_mouse_position(double& mouse_x, double& mouse_y) const
    {
        glfwGetCursorPos(window, &mouse_x, &mouse_y);
    }

    const Config& get_config_ref()
    {
        return config;
	}

    //This should always be this class anyway, porbibly uneeded func
    Window_Manager* get_user_pointer() const
    {
        return static_cast<Window_Manager*>(glfwGetWindowUserPointer(window));
	}

    Window_Manager(Event_manager& event_manager, const Config& config_in = Config())
		: config(config_in)
    {
        // Thread-safe ref-count for glfwInit / glfwTerminate
        if (instance_count++ == 0)
        {
            if (!glfwInit())
            {
                --instance_count;
                LOG_FATAL("Window_Manager - Failed to initialise GLFW!");
                throw std::runtime_error("Window_Manager - glfwInit() failed");
            }
        }

        create_window(event_manager);
    }

    ~Window_Manager()
    {
        if (window)
        {
            glfwDestroyWindow(window);
            window = nullptr;
        }

        if (--instance_count == 0)
            glfwTerminate();
    }

    void Tick()
    {
        glfwSwapBuffers(window);
        glfwPollEvents();
	}

	//Change config-----------------------------------------------------------------
    void set_title(const std::string& new_title)
    {
        glfwSetWindowTitle(window, new_title.c_str());
		config.title = new_title;
    }
    void set_size(const int w, const int h){ glfwSetWindowSize(window, w, h); }
    void set_position(int x, int y) { glfwSetWindowPos(window, x, y); }
    void set_should_close(bool value)
    {
        glfwSetWindowShouldClose(window, value ? GLFW_TRUE : GLFW_FALSE);
    }
    void iconify_window() { glfwIconifyWindow(window); }
    void restore_window() { glfwRestoreWindow(window); }
    void maximize_window() { glfwMaximizeWindow(window); }
    void focus_window() { glfwFocusWindow(window); }
    void request_attention() { glfwRequestWindowAttention(window); }
	void hide_window() { glfwHideWindow(window); }
	void show_window() { glfwShowWindow(window); }
    void set_vsync(bool enabled = false) { glfwSwapInterval(enabled ? 1 : 0); }
    void set_mouse_input_mode(int mode = GLFW_CURSOR_NORMAL) { glfwSetInputMode(window, GLFW_CURSOR, mode); }

    void toggle_fullscreen()
    {
        if (config.maximized)
        {
            glfwSetWindowMonitor(window, nullptr, config.pos_x, config.pos_y, config.width, config.height, 0);
            config.maximized = false;
        }
        else
        {

            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(window, monitor, 0, 0,
                mode->width, mode->height, mode->refreshRate);
            config.maximized = true;
        }
    }

};