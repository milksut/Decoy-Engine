#pragma once

//TODO: Change it to work with any class that has a ParentComponent and ChildComponent, not just game_object_basic

struct ChildComponent 
{
    std::vector<void*> children;
};

struct ParentComponent 
{
    void* parent = nullptr;
};

struct Self_component //to get responding class from entt
{
    void* this_object = nullptr;
};