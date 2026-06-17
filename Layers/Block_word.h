#include "Globals.h"
#include "Physics_manager.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

// Her layer ile carpisan basit filter siniflar
// (Bu Jolt versiyonunda BroadPhaseLayerFilterAll / ObjectLayerFilterAll yok)
class AllBroadPhaseFilter : public JPH::BroadPhaseLayerFilter
{
public:
    bool ShouldCollide(JPH::BroadPhaseLayer) const override { return true; }
};
class AllObjectLayerFilter : public JPH::ObjectLayerFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer) const override { return true; }
};

#include <unordered_map>
#include <glm/glm.hpp>

struct IVec3Hash
{
    size_t operator()(const glm::ivec3& v) const
    {
        size_t h = 0;
        h ^= std::hash<int>()(v.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>()(v.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>()(v.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

// BodyID icin hash
struct BodyIDHash
{
    size_t operator()(const JPH::BodyID& id) const
    {
        return std::hash<uint32_t>()(id.GetIndex());
    }
};

struct BodyIDEqual
{
    bool operator()(const JPH::BodyID& a, const JPH::BodyID& b) const
    {
        return a == b;
    }
};

enum class BlockType : uint8_t
{
    Air = 0,
    Dirt = 1,
};

template<typename BlockObjectType>
class BlockWorld
{
public:
    struct BlockEntry
    {
        BlockType        type = BlockType::Air;
        BlockObjectType* object = nullptr;
        JPH::BodyID      body_id = JPH::BodyID();
    };

private:
    std::unordered_map<glm::ivec3, BlockEntry, IVec3Hash>         m_blocks;

    // Ters harita: Jolt BodyID -> grid pozisyonu
    // Bu sayede raycast'te hit.mBodyID'den direkt blok pozisyonu bulunabilir
    std::unordered_map<JPH::BodyID, glm::ivec3, BodyIDHash, BodyIDEqual> m_body_to_grid;

    Physics_manager* m_physics = nullptr;
    entt::registry& m_registry;

    static constexpr float BLOCK_SIZE = 1.0f;
    static constexpr float BLOCK_HALF = 0.5f;

    glm::vec3 grid_to_world(const glm::ivec3& g) const
    {
        return glm::vec3(g.x + BLOCK_HALF, g.y + BLOCK_HALF, g.z + BLOCK_HALF);
    }

    glm::ivec3 world_to_grid(const glm::vec3& w) const
    {
        return glm::ivec3(glm::floor(w));
    }

public:
    BlockWorld(entt::registry& registry, Physics_manager* physics)
        : m_registry(registry), m_physics(physics)
    {
    }

    ~BlockWorld()
    {
        clear();
    }

    template<typename Factory>
    bool place_block(const glm::ivec3& pos, BlockType type, Factory object_factory)
    {
        if (m_blocks.count(pos)) return false;

        BlockEntry entry;
        entry.type = type;
        entry.object = object_factory(pos);

        if (m_physics)
        {
            JPH::BodyInterface* bi = m_physics->get_body_interface();
            JPH::BoxShapeSettings shape(JPH::Vec3(BLOCK_HALF, BLOCK_HALF, BLOCK_HALF));
            JPH::ShapeSettings::ShapeResult shape_result = shape.Create();

            glm::vec3 wp = grid_to_world(pos);
            JPH::BodyCreationSettings body_settings(
                shape_result.Get(),
                JPH::RVec3(wp.x, wp.y, wp.z),
                JPH::Quat::sIdentity(),
                JPH::EMotionType::Static,
                Object_layers::NON_MOVING
            );

            entry.body_id = bi->CreateAndAddBody(body_settings, JPH::EActivation::DontActivate);

            // Ters haritaya kaydet
            if (!entry.body_id.IsInvalid())
                m_body_to_grid[entry.body_id] = pos;
        }

        m_blocks[pos] = entry;
        return true;
    }

    bool remove_block(const glm::ivec3& pos)
    {
        auto it = m_blocks.find(pos);
        if (it == m_blocks.end()) return false;

        // Ters haritadan temizle
        if (!it->second.body_id.IsInvalid())
        {
            m_body_to_grid.erase(it->second.body_id);
            if (m_physics)
                m_physics->delete_body(it->second.body_id);
        }

        delete it->second.object;
        m_blocks.erase(it);
        return true;
    }

    bool has_block(const glm::ivec3& pos) const
    {
        auto it = m_blocks.find(pos);
        return it != m_blocks.end() && it->second.type != BlockType::Air;
    }

    BlockEntry* get_block(const glm::ivec3& pos)
    {
        auto it = m_blocks.find(pos);
        return (it != m_blocks.end()) ? &it->second : nullptr;
    }

    struct RayHit
    {
        glm::ivec3 block_pos;
        glm::ivec3 normal;
    };

    // -------------------------------------------------------------------
    // Raycast � Jolt'tan gelen BodyID ile ters haritadan blok bulunur.
    // Normal hesab�: �arpma noktas� ile blok merkezi aras�ndaki fark�n
    // dominant ekseni kullan�l�r; ama �nce y�zey epsilon'u uygulan�r.
    // -------------------------------------------------------------------
    std::optional<RayHit> raycast(const glm::vec3& origin, const glm::vec3& dir, float max_dist = 10.0f, JPH::BodyID skip_body_id = JPH::BodyID())
    {
        if (!m_physics) return std::nullopt;

        glm::vec3 norm_dir = glm::normalize(dir);

        JPH::RRayCast ray(
            JPH::RVec3(origin.x, origin.y, origin.z),
            JPH::Vec3(norm_dir.x * max_dist, norm_dir.y * max_dist, norm_dir.z * max_dist)
        );

        AllBroadPhaseFilter bp_filter;
        AllObjectLayerFilter ol_filter;

        // Tum hit'leri topla (en yakin once sirali)
        std::vector<JPH::RayCastResult> hits;
        if (!m_physics->cast_ray_all(ray, hits, bp_filter, ol_filter))
            return std::nullopt;

        // skip_body_id'yi atla, ilk blok body'sini bul
        JPH::RayCastResult best;
        best.mBodyID = JPH::BodyID();
        for (auto& h : hits)
        {
            if (!skip_body_id.IsInvalid() && h.mBodyID == skip_body_id)
                continue;
            if (m_body_to_grid.find(h.mBodyID) != m_body_to_grid.end())
            {
                best = h;
                break;
            }
        }

        if (best.mBodyID.IsInvalid())
            return std::nullopt;

        glm::ivec3 block_pos = m_body_to_grid.at(best.mBodyID);

        // Normal: carpma noktasi ile blok merkezinin dominant ekseni
        JPH::RVec3 jolt_hit = ray.GetPointOnRay(best.mFraction);
        glm::vec3  hp(jolt_hit.GetX(), jolt_hit.GetY(), jolt_hit.GetZ());
        glm::vec3  center = grid_to_world(block_pos);
        glm::vec3  diff = hp - center;

        glm::ivec3 normal(0);
        float ax = fabsf(diff.x), ay = fabsf(diff.y), az = fabsf(diff.z);
        if (ax >= ay && ax >= az)
            normal.x = (diff.x >= 0.0f) ? 1 : -1;
        else if (ay >= ax && ay >= az)
            normal.y = (diff.y >= 0.0f) ? 1 : -1;
        else
            normal.z = (diff.z >= 0.0f) ? 1 : -1;

        return RayHit{ block_pos, normal };
    }

    void clear()
    {
        for (auto& [pos, entry] : m_blocks)
        {
            if (m_physics && !entry.body_id.IsInvalid())
                m_physics->delete_body(entry.body_id);
            delete entry.object;
        }
        m_blocks.clear();
        m_body_to_grid.clear();
    }

    size_t block_count() const { return m_blocks.size(); }
};