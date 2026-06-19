#pragma once

#include "Globals.h" 
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

// Inbound commands
struct Apply_force_event : public Event_management::Event
{
    JPH::BodyID body_id;
    JPH::Vec3   force;
    Apply_force_event(JPH::BodyID id, JPH::Vec3 f)
        : Event(Event_management::Event_timing::Queued, Event_management::Event_type::Physics_apply_force),
        body_id(id), force(f)
        {}
};

struct Apply_impulse_event : public Event_management::Event
{
    JPH::BodyID body_id;
    JPH::Vec3   impulse;
    Apply_impulse_event(JPH::BodyID id, JPH::Vec3 imp)
        : Event(Event_management::Event_timing::Queued, Event_management::Event_type::Physics_apply_impulse),
        body_id(id), impulse(imp)
        {}
};

struct Set_velocity_event : public Event_management::Event
{
    JPH::BodyID body_id;
    JPH::Vec3   velocity;
    Set_velocity_event(JPH::BodyID id, JPH::Vec3 v)
        : Event(Event_management::Event_timing::Queued, Event_management::Event_type::Physics_set_velocity),
        body_id(id), velocity(v)
            {}
};

struct Set_gravity_event : public Event_management::Event
{
    JPH::Vec3 gravity;
    Set_gravity_event(JPH::Vec3 g)
        : Event(Event_management::Event_timing::Queued, Event_management::Event_type::Physics_set_gravity),
        gravity(g)
        {}
};

// Outbound notifications
struct Physics_body_created_event : public Event_management::Event
{
    JPH::BodyID body_id;
    Physics_body_created_event(JPH::BodyID id)
        : Event(Event_management::Event_timing::Queued, Event_management::Event_type::Physics_body_crated),
        body_id(id)
    {
    }
};

struct Physics_body_removed_event : public Event_management::Event
{
    JPH::BodyID body_id;
    Physics_body_removed_event(JPH::BodyID id)
        : Event(Event_management::Event_timing::Queued, Event_management::Event_type::Physics_body_removed),
        body_id(id)
    {
    }
};

struct Collision_begin_event : public Event_management::Event
{
    unsigned int self_id;
    unsigned int other_id;
    glm::vec3   contact_point;

    Collision_begin_event(unsigned int a, unsigned int b, glm::vec3 pt,const Event_management::Event_timing timing,
        const Event_management::Event_receiver_shared& target_receiver_in)
        : Event(timing, target_receiver_in, Event_management::Event_type::Physics_collision_begin),
        self_id(a), other_id(b), contact_point(pt)
    {}

    Collision_begin_event(unsigned int a, unsigned int b, glm::vec3 pt, const Event_management::Event_timing timing)
        : Event(timing, Event_management::Event_type::Physics_collision_begin),
        self_id(a), other_id(b), contact_point(pt)
    {}
};

struct Collision_persisted_event : public Event_management::Event
{
    unsigned int self_id;
    unsigned int other_id;
    glm::vec3   contact_point;

    Collision_persisted_event(unsigned int a, unsigned int b, glm::vec3 pt, const Event_management::Event_timing timing,
        const Event_management::Event_receiver_shared& target_receiver_in)
        : Event(timing, target_receiver_in, Event_management::Event_type::Physics_collision_persists),
        self_id(a), other_id(b), contact_point(pt)
    {}

    Collision_persisted_event(unsigned int a, unsigned int b, glm::vec3 pt, const Event_management::Event_timing timing)
        : Event(timing, Event_management::Event_type::Physics_collision_persists),
        self_id(a), other_id(b), contact_point(pt)
    {}
};

struct Collision_end_event : public Event_management::Event
{
    unsigned int self_id;
    unsigned int other_id;

    Collision_end_event(unsigned int a, unsigned int b, const Event_management::Event_timing timing,
        const Event_management::Event_receiver_shared& target_receiver_in)
        : Event(timing, target_receiver_in, Event_management::Event_type::Physics_collision_end),
        self_id(a), other_id(b)
    {}

    Collision_end_event(unsigned int a, unsigned int b, const Event_management::Event_timing timing)
        : Event(timing, Event_management::Event_type::Physics_collision_end),
        self_id(a), other_id(b)
    {}
};

struct Trigger_enter_event : public Event_management::Event
{
    unsigned int self_id;
    unsigned int other_id;
    glm::vec3   contact_point;

    Trigger_enter_event(unsigned int a, unsigned int b, glm::vec3 pt, const Event_management::Event_timing timing,
        const Event_management::Event_receiver_shared& target_receiver_in)
        : Event(timing, target_receiver_in, Event_management::Event_type::Physics_trigger_enter),
        self_id(a), other_id(b), contact_point(pt)
    {}

    Trigger_enter_event(unsigned int a, unsigned int b, glm::vec3 pt, const Event_management::Event_timing timing)
        : Event(timing, Event_management::Event_type::Physics_trigger_enter),
        self_id(a), other_id(b), contact_point(pt)
    {}
};

struct Trigger_persisted_event : public Event_management::Event
{
    unsigned int self_id;
    unsigned int other_id;
    glm::vec3   contact_point;

    Trigger_persisted_event(unsigned int a, unsigned int b, glm::vec3 pt, const Event_management::Event_timing timing,
        const Event_management::Event_receiver_shared& target_receiver_in)
        : Event(timing, target_receiver_in, Event_management::Event_type::Physics_trigger_persists),
        self_id(a), other_id(b), contact_point(pt)
    {}

    Trigger_persisted_event(unsigned int a, unsigned int b, glm::vec3 pt, const Event_management::Event_timing timing)
        : Event(timing, Event_management::Event_type::Physics_trigger_persists),
        self_id(a), other_id(b), contact_point(pt)
    {}
};

struct Trigger_exit_event : public Event_management::Event
{
    unsigned int self_id;
    unsigned int other_id;

    Trigger_exit_event(unsigned int a, unsigned int b, const Event_management::Event_timing timing,
        const Event_management::Event_receiver_shared& target_receiver_in)
        : Event(timing, target_receiver_in, Event_management::Event_type::Physics_trigger_exit),
        self_id(a), other_id(b)
    {}

    Trigger_exit_event(unsigned int a, unsigned int b, const Event_management::Event_timing timing)
        : Event(timing, Event_management::Event_type::Physics_trigger_exit),
        self_id(a), other_id(b)
    {}
};

