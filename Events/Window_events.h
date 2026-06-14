#pragma once

#include "Globals.h"

/// <summary>
///     Event triggered when the application window is resized.
/// </summary>
/// <remarks>
///     Stores the new window dimensions and computed aspect ratio.
/// </remarks>
struct Window_resize_event : public Event_management::Event
{
	int new_width;
	int new_height;
	float new_aspect_ratio;
	Window_resize_event(int width, int height)
		: Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_resized),
		new_width(width), new_height(height), new_aspect_ratio(static_cast<float>(width) / static_cast<float>(height))
	{}
};	

/// <summary>
///     Event triggered when the window framebuffer is resized.
/// </summary>
/// <remarks>
///     Stores the new framebuffer dimensions and computed aspect ratio.
/// </remarks>
struct Window_framebuffer_resize_event : public Event_management::Event
{
	int new_width;
	int new_height;
	float new_aspect_ratio;
	Window_framebuffer_resize_event(int width, int height)
		: Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_framebuffer_resized),
		new_width(width), new_height(height), new_aspect_ratio(static_cast<float>(width) / static_cast<float>(height))
	{}
};

/// <summary>
///     Event triggered when the window is requested to close.
/// </summary>
/// <remarks>
///     Contains no additional data; only signals a close request.
/// </remarks>
struct Window_close_event : public Event_management::Event
{
    Window_close_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_closed)
    {}
};

/// <summary>
///     Event triggered when the window is opened.
/// </summary>
/// <param name="none">This event has no parameters.</param>
struct Window_open_event : public Event_management::Event
{
    Window_open_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_opened)
    {}
};

/// <summary>
/// Triggered when the window gains focus.
/// </summary>
struct Window_focus_event : public Event_management::Event
{
    Window_focus_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_focused)
    {}
};

/// <summary>Triggered when the window loses focus.</summary>
struct Window_unfocus_event : public Event_management::Event
{
    Window_unfocus_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_unfocused)
    {}
};

/// <summary>Triggered when the window is minimized (iconified).</summary>
struct Window_iconify_event : public Event_management::Event
{

    Window_iconify_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_iconified)
    {}
};

/// <summary>Triggered when the window is restored from minimized state.</summary>
struct Window_uniconify_event : public Event_management::Event
{

    Window_uniconify_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_uniconified)
    {}
};

/// <summary>Triggered when the window is maximized.</summary>
struct Window_maximize_event : public Event_management::Event
{

    Window_maximize_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_maximized)
    {}
};

/// <summary>Triggered when the window is restored from maximized or minimized state.</summary>
struct Window_restore_event : public Event_management::Event
{

    Window_restore_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_restored)
    {}
};

/// <summary>Triggered when the window is moved on the screen.</summary>
struct Window_move_event : public Event_management::Event
{
    int x, y;

    Window_move_event(int x_in, int y_in)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_moved),
        x(x_in), y(y_in)
    {}
};

/// <summary>Triggered when the window content scaling changes (e.g., DPI scaling).</summary>
struct Window_content_scale_event : public Event_management::Event
{
    float x_scale, y_scale;

    Window_content_scale_event(float xs, float ys)
        : Event(Event_management::Event_timing::Queued, Event_management::Event_type::Window_content_scaled),
        x_scale(xs), y_scale(ys) 
    {}
};

/// <summary>Triggered when the window requests a redraw/refresh.</summary>
/// <remarks>Usually fired when the window content needs to be repainted.</remarks>
struct Window_refresh_event : public Event_management::Event
{
    Window_refresh_event()
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Window_refresh)
    {}
};