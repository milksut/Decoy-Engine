#pragma once

#include <entt/entt.hpp>

struct ChildComponent 
{
    std::vector<entt::entity> children;
};

struct ParentComponent 
{
    entt::entity parent{ entt::null };
};