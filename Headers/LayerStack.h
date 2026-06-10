#pragma once
#include <vector>
#include <algorithm>
#include "Layer.h"

namespace Decoy {
    class LayerStack
    {
    public:
        LayerStack() : m_LayerInsertIndex(0) {}

        ~LayerStack()
        {
            for (Layer* layer : m_Layers)
            {
                layer->onDetach();
                delete layer;
            }
        }

        void pushLayer(Layer* layer)
        {
            m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
            m_LayerInsertIndex++;
            layer->onAttach();
        }

        void pushOverlay(Layer* overlay)
        {
            m_Layers.emplace_back(overlay);
            overlay->onAttach();
        }

        void popLayer(Layer* layer)
        {
            auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);
            if (it != m_Layers.begin() + m_LayerInsertIndex)
            {
                layer->onDetach();
                m_Layers.erase(it);
                m_LayerInsertIndex--;
            }
        }

        void popOverlay(Layer* overlay)
        {
            auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);
            if (it != m_Layers.end())
            {
                overlay->onDetach();
                m_Layers.erase(it);
            }
        }

        std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
        std::vector<Layer*>::iterator end() { return m_Layers.end(); }

    private:
        std::vector<Layer*> m_Layers;
        unsigned int m_LayerInsertIndex = 0;
    };
}