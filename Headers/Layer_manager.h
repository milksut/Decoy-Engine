#pragma once
#include <Globals.h>
#include "The_event_manager.h"

class Layer_manager;

class Layer
{
protected:
    std::string Layer_name = "Layer";
    Layer_manager* layer_manager = nullptr;
    bool is_blocking_events = false;
    Event_manager event_manager;

public:
    int layer_index = -1;

    Layer(const std::string& name = "Layer")
        : Layer_name(name)
    {
    }

    virtual ~Layer() = default;

    virtual void onAttach() = 0;
    virtual void onDetach() = 0;
    virtual void Update() = 0;
    virtual void Render() = 0;

    void Throw_event(const std::string channel_name, std::unique_ptr<Event_management::Event> event)
    {
        event_manager.throw_event(channel_name, std::move(event));
    }

    Event_manager* get_event_manager()
    {
        return &event_manager;
    }

    const std::string& get_name() const
    {
        return Layer_name;
    }

    void set_blocking_events(bool blocking)
    {
        is_blocking_events = blocking;
    }

    bool is_blocking() const
    {
        return is_blocking_events;
    }

    void set_layer_manager(Layer_manager* manager)
    {
        if (layer_manager == nullptr)
            layer_manager = manager;
    }

    Layer_manager* get_layer_manager() const
    {
        return layer_manager;
    }
};

class Layer_manager
{
private:

    std::vector<std::unique_ptr<Layer>> layers;
    std::vector<std::string> mandatory_channels;
public:
    bool add_mandatory_channel(const std::string& channel_name)
    {
        auto it = std::find(mandatory_channels.begin(), mandatory_channels.end(), channel_name);
        if (it != mandatory_channels.end())
            return false;
        mandatory_channels.push_back(channel_name);
        rewire_event_managers();
        return true;
    }

    int try_add_layer(std::unique_ptr<Layer> layer, int layer_index = -1)
    {
        if (!layer)
            return -1;

        for (const auto& existing : layers)
        {
            if (existing->get_name() == layer->get_name())
                return -1;
        }

        if (layer_index < 0 || layer_index > layers.size())
            layer_index = (int)(layers.size());

        layer->set_layer_manager(this);
        layer->layer_index = layer_index;

        for (int i = layer_index; i < layers.size(); i++)
        {
            layers[i]->layer_index++;
        }

        layer->onAttach();
        layers.insert(layers.begin() + layer_index, std::move(layer));

        rewire_event_managers(layer_index - 1, layer_index + 1);

        return layer_index;
    }

    void remove_layer(const std::string& name)
    {
        int found_index = -1;

        for (int i = 0; i < layers.size(); i++)
        {
            if (found_index >= 0)
            {
                layers[i]->layer_index--;
            }
            else if (layers[i]->get_name() == name)
            {
                layers[i]->onDetach();
                layers.erase(layers.begin() + i);
                found_index = i;
                i--;
            }
        }

        rewire_event_managers(found_index - 1, found_index + 1);
    }

    Layer* get_layer(const std::string& name) const
    {
        auto it = std::find_if(layers.begin(), layers.end(),
            [&name](const std::unique_ptr<Layer>& layer) { return layer->get_name() == name; });

        return (it != layers.end()) ? it->get() : nullptr;
    }

    Layer* get_layer_at(size_t index) const
    {
        if (index >= layers.size())
            return nullptr;
        return layers[index].get();
    }

    size_t get_layer_count() const
    {
        return layers.size();
    }

    bool throw_event_at(const std::string& channel_name, std::unique_ptr<Event_management::Event> event, int layer_index)
    {
        if (layer_index < 0 || layer_index >= static_cast<int>(layers.size()))
            return false;

        if (layers[layer_index]->is_blocking())
            return false;

        layers[layer_index]->Throw_event(channel_name, std::move(event));
        return true;
    }

    bool throw_event_at(const std::string& channel_name, std::unique_ptr<Event_management::Event> event, std::string name)
    {
        Layer* ptr = get_layer(name);

        if (ptr == nullptr)
            return false;

        if (ptr->is_blocking())
            return false;

        ptr->Throw_event(channel_name, std::move(event));
        return true;
    }

    void update_all_layers()
    {
        for (const auto& layer : layers)
        {
            layer->Update();
        }
    }

    void render_all_layers()
    {
        for (const auto& layer : layers)
        {
            layer->Render();
        }
    }

    void clear_all_layers()
    {
        for (auto it = layers.rbegin(); it != layers.rend(); ++it)
        {
            (*it)->onDetach();
        }
        layers.clear();
    }

    void add_mandatory_channels()
    {
        for (auto& layer : layers)
        {
            for (std::string name : mandatory_channels)
            {
                if (layer->get_event_manager()->get_channel(name).expired())
                    layer->get_event_manager()->create_channel(name);
            }

        }
    }

    void rewire_event_managers(unsigned int start_index = 0, unsigned int end_index = 0)
    {
        if (layers.empty()) return;

        if (end_index <= 0 || end_index >= static_cast<unsigned int>(layers.size()))
            end_index = static_cast<unsigned int>(layers.size()) - 1;

        if (start_index > end_index)
            return;

        add_mandatory_channels();

        //it is importand it is i < end and not i <= end, becouse we dont need to add downstream for last layer 
        //and also it will couse out of range error
        for (unsigned int i = start_index; i < end_index; i++)
        {
            std::vector<std::string> names = layers[i]->get_event_manager()->get_all_channel_names();
            for (std::string name : names)
            {
                std::shared_ptr<Event_manager::Channel> downstream =
                    layers[i + 1]->get_event_manager()->get_channel(name).lock();
                layers[i]->get_event_manager()->connect(name, downstream);
            }
        }

        if (end_index >= static_cast<unsigned int>(layers.size()) - 1)
        {
            std::vector<std::string> names = layers[end_index]->get_event_manager()->get_all_channel_names();
            for (std::string name : names)
            {
                layers[end_index]->get_event_manager()->connect(name, nullptr);
            }
        }

    }
};