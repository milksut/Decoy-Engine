#pragma once

#include <glm/glm.hpp>

struct Transform_component 
{
    //the local position,not affected by parent
    glm::vec3 position = glm::vec3(0.0);
    glm::vec3 rotation = glm::vec3(0.0);
    glm::vec3 scale = glm::vec3(1.0);
    
    //the position on the world, affected by parent
    glm::mat4 world = glm::mat4(1.0);
};