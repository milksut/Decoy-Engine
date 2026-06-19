#pragma once

#include "Globals.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

struct RigidBody_component
{
    JPH::BodyID body_id;
    bool is_trigger = false;
    bool is_static = false;

    Event_management::Event_receiver_shared event_reciver = nullptr;

    RigidBody_component(const JPH::BodyID id, Event_management::Event_receiver_shared event_reciver_in,
        const bool is_static_in = false, const bool is_trigger_in = false)
        : body_id(id), is_static(is_static_in), is_trigger(is_trigger_in), event_reciver(event_reciver_in)
    {
    }
};