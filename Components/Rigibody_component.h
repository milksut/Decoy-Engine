#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

struct RigidBody_component
{
    JPH::BodyID body_id;
    bool        is_static = false;

    RigidBody_component() = default;
    RigidBody_component(JPH::BodyID id, bool static_body = false)
        : body_id(id), is_static(static_body)
    {
    }
};