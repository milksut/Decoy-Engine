#pragma once
#include <glm/glm.hpp>
#include <entt/entt.hpp>

// Transform Component
struct TransformComponent {
    glm::vec3 position{ 0.0f };
    glm::vec3 rotation{ 0.0f };
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

    glm::mat4 local{ 1.0f };
    glm::mat4 world{ 1.0f };

    bool pos_change_flag = false; 
};

// Child ID Component
struct ChildComponent {
    entt::entity parent{ entt::null }; // ebeveyn entity
};

// Tag Component
struct TagComponent {
    std::string name;
};