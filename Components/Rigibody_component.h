#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

struct RigidBody_component
{
    JPH::BodyID body_id;
    bool is_trigger = false;
    bool is_static = false;

    //TODO:add evenet reciver for selected object

    RigidBody_component(const JPH::BodyID id, const bool is_static_in = false, const bool is_trigger_in = false)
        : body_id(id), is_static(is_static_in), is_trigger(is_trigger_in)
    {
    }
};