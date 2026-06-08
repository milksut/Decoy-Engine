#pragma once

#include <string>

class Event_Manager; 

namespace Decoy {

    class Layer
    {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer();

        virtual void onAttach() {}
        virtual void onDetach() {}
        virtual void onUpdate(float dt) {}
        virtual void onImGuiRender() {}
        virtual void onEvent(Event_Manager& event) {}

        const std::string& getName() const { return m_DebugName; }

    protected:
        std::string m_DebugName;
    };
}