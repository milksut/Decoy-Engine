#pragma once
#include "Globals.h"
#include "The_event_manager.h"
#include "Components/Transform_components.h"
#include "Components/Rigibody_component.h"

#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

namespace Object_layers
{
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::uint        NUM_LAYERS = 2;
}

namespace Broadphase_layers
{
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint            NUM_LAYERS = 2;
}

class Broadphase_layers_interface : public JPH::BroadPhaseLayerInterface
{
public:
    JPH::uint GetNumBroadPhaseLayers() const override
    {
        return Broadphase_layers::NUM_LAYERS;
    }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {
        return layer == Object_layers::NON_MOVING
            ? Broadphase_layers::NON_MOVING
            : Broadphase_layers::MOVING;
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
    {
        return layer == Broadphase_layers::NON_MOVING ? "NON_MOVING" : "MOVING";
    }
#endif
};

class Object_layers_filter : public JPH::ObjectLayerPairFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override
    {
        switch (a)
        {
        case Object_layers::NON_MOVING: return b == Object_layers::MOVING;
        case Object_layers::MOVING:     return true;
        default:                        return false;
        }
    }
};

class Object_vs_Broadphase_filter : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer bp) const override
    {
        switch (layer)
        {
        case Object_layers::NON_MOVING: return bp == Broadphase_layers::MOVING;
        case Object_layers::MOVING:     return true;
        default:                        return false;
        }
    }
};

class Physics_manager
{
private:
    const unsigned int      temp_allocator_size = 10 * 1024 * 1024; // ~10mb
    const unsigned int      Max_bodies = 65536;
    const unsigned int      Num_body_mutexes = 0; // 0 = auto
    const unsigned int      Max_body_pairs = 65536;
    const unsigned int      Max_contact_constraints = 10240;

    static constexpr float  fixed_step = 1.0f / 60.0f;
    float                   accumulator = 0.0f;

    JPH::Vec3 gravity = JPH::Vec3(0.f, -9.81f, 0.f);

    Broadphase_layers_interface Bp_layer_interface;
    Object_layers_filter        O_layer_filter;
    Object_vs_Broadphase_filter O_vs_BP_filter;

    // unique_ptr kullanilmiyor: JPH nesneleri new ile atanmali, unique_ptr ile uyumsuz
    JPH::TempAllocatorImpl* temp_allocator = nullptr;
    JPH::JobSystemThreadPool* job_system = nullptr;
    JPH::PhysicsSystem* physics_system = nullptr;

    //TODO: integrate events
    Event_manager* event_manager = nullptr;

public:

    void SyncPhysicsToECS(entt::registry& registry)
    {

    }

    //TODO: add param, logs
    Physics_manager(Event_manager* event_manager_in = nullptr)
        : event_manager(event_manager_in)
    {
        // Siralama onemli: once allocator, sonra factory, sonra types
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        // Temp allocator: frame basi scratch bellek (stack tabanli, cok hizli)
        temp_allocator = new JPH::TempAllocatorImpl(temp_allocator_size);
        // Job system: Jolt'un kendi thread pool'u
        job_system = new JPH::JobSystemThreadPool(
            JPH::cMaxPhysicsJobs,
            JPH::cMaxPhysicsBarriers,
            static_cast<int>(std::thread::hardware_concurrency()) - 1
        );
        physics_system = new JPH::PhysicsSystem();
        physics_system->Init(
            Max_bodies, Num_body_mutexes, Max_body_pairs, Max_contact_constraints,
            Bp_layer_interface, O_vs_BP_filter, O_layer_filter
        );

        set_gravity(gravity);
    }

    ~Physics_manager()
    {
        delete physics_system;
        delete job_system;
        delete temp_allocator;

        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    Physics_manager(const Physics_manager&) = delete;
    Physics_manager& operator=(const Physics_manager&) = delete;

    void set_gravity(JPH::Vec3 new_grav = JPH::Vec3(0.f, -9.81f, 0.f))
    {
        if (!physics_system) return;
        gravity = new_grav;
        physics_system->SetGravity(gravity);
    }

    //TODO: make body creating methods so dont need to release body_interface
    JPH::BodyInterface* get_body_interface()
    {
        if (!physics_system) return nullptr;
        return &physics_system->GetBodyInterface();
    }

    void Tick(float delta_time, entt::registry& registry)
    {
        if (!temp_allocator || !job_system || !physics_system) return;

        accumulator += delta_time;
        accumulator = std::min(accumulator, 0.25f);

        while (accumulator >= fixed_step)
        {
            // collision_steps = 1 cogu oyun icin yeterli, hizli objeler icin artir
            physics_system->Update(fixed_step, 1, temp_allocator, job_system);
            accumulator -= fixed_step;
        }

        SyncPhysicsToECS(registry);
    }

    //TODO: add safety checks
    bool cast_ray(const JPH::RRayCast& ray, JPH::RayCastResult& hit,
        const JPH::BroadPhaseLayerFilter& bp_filter = {},
        const JPH::ObjectLayerFilter& ol_filter = {},
        const JPH::BodyFilter& body_filter = {})
    {
        if (!physics_system) return false;
        return physics_system->GetNarrowPhaseQuery().CastRay(
            ray, hit, bp_filter, ol_filter, body_filter);
    }
    bool cast_ray_all(const JPH::RRayCast& ray,
        std::vector<JPH::RayCastResult>& hits,
        const JPH::BroadPhaseLayerFilter& bp_filter = {},
        const JPH::ObjectLayerFilter& ol_filter = {},
        const JPH::BodyFilter& body_filter = {})
    {
        if (!physics_system) return false;

        JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
        physics_system->GetNarrowPhaseQuery().CastRay(
            ray, JPH::RayCastSettings{}, collector, bp_filter, ol_filter, body_filter);

        if (!collector.HadHit()) return false;

        collector.Sort();

        hits.reserve(collector.mHits.size());
        for (auto& result : collector.mHits)
            hits.push_back(result);

        return !hits.empty();
    }

    // Surekli kuvvet (ornegin itki) - her fizik adiminda cagir
    void add_force(JPH::BodyID body_id, JPH::Vec3 force)
    {
        physics_system->GetBodyInterface().AddForce(body_id, force);
    }

    // Ani impulse (ornegin patlama geri tepme)
    void add_impulse(JPH::BodyID body_id, JPH::Vec3 force)
    {
        physics_system->GetBodyInterface().AddImpulse(body_id, force);
    }

    // Hizi direkt set et
    void set_linear_velocity(JPH::BodyID body_id, JPH::Vec3 velocity)
    {
        physics_system->GetBodyInterface().SetLinearVelocity(body_id, velocity);
    }

    // Yerekimi carpani (ornegin tuy = 0.1, agir kaya = 2.0)
    void set_gravity_factor(JPH::BodyID body_id, float factor)
    {
        physics_system->GetBodyInterface().SetGravityFactor(body_id, factor);
    }

    void remove_body(JPH::BodyID body_id)
    {
        physics_system->GetBodyInterface().RemoveBody(body_id);
    }

    void delete_body(JPH::BodyID body_id)
    {
        remove_body(body_id);
        physics_system->GetBodyInterface().DestroyBody(body_id);
    }
};