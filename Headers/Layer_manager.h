#pragma once
#include <Globals.h>

class Layer_manager;

class Layer
{
protected:
    std::string Layer_name = "Layer";
    Layer_manager* layer_manager = nullptr;
    bool is_blocking_events = false;

public:
    int layer_index = 0;

    Layer(const std::string& name = "Layer");
	: Layer_name(name)
    }

    virtual ~Layer() = default;

    virtual void onAttach() = 0;
    virtual void onDetach() = 0;
    virtual void Update() = 0;
    virtual void Render() = 0;

    //virtual bool onEvent(Event_management::Event& event) = 0;

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
        if(layer_manager == nullptr)
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

public:
    int try_add_layer(std::unique_ptr<Layer> layer, unsigned int layer_index = -1)
    {
        if (!layer)
            return -1;

        for (const auto& existing : layers)
        {
            if (existing->get_name() == layer->get_name())
                return -1;
        }

        if(layer_index < 0 || layer_index > layers.size())
			layer_index = layers.size();

        layer->set_layer_manager(this);
		layer->layer_index = layer_index;

        for(int i = layer_index; i < layers.size(); i++)
        {
            layers[i]->layer_index++;
		}

        layers.push_back(std::move(layer));
        layer->onAttach();

        return layer->layer_index;
    }

    void remove_layer(const std::string& name)
    {
		bool found = false;

        for(int i = 0; i < layers.size(); i++)
        {
            if (found)
            {
                layers[i]->layer_index--;
            }
            else if(layers[i]->get_name() == name)
            {
                layers[i]->onDetach();
                layers.erase(layers.begin() + i);
                found = true;
                i--;
			}
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

    bool dispatch_event(Event_management::Event& event)
    {
        for (auto it = layers.rbegin(); it != layers.rend(); ++it)
        {
            if ((*it)->onEvent(event))
                return true;

            if ((*it)->is_blocking())
                return false;
        }
        return false;
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
        next_z_order = 0;
    }
};