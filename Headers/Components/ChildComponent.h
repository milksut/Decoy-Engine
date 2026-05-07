#pragma once

#include <entt/entt.hpp>

struct ChildComponent {
    entt::entity parent{ entt::null };
};