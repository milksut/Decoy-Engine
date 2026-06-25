#pragma once
#include "Globals.h"
#include "The_event_manager.h"
#include "Events/Physics_events.h"
#include "game_object_basic.h"

#include "Components/Transform_components.h"
#include "Components/Rigidbody_component.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/ContactListener.h>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

//TODO: AI code - test and refactor -start--------------------------------------------------------------------------------------
#include <Jolt/Renderer/DebugRenderer.h>
#include "Shader.h"

#ifdef JPH_DEBUG_RENDERER

class Jolt_debug_renderer : public JPH::DebugRenderer
{
    struct Line_vertex
    {
        float x, y, z;
        float r, g, b, a;
    };

    std::vector<Line_vertex> m_lines;

    unsigned int m_VAO = 0, m_VBO = 0;
    Shader* m_shader = nullptr;

    // Jolt requires a concrete Batch type
    class Batch_impl : public JPH::RefTargetVirtual
    {
    public:
        void AddRef()  override { ++m_ref; }
        void Release() override { if (--m_ref == 0) delete this; }
    private:
        std::atomic<uint32_t> m_ref = 0;
    };

public:
    Jolt_debug_renderer(int camera_ubo_slot)
    {
        Initialize(); // must call before anything else

        m_shader = new Shader(
            "Shaders/Vertex_shaders/debug_line_vertex.vert",
            "Shaders/Fragment_shaders/basic_fragment.frag");
        m_shader->bind_UBO("projectionXview_block", camera_ubo_slot);

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Line_vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Line_vertex), (void*)(3 * sizeof(float)));

        glBindVertexArray(0);
    }

    ~Jolt_debug_renderer()
    {
        glDeleteBuffers(1, &m_VBO);
        glDeleteVertexArrays(1, &m_VAO);
        delete m_shader;
    }

    // ── collect lines each frame ──────────────────────────────────────────

    void DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to, JPH::ColorArg color) override
    {
        auto push = [&](JPH::RVec3Arg p)
            {
                m_lines.push_back({
                    (float)p.GetX(), (float)p.GetY(), (float)p.GetZ(),
                    color.r / 255.0f, color.g / 255.0f,
                    color.b / 255.0f, color.a / 255.0f });
            };
        push(from);
        push(to);
    }

    // ── flush to GPU and draw ─────────────────────────────────────────────

    void flush()
    {
        if (m_lines.empty()) return;

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER,
            m_lines.size() * sizeof(Line_vertex),
            m_lines.data(), GL_DYNAMIC_DRAW);

        m_shader->use();
        glBindVertexArray(m_VAO);
        glDrawArrays(GL_LINES, 0, (GLsizei)m_lines.size());
        glBindVertexArray(0);

        m_lines.clear();
    }

    // ── required no-ops ──────────────────────────────────────────────────

    void DrawTriangle(JPH::RVec3Arg, JPH::RVec3Arg, JPH::RVec3Arg,
        JPH::ColorArg, ECastShadow) override {
    }

    void DrawText3D(JPH::RVec3Arg, const std::string_view&,
        JPH::ColorArg, float) override {
    }

    Batch CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) override
    {
        return new Batch_impl();
    }

    Batch CreateTriangleBatch(const Vertex* inVertices, int inVertexCount,
        const JPH::uint32* inIndices, int inIndexCount) override
    {
        return new Batch_impl();
    }

    void DrawGeometry(JPH::RMat44Arg, const JPH::AABox&, float,
        JPH::ColorArg, const GeometryRef&, ECullMode, ECastShadow, EDrawMode) override {
    }
};

#endif // JPH_DEBUG_RENDERER
//TODO: AI code - test and refactor -end--------------------------------------------------------------------------------------_


//TODO:add param, logs

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

    Event_manager* event_manager = nullptr;
    entt::registry* registry = nullptr;


    Event_management::Event_receiver_shared event_reciver = Event_management::make_receiver([this](const Event_management::Event& e)
    {

        if (e.type == Event_management::Event_type::Physics_apply_force)
        {
            const auto& force_ev = static_cast<const Apply_force_event&>(e);
            add_force(force_ev.body_id, force_ev.force);
        }
        else if (e.type == Event_management::Event_type::Physics_apply_impulse)
        {
            const auto& impulse_ev = static_cast<const Apply_impulse_event&>(e);
            add_impulse(impulse_ev.body_id, impulse_ev.impulse);
        }
        else if(e.type == Event_management::Event_type::Physics_set_velocity)
        {
            const auto& velocity_ev = static_cast<const Set_velocity_event&>(e);
            set_linear_velocity(velocity_ev.body_id, velocity_ev.velocity);
        }
        else if(e.type == Event_management::Event_type::Physics_set_gravity)
        {
            const auto& garvity_ev = static_cast<const Set_gravity_event&>(e);
            set_gravity(garvity_ev.gravity);
        }
    });

    class Physics_contact_listener : public JPH::ContactListener
    {
        Event_manager* event_manager;
        entt::registry* registry;
        JPH::PhysicsSystem* physics_system;

        // Reads object id directly from body's UserData
        unsigned int entity_id_from_body(const JPH::Body& body) const
        {
            return static_cast<unsigned int>(body.GetUserData()); 
        }

        // For OnContactRemoved — only has BodyID, needs a lock to read body
        unsigned int entity_id_from_body_id(const JPH::BodyID id) const
        {
            // Use NoLock variant — safe to call from within Jolt callbacks
            // because Jolt guarantees the body is still alive during OnContactRemoved
            JPH::BodyLockRead lock(physics_system->GetBodyLockInterfaceNoLock(), id);
            if (!lock.Succeeded())
                return 0;
            return entity_id_from_body(lock.GetBody());
        }

        // Gets the event_reciver from a RigidBody_component, returns nullptr if missing
        RigidBody_component* get_rigid_body_comp(unsigned int object_id) const
        {
            if (object_id == 0)
                return nullptr;

            game_object_base* obj = Global_object_map::get_object(object_id);

            if (obj == nullptr)
                return nullptr;

            RigidBody_component* rb = obj->get_component<RigidBody_component>();

            return rb;
        }

        bool is_trigger_pair(unsigned int e1_id , unsigned int e2_id) const
        {
            RigidBody_component* e1_body = get_rigid_body_comp(e1_id);
            RigidBody_component* e2_body = get_rigid_body_comp(e2_id);

            bool e1_trigger = e1_body != nullptr ? e1_body->is_trigger : false;
            bool e2_trigger = e2_body != nullptr ? e2_body->is_trigger : false;

            return e1_trigger || e2_trigger;
        }

    public:
        Physics_contact_listener(Event_manager* em, entt::registry* reg, JPH::PhysicsSystem* ps)
            : event_manager(em), registry(reg), physics_system(ps)
        {}

        JPH::ValidateResult OnContactValidate(const JPH::Body&, const JPH::Body&, JPH::RVec3Arg, const JPH::CollideShapeResult&) override
        {
            return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
        }

        void OnContactAdded(const JPH::Body& b1, const JPH::Body& b2,
            const JPH::ContactManifold& manifold, JPH::ContactSettings&) override
        {
            unsigned int e1 = entity_id_from_body(b1);
            unsigned int e2 = entity_id_from_body(b2);
            bool trigger = is_trigger_pair(e1, e2);

            JPH::RVec3 jpt = manifold.GetWorldSpaceContactPointOn1(0);
            glm::vec3  pt(jpt.GetX(), jpt.GetY(), jpt.GetZ());

            RigidBody_component* e1_body = get_rigid_body_comp(e1);

            // Notify body A — "I hit B"
            if (e1_body != nullptr)
            {
                Event_management::Event_receiver_shared recv = e1_body->event_reciver;
                if (recv != nullptr)
                {
                    if (trigger)
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Trigger_enter_event>(e1, e2, pt,
                                Event_management::Event_timing::Queued, recv));
                    }
                    else
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Collision_begin_event>(e1, e2, pt,
                                Event_management::Event_timing::Queued, recv));
                    }
                        
                }
                else //Throw a annauncment event rather than a targeted one
                {
                    if (trigger)
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Trigger_enter_event>(e1, e2, pt,
                                Event_management::Event_timing::Queued));
                    }
                    else
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Collision_begin_event>(e1, e2, pt,
                                Event_management::Event_timing::Queued));
                    }
                }
            }
           

            // Notify body B — "I hit A"
            RigidBody_component* e2_body = get_rigid_body_comp(e2);
            if (e2_body != nullptr)
            {
                Event_management::Event_receiver_shared recv = e2_body->event_reciver;
                if (recv != nullptr)
                {
                    if (trigger)
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Trigger_enter_event>(e2, e1, pt,
                                Event_management::Event_timing::Queued, recv));
                    }
                    else
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Collision_begin_event>(e2, e1, pt,
                                Event_management::Event_timing::Queued, recv));
                    }

                }
                else //Throw a annauncment event rather than a targeted one
                {
                    if (trigger)
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Trigger_enter_event>(e2, e1, pt,
                                Event_management::Event_timing::Queued));
                    }
                    else
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Collision_begin_event>(e2, e1, pt,
                                Event_management::Event_timing::Queued));
                    }
                }
            }
        }

        void OnContactRemoved(const JPH::SubShapeIDPair& pair) override
        {
            JPH::BodyID b1 = pair.GetBody1ID();
            JPH::BodyID b2 = pair.GetBody2ID();

            unsigned int e1 = entity_id_from_body_id(b1);
            unsigned int e2 = entity_id_from_body_id(b2);
            bool trigger = is_trigger_pair(e1, e2);

            RigidBody_component* e1_body = get_rigid_body_comp(e1);

            // Notify body A — "I hit B"
            if (e1_body != nullptr)
            {
                Event_management::Event_receiver_shared recv = e1_body->event_reciver;
                if (recv != nullptr)
                {
                    if (trigger)
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Trigger_exit_event>(e1, e2,
                                Event_management::Event_timing::Queued, recv));
                    }
                    else
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Collision_end_event>(e1, e2,
                                Event_management::Event_timing::Queued, recv));
                    }

                }
                else //Throw a annauncment event rather than a targeted one
                {
                    if (trigger)
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Trigger_exit_event>(e1, e2,
                                Event_management::Event_timing::Queued));
                    }
                    else
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Collision_end_event>(e1, e2,
                                Event_management::Event_timing::Queued));
                    }
                }
            }


            // Notify body B — "I hit A"
            RigidBody_component* e2_body = get_rigid_body_comp(e2);
            if (e2_body != nullptr)
            {
                Event_management::Event_receiver_shared recv = e2_body->event_reciver;
                if (recv != nullptr)
                {
                    if (trigger)
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Trigger_exit_event>(e2, e1,
                                Event_management::Event_timing::Queued, recv));
                    }
                    else
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Collision_end_event>(e2, e1,
                                Event_management::Event_timing::Queued, recv));
                    }

                }
                else //Throw a annauncment event rather than a targeted one
                {
                    if (trigger)
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Trigger_exit_event>(e2, e1,
                                Event_management::Event_timing::Queued));
                    }
                    else
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Collision_end_event>(e2, e1,
                                Event_management::Event_timing::Queued));
                    }
                }
            }
        }

        void OnContactPersisted(const JPH::Body& b1, const JPH::Body& b2,
            const JPH::ContactManifold& manifold, JPH::ContactSettings&) override
        {
            unsigned int e1 = entity_id_from_body(b1);
            unsigned int e2 = entity_id_from_body(b2);
            bool trigger = is_trigger_pair(e1, e2);

            JPH::RVec3 jpt = manifold.GetWorldSpaceContactPointOn1(0);
            glm::vec3  pt(jpt.GetX(), jpt.GetY(), jpt.GetZ());

            RigidBody_component* e1_body = get_rigid_body_comp(e1);

            // Notify body A — "I hit B"
            if (e1_body != nullptr)
            {
                Event_management::Event_receiver_shared recv = e1_body->event_reciver;
                if (recv != nullptr)
                {
                    if (trigger)
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Trigger_persisted_event>(e1, e2, pt,
                                Event_management::Event_timing::Queued, recv));
                    }
                    else
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Collision_persisted_event>(e1, e2, pt,
                                Event_management::Event_timing::Queued, recv));
                    }

                }
                else //Throw a annauncment event rather than a targeted one
                {
                    if (trigger)
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Trigger_persisted_event>(e1, e2, pt,
                                Event_management::Event_timing::Queued));
                    }
                    else
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Collision_persisted_event>(e1, e2, pt,
                                Event_management::Event_timing::Queued));
                    }
                }
            }


            // Notify body B — "I hit A"
            RigidBody_component* e2_body = get_rigid_body_comp(e2);
            if (e2_body != nullptr)
            {
                Event_management::Event_receiver_shared recv = e2_body->event_reciver;
                if (recv != nullptr)
                {
                    if (trigger)
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Trigger_persisted_event>(e2, e1, pt,
                                Event_management::Event_timing::Queued, recv));
                    }
                    else
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Collision_persisted_event>(e2, e1, pt,
                                Event_management::Event_timing::Queued, recv));
                    }

                }
                else //Throw a annauncment event rather than a targeted one
                {
                    if (trigger)
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Trigger_persisted_event>(e2, e1, pt,
                                Event_management::Event_timing::Queued));
                    }
                    else
                    {
                        event_manager->throw_event(Physics_manager::CHANNEL_NAME,
                            std::make_unique<Collision_persisted_event>(e2, e1, pt,
                                Event_management::Event_timing::Queued));
                    }
                }
            }
        }
    };

    std::unique_ptr<Physics_contact_listener> contact_listener;

public:
    static constexpr const char* CHANNEL_NAME = "physics";

    void SyncPhysicsToECS()
    {
        if (physics_system == nullptr || registry == nullptr)
            return;

        auto view = registry->view<RigidBody_component, Transform_component, Id_component>();
        JPH::BodyInterface& bi = physics_system->GetBodyInterface();

        view.each([&](entt::entity /*entity*/, RigidBody_component& rb, Transform_component& tf, Id_component& id_comp)
        {
            /*if (!bi.IsActive(rb.body_id))
                return; // skip sleeping bodies*/

            JPH::RVec3 pos = bi.GetCenterOfMassPosition(rb.body_id);
            JPH::Quat  rot = bi.GetRotation(rb.body_id);

            // Convert JPH → GLM (Jolt uses right-hand Y-up, same as OpenGL)
            tf.position = glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
            tf.rotation_quat = glm::quat(rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ());

            //becouse euler returns radians ZYX
            tf.rotation = glm::degrees(glm::eulerAngles(tf.rotation_quat));
            std::swap(tf.rotation.x, tf.rotation.z);

            // tell the hierarchy the transform changed
            game_object_base* obj = Global_object_map::get_object(id_comp.id);
            if (obj != nullptr)
                obj->trigger_pos_changed_flags();

        });
    }

    Physics_manager(Event_manager* event_manager_in, entt::registry* registry_in)
        : event_manager(event_manager_in), registry(registry_in)
    {
        // must be FIRST
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        // Temp allocator: temp_allacator_size scratch per frame (stack-based, very fast)
        temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(temp_allocator_size);

        unsigned int hw = std::thread::hardware_concurrency();
        unsigned int worker_threads = 10; // default safe fallback
        if (hw > 1) worker_threads = hw - 1; // leave one thread for main loop

        // Job system: uses Jolt's built-in thread pool
        job_system = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, worker_threads);

        // ???
        physics_system = std::make_unique<JPH::PhysicsSystem>();
        physics_system->Init(Max_bodies, Num_body_mutexes, Max_body_pairs, Max_contact_constraints,
            Bp_layer_interface, O_vs_BP_filter, O_layer_filter);

        set_gravity(gravity);

        if(event_manager != nullptr)
        {
            event_manager->create_channel(CHANNEL_NAME);

            event_manager->subscribe(CHANNEL_NAME, Event_management::Event_type::Physics_apply_force, event_reciver);
            event_manager->subscribe(CHANNEL_NAME, Event_management::Event_type::Physics_apply_impulse, event_reciver);
            event_manager->subscribe(CHANNEL_NAME, Event_management::Event_type::Physics_set_velocity, event_reciver);
            event_manager->subscribe(CHANNEL_NAME, Event_management::Event_type::Physics_set_gravity, event_reciver);

            if(registry != nullptr)
            {
                contact_listener = std::make_unique<Physics_contact_listener>(event_manager, registry, physics_system.get());

                physics_system->SetContactListener(contact_listener.get());
            }
        }
    }

    ~Physics_manager()
    {
        physics_system->SetContactListener(nullptr);
        contact_listener.reset();

        physics_system.reset();
        job_system.reset();
        temp_allocator.reset();

        event_reciver.reset();

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

    // use this to crate bodys
    JPH::BodyInterface* get_body_interface()
    {
        if (physics_system == nullptr)
            return nullptr;

        return &(physics_system->GetBodyInterface());
    }

    //TODO: AI code - test and refactor - AI code start-------------------------------------------------------------
    // Base version — full control, you set everything in BodyCreationSettings yourself
    // Layer is auto-forced based on motion type so it can't mismatch
    JPH::BodyID create_body(unsigned int object_id, JPH::BodyCreationSettings settings,
        JPH::EActivation activation = JPH::EActivation::Activate)
    {
        if (physics_system == nullptr)
            return JPH::BodyID(); // invalid ID

        // force correct layer based on motion type — prevents layer/motiontype mismatch bugs
        if (settings.mMotionType == JPH::EMotionType::Static)
            settings.mObjectLayer = Object_layers::NON_MOVING;
        else
            settings.mObjectLayer = Object_layers::MOVING;

        JPH::BodyInterface& bi = physics_system->GetBodyInterface();

        JPH::Body* body = bi.CreateBody(settings);

        if (body == nullptr)
        {
            LOG_ERROR("Physics_manager: CreateBody failed — body limit reached? (Max_bodies = %u)", Max_bodies);
            return JPH::BodyID();
        }

        body->SetUserData(static_cast<uint64_t>(object_id));

        bi.AddBody(body->GetID(), activation);

        return body->GetID();
    }

    // Convenience — dynamic box
    JPH::BodyID create_dynamic_box(unsigned int object_id,
        JPH::Vec3 half_extents,
        JPH::RVec3 position,
        JPH::Quat rotation = JPH::Quat::sIdentity(),
        float friction = 0.5f,
        float restitution = 0.3f)
    {
        JPH::BodyCreationSettings settings(
            new JPH::BoxShape(half_extents),
            position, rotation,
            JPH::EMotionType::Dynamic,
            Object_layers::MOVING
        );
        settings.mFriction = friction;
        settings.mRestitution = restitution;

        return create_body(object_id, settings, JPH::EActivation::Activate);
    }

    // Convenience — dynamic sphere
    JPH::BodyID create_dynamic_sphere(unsigned int object_id,
        float radius,
        JPH::RVec3 position,
        JPH::Quat rotation = JPH::Quat::sIdentity(),
        float friction = 0.5f,
        float restitution = 0.3f)
    {
        JPH::BodyCreationSettings settings(
            new JPH::SphereShape(radius),
            position, rotation,
            JPH::EMotionType::Dynamic,
            Object_layers::MOVING
        );
        settings.mFriction = friction;
        settings.mRestitution = restitution;

        return create_body(object_id, settings, JPH::EActivation::Activate);
    }

    // Convenience — dynamic capsule (good default for characters/enemies)
    JPH::BodyID create_dynamic_capsule(unsigned int object_id,
        float half_height,
        float radius,
        JPH::RVec3 position,
        JPH::Quat rotation = JPH::Quat::sIdentity(),
        float friction = 0.5f,
        float restitution = 0.0f)
    {
        JPH::BodyCreationSettings settings(
            new JPH::CapsuleShape(half_height, radius),
            position, rotation,
            JPH::EMotionType::Dynamic,
            Object_layers::MOVING
        );
        settings.mFriction = friction;
        settings.mRestitution = restitution;

        return create_body(object_id, settings, JPH::EActivation::Activate);
    }

    // Convenience — static box (floors, walls, terrain pieces)
    JPH::BodyID create_static_box(unsigned int object_id,
        JPH::Vec3 half_extents,
        JPH::RVec3 position,
        JPH::Quat rotation = JPH::Quat::sIdentity(),
        float friction = 0.5f)
    {
        JPH::BodyCreationSettings settings(
            new JPH::BoxShape(half_extents),
            position, rotation,
            JPH::EMotionType::Static,
            Object_layers::NON_MOVING
        );
        settings.mFriction = friction;

        return create_body(object_id, settings, JPH::EActivation::DontActivate);
    }

    //TODO: AI code - test and refactor - AI code end--------------------------------------------------------------

    void Tick(float delta_time)
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

        SyncPhysicsToECS();
    }

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

    JPH::Vec3 get_linear_velocity(const JPH::BodyID body_id)
    {
        if (physics_system == nullptr) 
            return JPH::Vec3::sZero();
        return physics_system->GetBodyInterface().GetLinearVelocity(body_id);
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

#ifdef JPH_DEBUG_RENDERER
    void draw_debug(Jolt_debug_renderer& renderer)
    {
        JPH::BodyManager::DrawSettings settings;
        settings.mDrawShape = true;
        settings.mDrawShapeWireframe = true;
        settings.mDrawBoundingBox = false;

        physics_system->DrawBodies(settings, &renderer);
        renderer.flush();
    }
#endif // JPH_DEBUG_RENDERER
};
