#pragma once
#include "Globals.h"
#include "The_event_manager.h"
#include "Components/Transform_components.h"
#include "Components/Rigibody_component.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

//TODO:add param, logs
//TODO: integrate events

// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
namespace Object_layers
{
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::uint        NUM_LAYERS = 2;
}

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace Broadphase_layers
{
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint            NUM_LAYERS = 2;
}


// This defines a mapping between object and broadphase layers.
class Broadphase_layers_interface : public JPH::BroadPhaseLayerInterface
{
public:
    JPH::uint GetNumBroadPhaseLayers() const override
    {
        return Broadphase_layers::NUM_LAYERS;
    }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {
        return layer == Object_layers::NON_MOVING ? Broadphase_layers::NON_MOVING : Broadphase_layers::MOVING;
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
    {
        return layer == Broadphase_layers::NON_MOVING ? "NON_MOVING" : "MOVING";
    }
#endif
};

// Class that determines if two object layers can collide
class Object_layers_filter : public JPH::ObjectLayerPairFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override
    {
        switch (a)
        {
            case Object_layers::NON_MOVING:
                return b == Object_layers::MOVING;

            case Object_layers::MOVING:
                return true; // moving vs everything

            default: return false;
        }
    }
};

// Class that determines if an object layer can collide with a broadphase layer
class Object_vs_Broadphase_filter : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer bp) const override
    {
        switch (layer)
        {
            case Object_layers::NON_MOVING:
                return bp == Broadphase_layers::MOVING;

            case Object_layers::MOVING:
                return true;

            default:
                return false;
        }
    }
};

class Physics_manager
{
private:
    const unsigned int      temp_allocator_size = 10 * 1024 * 1024; // ~10mb
    const unsigned int      Max_bodies = 65536;
    const unsigned int      Num_body_mutexes = 0; // 0 = auto ???
    const unsigned int      Max_body_pairs = 65536;
    const unsigned int      Max_contact_constraints = 10240;

    static constexpr float  fixed_step = 1.0f / 60.0f;
    float                   accumulator = 0.0f;

    JPH::Vec3 gravity = JPH::Vec3(0.f, -9.81f, 0.f);

    Broadphase_layers_interface Bp_layer_interface;
    Object_layers_filter        O_layer_filter;
    Object_vs_Broadphase_filter O_vs_BP_filter;

    std::unique_ptr<JPH::TempAllocatorImpl>    temp_allocator = nullptr;
    std::unique_ptr<JPH::JobSystemThreadPool>  job_system = nullptr;
    std::unique_ptr<JPH::PhysicsSystem>        physics_system = nullptr;

    //TODO: integrate events
    Event_manager* event_manager = nullptr;

public:

    void SyncPhysicsToECS(entt::registry& registry)
    {
        if (physics_system == nullptr)
            return;

        auto view = registry.view<RigidBody_component, Transform_component>();
        JPH::BodyInterface& bi = physics_system->GetBodyInterface();

        for (auto entity : view)
        {
            auto& rb = view.get<RigidBody_component>(entity);
            auto& tf = view.get<Transform_component>(entity);

            if (!bi.IsActive(rb.body_id)) continue; // skip sleeping bodies

            JPH::RVec3 pos = bi.GetCenterOfMassPosition(rb.body_id);
            JPH::Quat  rot = bi.GetRotation(rb.body_id);

            // Convert JPH → GLM (Jolt uses right-hand Y-up, same as OpenGL)
            tf.position = glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
            tf.rotation_quat = glm::quat(rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ());

            //becouse euler returns radians ZYX
            tf.rotation = glm::degrees(glm::eulerAngles(tf.rotation_quat));
            std::swap(tf.rotation.x, tf.rotation.z);
        }
    }

    //TODO: event manager connection
    Physics_manager(Event_manager* event_manager_in)
        : event_manager(event_manager_in)
    {
        // must be FIRST
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        // Temp allocator: temp_allacator_size scratch per frame (stack-based, very fast)
        temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(temp_allocator_size);

        // Job system: uses Jolt's built-in thread pool
        job_system = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
            std::thread::hardware_concurrency() - 1);

        // ???
        physics_system = std::make_unique<JPH::PhysicsSystem>();
        physics_system->Init(Max_bodies, Num_body_mutexes, Max_body_pairs, Max_contact_constraints,
            Bp_layer_interface, O_vs_BP_filter, O_layer_filter);

        set_gravity(gravity);
    }

    ~Physics_manager()
    {
        physics_system.reset();
        job_system.reset();
        temp_allocator.reset();

        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    Physics_manager(const Physics_manager&) = delete;
    Physics_manager& operator=(const Physics_manager&) = delete;

    void set_gravity(JPH::Vec3 new_grav = JPH::Vec3(0.f, -9.81f, 0.f))
    {
        if (physics_system == nullptr)
            return;

        gravity = new_grav;
        physics_system->SetGravity(gravity);
    }

    //TODO: make body creating methods so dont need to release body_interface
    // use this to crate bodys
    JPH::BodyInterface* get_body_interface()
    {
        if (physics_system == nullptr)
            return nullptr;

        return &(physics_system->GetBodyInterface());
    }

    void Tick(float delta_time, entt::registry& registry)
    {
        if (temp_allocator == nullptr || job_system == nullptr || physics_system == nullptr)
            return;

        accumulator += delta_time;
        accumulator = std::min(accumulator, 0.25f);

        while (accumulator >= fixed_step)
        {
            // collision_steps = 1 is fine for most games; increase for fast objects
            physics_system->Update(fixed_step, /*collision_steps=*/1, temp_allocator.get(), job_system.get());
            accumulator -= fixed_step;
        }

        SyncPhysicsToECS(registry);
    }

    //TODO add safety checks to these
    bool cast_ray(const JPH::RRayCast ray, JPH::RayCastResult& hit, JPH::BroadPhaseLayerFilter bp_filter = {},
        JPH::ObjectLayerFilter ol_filter = {}, JPH::BodyFilter body_filter = {})
    {
        if (physics_system == nullptr)
            return false;

        return physics_system->GetNarrowPhaseQuery().CastRay(
            ray, hit, bp_filter, ol_filter, body_filter);
    }

    bool cast_ray_all(const JPH::RRayCast& ray, std::vector<JPH::RayCastResult>& hits, const JPH::BroadPhaseLayerFilter& bp_filter = {},
        const JPH::ObjectLayerFilter& ol_filter = {}, const JPH::BodyFilter& body_filter = {})
    {
        if (physics_system == nullptr)
            return false;

        JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
        physics_system->GetNarrowPhaseQuery().CastRay(
            ray, JPH::RayCastSettings{}, collector, bp_filter, ol_filter, body_filter);

        if (!collector.HadHit())
            return false;

        collector.Sort();

        hits.reserve(collector.mHits.size());

        hits.assign(collector.mHits.begin(), collector.mHits.end());

        /*for (auto& result : collector.mHits)
            hits.push_back(result);*/

        return !hits.empty();
    }

    //Continuous force(e.g.thrust) — call every physics step
    void add_force(const JPH::BodyID body_id, const JPH::Vec3 force)
    {
        if (physics_system == nullptr)
            return;

        physics_system->GetBodyInterface().AddForce(body_id, force);
    }

    // Instant impulse (e.g. explosion knockback)
    void add_impulse(const JPH::BodyID body_id, const JPH::Vec3 force)
    {
        if (physics_system == nullptr)
            return;

        physics_system->GetBodyInterface().AddImpulse(body_id, force);
    }

    // Velocity directly
    void set_linear_velocity(const JPH::BodyID body_id, const JPH::Vec3 velocity)
    {
        if (physics_system == nullptr)
            return;

        physics_system->GetBodyInterface().SetLinearVelocity(body_id, velocity);
    }

    // Gravity scale per body (e.g. feather = 0.1, heavy rock = 2.0)
    void set_gravity_factor(const JPH::BodyID body_id, const float factor)
    {
        if (physics_system == nullptr)
            return;

        physics_system->GetBodyInterface().SetGravityFactor(body_id, factor);
    }


    void remove_body(const JPH::BodyID body_id)
    {
        if (physics_system == nullptr)
            return;

        physics_system->GetBodyInterface().RemoveBody(body_id);
    }


    void delete_body(const JPH::BodyID body_id)
    {
        if (physics_system == nullptr)
            return;

        remove_body(body_id);
        physics_system->GetBodyInterface().DestroyBody(body_id);
    }
};