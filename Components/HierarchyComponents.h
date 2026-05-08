#pragma once

#include <entt/entt.hpp>
#include <game_object_basic.h>

//TODO: Change it to work with any class that has a ParentComponent and ChildComponent, not just game_object_basic

struct ChildComponent 
{
    std::vector<game_object_basic*> children;
};

struct ParentComponent 
{
    game_object_basic* parent = nullptr;
};

struct Self_component //to get responding class from entt
{
    game_object_basic* this_object = nullptr;
};