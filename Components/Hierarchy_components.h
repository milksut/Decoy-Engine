#pragma once

#include <vector>

struct Child_component 
{
    std::vector<unsigned int> children_ids;
};

struct Parent_component 
{
    unsigned int parent_id = 0;
};

//Self_Components are now Depricated, use own id(id_component from Tag_components) to get your pointer from Global_object_registry

//struct Self_component //to get responding class from entt
//{
//    void* this_object = nullptr;
//};