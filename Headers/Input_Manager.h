#pragma once

#include "Globals.h"
#include "The_event_manager.h"
#include "Events/Keyborad_events.h"
#include "Events/Mouse_events.h"
#include "Events/Combo_press_events.h"

///Works on 1 window, crate 1 manager per window, and use it on same thread.
///You can set down streams if you want to transport event but
///events only sent to next only if they are not consumed!
///Run Poll_keys every tick or so to check key event, it's not work by himself
class Input_Manager
{
private:
    
    //Separate pressed state arrays for keys and mouse buttons
    Key_state Keyboard_buttons[GLFW_KEY_LAST + 1] = { Idle };
    Key_state Mouse_buttons[GLFW_MOUSE_BUTTON_LAST + 1] = { Idle };

    std::vector<std::pair<Key_combo, Key_state>> registered_combos;
    Event_manager& event_manager;
	GLFWwindow* window = nullptr;

public:

    double mouse_x = 0, mouse_y = 0, mouse_sensitivity = 1;  

    //TODO:fix params
    /// <summary>
    ///     Initializes the input manager, sets up event channels and registers GLFW input callbacks. Width and height only used for starting mouse position.
    /// </summary>
    /// <param name="event_manager">[in] Reference to the event manager used for input events.</param>
    /// <param name="window">[in] Pointer to the GLFW window used for input callbacks.</param>
    Input_Manager(Event_manager& event_manager_in, GLFWwindow* window_in)
        : event_manager(event_manager_in), window(window_in)
    {
        
        //TODO: decide what to do if window chcek fails
        //Checking if window crated using a window manager class
        void* ptr = glfwGetWindowUserPointer(window);

        if (!ptr)
        {
			LOG_ERROR("Input_Manager - No user pointer found for window, ensure the window was created using Window_Manager.");
        }

        for(int i=0; i< Input_channel_amount; i++)
        {
            event_manager.create_channel(Input_channel_names[i]);
		}
    }

    /// <summary>
    ///     Returns the current state of the specified input key (keyboard or mouse).
    /// </summary>
    /// <param name="key">[in] The input key to query.</param>
    /// <returns>Current state of the key, or Idle if the input type is unknown.</returns>
    Key_state Get_key_state(Input_key key) const
    {
        if (key.type == Keyboard_input) return Keyboard_buttons[key.code];
        else if (key.type == Mouse_input)    return Mouse_buttons[key.code];
        else
        {
			LOG_ERROR("Input_Manager - Get_key_state failed, unknown input type: %d", (int)key.type);
            return Idle;
        }
        
    }
	//TODO: Add a function to get combo state, Probably not needed though,

    /// <summary>
    ///     Registers a new input key combo if it is not already registered.
    /// </summary>
    /// <param name="keys">[in] List of keys that form the combo.</param>
    void register_combo(const std::vector<Input_key>& keys)
    {
        //i am not sure if this okey, it's slow, but how often do you add combos any way?
        for (const auto& combo : registered_combos)
            if (combo.first.keys == keys) return; // already registered

		registered_combos.push_back({ Key_combo{keys}, Idle });
	}

    /// <summary>
    ///     Polls keyboard, mouse, and registered combo inputs and dispatches corresponding input events.
    /// </summary>
    void Poll_keys()
    {
        // Poll keyboard keys explicitly
        for (int key = 0; key <= GLFW_KEY_LAST; key++)
        {
            bool is_pressed = glfwGetKey(window, key) == GLFW_PRESS;
			bool was_pressed = Keyboard_buttons[key] == Pressed || Keyboard_buttons[key] == Hold;

            if (is_pressed && was_pressed)
            {
                event_manager.throw_event("Keyboard_input", std::make_unique<Key_hold_event>(Input_key{ Keyboard_input, key }));
				Keyboard_buttons[key] = Hold;
            }
            else if (is_pressed)
            {
                event_manager.throw_event("Keyboard_input", std::make_unique<Key_press_event>(Input_key{ Keyboard_input, key}));
				Keyboard_buttons[key] = Pressed;
            }
            else if (was_pressed)
            {
                event_manager.throw_event("Keyboard_input", std::make_unique<Key_release_event>(Input_key{ Keyboard_input, key}));
				Keyboard_buttons[key] = Released;
            }
            else
            {
				Keyboard_buttons[key] = Idle;
            }
        }

        // Poll mouse buttons explicitly
        for (int btn = 0; btn <= GLFW_MOUSE_BUTTON_LAST; btn++)
        {
            bool is_pressed = glfwGetMouseButton(window, btn) == GLFW_PRESS;
            bool was_pressed = Mouse_buttons[btn] == Pressed || Mouse_buttons[btn] == Hold;

            if (is_pressed && was_pressed)
            {
                event_manager.throw_event("Mouse_input", std::make_unique<Mouse_button_hold_event>(Input_key{ Mouse_input, btn }, mouse_x, mouse_y));
				Mouse_buttons[btn] = Hold;
            }
            else if (is_pressed)
            {
                event_manager.throw_event("Mouse_input", std::make_unique<Mouse_button_press_event>(Input_key{ Mouse_input, btn }, mouse_x, mouse_y));
				Mouse_buttons[btn] = Pressed;
            }
            else if (was_pressed)
            {
                event_manager.throw_event("Mouse_input", std::make_unique<Mouse_button_release_event>(Input_key{ Mouse_input, btn }, mouse_x, mouse_y));
				Mouse_buttons[btn] = Released;
            }
            else
            {
				Mouse_buttons[btn] = Idle;
            }
        }

        // Poll combo buttons explicitly
        for (auto& combo : registered_combos)
        {
            bool is_pressed = true;

            for (Input_key key : combo.first.keys)
            {
                Key_state state = Get_key_state(key);
                if (!(state == Pressed || state == Hold))
                {
                    is_pressed = false;
                    break;
                }
            }

            bool was_pressed = combo.second == Pressed || combo.second == Hold;

            if (is_pressed && was_pressed)
            {
                event_manager.throw_event("Combo_input", std::make_unique<Combo_button_hold_event>(combo.first));
                combo.second = Hold;
            }
            else if (is_pressed)
            {
                event_manager.throw_event("Combo_input", std::make_unique<Combo_button_press_event>(combo.first));
                combo.second = Pressed;
            }
            else if (was_pressed)
            {
                event_manager.throw_event("Combo_input", std::make_unique<Combo_button_release_event>(combo.first));
                combo.second = Released;
            }
            else
            {
                combo.second = Idle;
            }
        }
    }


    /// <summary>
    ///     Subscribes a receiver to a specific input channel and event type.
    /// </summary>
    /// <param name="channel_name">[in] Input channel name to subscribe to.</param>
    /// <param name="event_type">[in] Type of event to listen for.</param>
    /// <param name="receiver">[in] Event receiver to be notified.</param>
    void subscribe(std::string channel_name, const Event_management::Event_type event_type, const Event_management::Event_receiver_shared& receiver)
    {
		event_manager.subscribe(channel_name, event_type, receiver);
    }

    /// <summary>
    ///     Sends an event to the specified input channel.
    /// </summary>
    /// <param name="channel_name">[in] Input channel name where the event will be sent.</param>
    /// <param name="event">[in] Event object to send.</param>
    void throw_event(std::string channel_name, std::unique_ptr<Event_management::Event> event)
    {
        event_manager.throw_event(channel_name, std::move(event));
	}

    /// <summary>
    ///     Creates a new input event channel.
    /// </summary>
    /// <param name="channel_name">[in] Name of the channel to create.</param>
    void create_channel(std::string channel_name)
    {
        event_manager.create_channel(channel_name);
	}
};