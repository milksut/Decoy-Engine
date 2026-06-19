#pragma once

#include "Globals.h"

#include "game_object_basic_model.h"

#include "Components/Hierarchy_components.h"
#include "Components/Tag_components.h"
#include "Components/Transform_components.h"

class game_object_base;

namespace Global_object_map
{

	inline std::unordered_map<unsigned int, game_object_base*> object_list;

	void register_object(game_object_base* obj);
	void unregister_object(unsigned int id);
	game_object_base* get_object(unsigned int id);
}

//this class is for calling transform upload with each derived classes own static class region
class game_object_base
{
public:

	virtual void set_is_pos_changed_flag(bool val) = 0;
	virtual bool get_is_pos_changed_flag() = 0;
	virtual void set_is_any_child_pos_changed_flag(bool val) = 0;
	virtual bool get_is_any_child_pos_changed_flag() = 0;
	virtual void set_should_upload_flag(bool val) = 0;
	virtual bool get_should_upload_flag() = 0;

	virtual void trigger_child_pos_changed_flag() = 0;
	virtual void trigger_pos_changed_flags() = 0;

	virtual void tick_transforms(const glm::mat4) = 0;

	virtual unsigned int get_id() = 0;

	virtual void use_null_region_pos(const int null_region_index) = 0;
	virtual void swap_region_pos(const unsigned int other_object_id) = 0;


	virtual std::shared_ptr<class_region> get_active_region() const = 0;

	virtual int swap_region_index(int new_index) = 0;

	template <typename Component_type>
	Component_type* get_component() const
	{
		return registry.try_get<Component_type>(this_object);
	}

	/// <summary>
	///     Updates all root transforms and uploads final transformation data.
	/// </summary>
	/// <remarks>
	///     Iterates over entities without Parent_component, applies tick_transforms
	///     starting from identity matrix, then uploads all results to GPU or storage.
	/// </remarks>
	/// <param name="registry_in">[in] ECS registry containing scene entities.</param>
	void static Tick(entt::registry& registry_in)
	{
		auto view = registry_in.view<Transform_component, Id_component>(entt::exclude<Parent_component>);

		view.each([](auto /*entity*/, Transform_component& /*transform*/, Id_component& id_comp)
		{
			Global_object_map::get_object(id_comp.id)->tick_transforms(glm::mat4(1.0f));
		});

		upload_all_transforms();

	}

	/// <summary>
	///     Returns the current global ID counter value.
	/// </summary>
	/// <returns>Current value of the internal id counter.</returns>
	static unsigned int get_id_counter()
	{
		return id_counter;
	}

protected:

	entt::registry& registry;
	entt::entity this_object;

	/// <summary>
	///     Returns a static registry of callable functions.
	/// </summary>
	/// <remarks>
	///     The registry is stored as a static vector and persists for the lifetime of the program.
	///     Used to store deferred or global callbacks.
	/// </remarks>
	/// <returns>Reference to the function registry vector.</returns>
	static std::vector<std::function<void()>>& get_func_registry()
	{
		static std::vector<std::function<void()>> func_registry;
		return func_registry;
	}

	/// <summary>
	///     Registers a callback function to be executed during transform upload phase.
	/// </summary>
	/// <param name="fn">[in] Function to register and store in the global registry.</param>
	static void register_upload_transform(std::function<void()> fn)
	{
		get_func_registry().push_back(std::move(fn));
	}

	/// <summary>
	///     Executes all registered transform upload callbacks.
	/// </summary>
	/// <remarks>
	///     Iterates through the global function registry and calls each stored function.
	///     Typically used to push transform data to GPU or render pipeline.
	/// </remarks>
	static void upload_all_transforms()
	{
		for (auto& fn : get_func_registry())
			fn();
	}

	/// <summary>
	///     Generates a unique incremental ID.
	/// </summary>
	/// <remarks>
	///     Increments the internal counter after returning its current value.
	/// </remarks>
	/// <returns>New unique ID.</returns>
	static unsigned int generate_id()
	{
		return id_counter++;
	}

	game_object_base(entt::registry& registry_in)
		: registry(registry_in) {}

private:

	static inline unsigned int id_counter = 1;
};

template <typename Derived>
class game_object_basic : public game_object_base
{
private:
	struct auto_register
	{
		auto_register()
		{
			game_object_base::register_upload_transform([]() { Derived::uplad_transform_from_region(Derived::region, true); });
		}
	};

	static inline auto_register registerer{};

	static inline game_object_basic_model* model = nullptr;
	static inline std::shared_ptr<class_region> region = nullptr;
	static inline int transform_attrib_index = -1;   // VAO attrib slot for world mat4 (e.g. 3)
	static inline int transpose_inverse_transform_attrib_index = -1; // VAO attrib slot for transpose inverse world mat4 (e.g. 7)


	/// <summary>
	///     Uploads pending transform data to GPU in batched ranges.
	/// </summary>
	/// <remarks>
	///     Iterates over region objects and groups consecutive valid transforms into batches
	///     to minimize GPU buffer updates. Uploads both world matrices (mat4) and optional
	///     inverse-transpose matrices (mat3).
	/// </remarks>
	/// <param name="model">Model containing instance buffers.</param>
	/// <param name="region">Instance region holding object pointers.</param>
	/// <param name="transform_attrib_index">Attribute index for world matrices.</param>
	/// <param name="transpose_inverse_transform_attrib_index">
	/// Optional attribute index for inverse-transpose matrices (-1 if disabled).
	/// </param>
	/// <returns>void</returns>
	static void uplad_transform_from_region(std::shared_ptr<class_region> uploading_region, bool use_static_max_upload_index = true)
	{
		if (!model || !uploading_region || transform_attrib_index < 0)
			return;

		const int region_size = static_cast<int>(uploading_region->object_ptrs.size());

		std::vector<glm::mat4> staging;
		staging.reserve(region_size); // avoid reallocations mid-loop

		std::vector<glm::mat3> staging_inverse;
		staging_inverse.reserve(region_size); // avoid reallocations mid-loop

		int batch_start = -1; // index in object_ptrs where current batch began

		auto flush_batch = [&]()
			{
				if (batch_start == -1 || staging.empty())
					return;

				model->load_instance_buffer(
					reinterpret_cast<float*>(staging.data()),  // mat4 data
					static_cast<unsigned int>(staging.size()), // number of mat4s
					transform_attrib_index,
					uploading_region,
					static_cast<unsigned int>(batch_start)           // offset within region
				);

				staging.clear();

				if (transpose_inverse_transform_attrib_index >= 0)
				{
					model->load_instance_buffer(
						reinterpret_cast<float*>(staging_inverse.data()),
						static_cast<unsigned int>(staging_inverse.size()),
						transpose_inverse_transform_attrib_index,
						uploading_region,
						static_cast<unsigned int>(batch_start)
					);
					staging_inverse.clear();
				}

				batch_start = -1;

			};

		int max_region_upload_index_in = use_static_max_upload_index ? max_region_upload_index : region_size;

		for (int i = 0; i < region_size && i < max_region_upload_index_in; i++)
		{
			void* ptr = uploading_region->object_ptrs[i];

			if (ptr == nullptr)
			{
				flush_batch();
				continue;
			}

			// Cast void* back to game_object_basic — safe because only
			// game_object_basic instances ever write into object_ptrs
			game_object_basic* obj = static_cast<game_object_basic*>(ptr);

			if (!obj->should_upload_flag)
			{
				flush_batch();
				continue;
			}

			Transform_component& tc = obj->get_transform_ref();

			if (batch_start == -1)
				batch_start = i; // start a new batch here

			staging.push_back(tc.world);

			if (transpose_inverse_transform_attrib_index >= 0)
				staging_inverse.push_back(glm::transpose(glm::inverse(glm::mat3(tc.world))));

			obj->should_upload_flag = false; // reset flag after queuing for upload
		}

		flush_batch(); // flush any trailing batch
	}

	//-----------------------------------------------------------------------------------------

	//used when we want a spesfic object to have its own class region rather than sharing with other objects of the same class,
	//for example when we want to use asynch animations, we give each object its own region so that they can draw at differnt interwals rather than with instanced drawing
	std::shared_ptr<class_region> special_region = nullptr;

	int region_slot_index = -1; // this objects slot in region->object_ptrs

	/// <summary>
	///     Swaps the object's index inside its instance region.
	/// </summary>
	/// <remarks>
	///     Updates the region slot mapping, clears the old slot, assigns the new slot,
	///     and marks the object for transform upload.
	/// </remarks>
	/// <param name="new_index">[in] New index inside the region.</param>
	/// <returns>Previous region slot index, or -1 if region is null.</returns>
	int swap_region_index(const int new_index) override
	{
		// pick active region — special takes priority
		std::shared_ptr<class_region>& active = special_region ? special_region : region;

		if (active == nullptr)
		{
			LOG_ERROR("Game_object_basic : This object dont have a region! cant swap index");
			return -1;
		}

		int temp = region_slot_index;
		region_slot_index = new_index;

		if (temp >= 0 && temp < active->object_ptrs.size())
		{
			active->object_ptrs[temp] = nullptr;
		}

		if (region_slot_index >= 0 && region_slot_index < active->object_ptrs.size())
		{

			active->object_ptrs[region_slot_index] = this;
		}

		set_should_upload_flag(true);
		return temp;
	}

	//TODO: retire this function to use more general get_component func (derived from base classs)
	/// <summary>
	///     Returns a reference to the entity's Transform_component.
	/// </summary>
	/// <remarks>
	///     Directly queries the ECS registry using the stored entity handle.
	/// </remarks>
	/// <returns>Reference to Transform_component.</returns>
	Transform_component& get_transform_ref() const
	{
		return registry.get<Transform_component>(this_object);
	}

	/// <summary>
	///     Returns a pointer to the object's World_AABB_component.
	/// </summary>
	/// <remarks>
	///     Queries the ECS registry; returns nullptr if the component does not exist.
	/// </remarks>
	/// <returns>Pointer to World_AABB_component or nullptr.</returns>
	World_AABB_component* get_world_aabb_ref() const
	{
		return registry.try_get<World_AABB_component>(this_object);
	}

	/// <summary>
	///     Converts a raw region pointer back into a game object pointer.
	/// </summary>
	/// <remarks>
	///     Assumes the pointer originally points to a game_object_basic instance.
	///     Unsafe if used with invalid or mismatched types.
	/// </remarks>
	/// <param name="ptr">[in] Raw pointer stored in region.</param>
	/// <returns>Pointer to game_object_basic instance.</returns>
	static game_object_basic* from_region_ptr(void* ptr)
	{
		return static_cast<game_object_basic<Derived>*>(ptr);
	}

protected:

	bool should_upload_flag = false;
	bool is_pos_changed_flag = false;
	bool is_any_child_pos_changed_flag = false;

	unsigned int id;

	//TODO: renamke this to prevent miss use
	/// <summary>
	///     Propagates a "child position changed" flag up the parent hierarchy.
	/// </summary>
	/// <remarks>
	///     Marks the current object as having a changed child transform, then recursively
	///     notifies the parent (if it exists) unless it is already marked.
	/// </remarks>
	void trigger_child_pos_changed_flag() override
	{
		is_any_child_pos_changed_flag = true;
		Parent_component* comp = registry.try_get<Parent_component>(this_object);

		if (comp != nullptr)
		{
			if (Global_object_map::get_object(comp->parent_id)->get_is_any_child_pos_changed_flag())
				return;
			else
				Global_object_map::get_object(comp->parent_id)->trigger_child_pos_changed_flag();
		}
	}

	/// <summary>
	///     Marks the object as having a position/transform change.
	/// </summary>
	/// <remarks>
	///     Sets internal dirty flags and propagates the change to parent objects.
	/// </remarks>
	void trigger_pos_changed_flags() override
	{
		should_upload_flag = true;
		is_pos_changed_flag = true;
		trigger_child_pos_changed_flag();
	}

	/// <summary>
	///     Sets the internal position-changed flag.
	/// </summary>
	/// <param name="val">[in] New flag value.</param>
	void set_is_pos_changed_flag(bool val) override
	{
		is_pos_changed_flag = val;
	}

	/// <summary>
	///     Returns whether the object's position/transform has changed.
	/// </summary>
	/// <returns>True if position changed flag is set.</returns>
	bool get_is_pos_changed_flag() override
	{
		return is_pos_changed_flag;
	}

	/// <summary>
	///     Sets the flag indicating that any child transform has changed.
	/// </summary>
	/// <param name="val">[in] New flag value.</param>
	void set_is_any_child_pos_changed_flag(bool val) override
	{
		is_any_child_pos_changed_flag = val;
	}

	/// <summary>
	///     Returns whether any child object's position/transform has changed.
	/// </summary>
	/// <returns>True if a child transform change flag is set.</returns>
	bool get_is_any_child_pos_changed_flag() override
	{
		return is_any_child_pos_changed_flag;
	}

	/// <summary>
	///     Sets the flag that indicates whether this object should upload its transform data.
	/// </summary>
	/// <param name="val">[in] New flag value.</param>
	void set_should_upload_flag(bool val) override
	{
		should_upload_flag = val;
	}

	/// <summary>
	///     Returns whether this object is marked for transform upload.
	/// </summary>
	/// <returns>True if the object should upload its data.</returns>
	bool get_should_upload_flag() override
	{
		return should_upload_flag;
	}

	/// <summary>
	///     Updates world transforms recursively through the scene hierarchy.
	/// </summary>
	/// <remarks>
	///     Computes local-to-world matrices when needed and propagates transform changes
	///     to child objects using dirty-flag optimization.
	/// </remarks>
	/// <param name="parent_transform">[in] Parent world transform matrix.</param>
	void tick_transforms(const glm::mat4 parent_transform) override
	{
		Transform_component& this_transform = get_transform_ref();

		bool temp_trigger_child_flag = is_pos_changed_flag;

		if (is_pos_changed_flag)
		{
			glm::mat4 local = glm::mat4(1.0f);
			// Local transform
			if (this_transform.use_quat)
			{
				// Direct quaternion path — no angle extraction, no gimbal lock
				local = glm::translate(glm::mat4(1.0f), this_transform.position)
					* glm::mat4_cast(this_transform.rotation_quat)
					* glm::scale(glm::mat4(1.0f), this_transform.scale);
			}
			else
			{
				local = glm::translate(local, this_transform.position);
				local = glm::rotate(local, glm::radians(this_transform.rotation.x), glm::vec3(1, 0, 0));
				local = glm::rotate(local, glm::radians(this_transform.rotation.y), glm::vec3(0, 1, 0));
				local = glm::rotate(local, glm::radians(this_transform.rotation.z), glm::vec3(0, 0, 1));
				local = glm::scale(local, this_transform.scale);
			}

			// World Transform
			this_transform.world = parent_transform * local;

			is_pos_changed_flag = false;

			World_AABB_component* aabb = get_world_aabb_ref();
			if (aabb)
			{
				aabb->aabb = game_object_basic_model::get_world_aabb(model->model_aabb, get_transform_ref().world);
			}
			else if (model)
			{
				registry.emplace<World_AABB_component>(this_object,
					game_object_basic_model::get_world_aabb(model->model_aabb, get_transform_ref().world));
			}

		}

		Child_component* childs = registry.try_get<Child_component>(this_object);


		if (childs != nullptr)
		{
			for (unsigned int child_id : childs->children_ids)
			{
				game_object_base* child = Global_object_map::get_object(child_id);

				if (temp_trigger_child_flag)
					child->trigger_pos_changed_flags();

				else if (!child->get_is_any_child_pos_changed_flag())
					continue;

				child->tick_transforms(this_transform.world);
			}
		}

		set_is_any_child_pos_changed_flag(false);
	}


	/// <summary>
	///     Constructs a game object and registers it in the ECS and global object map.
	/// </summary>
	/// <remarks>
	///     Creates an entity, assigns ID, tag, transform, optional parent-child hierarchy,
	///     optionally assigns a special region, and places the object into a region slot if available.
	///     Finally marks the object as needing a transform update.
	/// </remarks>
	/// <param name="registry_in">[in] ECS registry reference.</param>
	/// <param name="tag">[in] Debug/tag name of the object.</param>
	/// <param name="parent_object">[in] Optional parent object for hierarchy.</param>
	/// <param name="special_region">[in] Optional custom region override for the object.</param>
	/// <param name="transform">[in] Initial transform component.</param>
	game_object_basic(entt::registry& registry_in, const std::string& tag = "Undefined tag", game_object_basic* parent_object = nullptr,
		std::shared_ptr<class_region> special_region = nullptr, Transform_component transform = Transform_component())
		: game_object_base(registry_in), special_region(special_region)
	{
		this_object = registry.create();

		id = generate_id();

		Global_object_map::register_object(this);

		registry.emplace<Id_component>(this_object, id);

		registry.emplace<Tag_component>(this_object, tag);

		registry.emplace<Transform_component>(this_object, transform);

		if (parent_object != nullptr)
		{
			registry.emplace<Parent_component>(this_object, parent_object->get_id());
			registry.get_or_emplace<Child_component>(parent_object->this_object).children_ids.push_back(id);
		}

		try_assign_region_slot();

		trigger_pos_changed_flags();
	};

	~game_object_basic()
	{
		std::shared_ptr<class_region>& active = special_region ? special_region : region;

		if (active && region_slot_index >= 0)
		{
			int last_slot = (int)active->object_ptrs.size() - 1;
			while (last_slot >= 0 && active->object_ptrs[last_slot] == nullptr)
				last_slot--;

			if (last_slot >= 0 && last_slot != region_slot_index)
			{
				static_cast<game_object_base*>(active->object_ptrs[last_slot])->swap_region_pos(id);
			}

			active->object_ptrs[region_slot_index] = nullptr;
		}


		Global_object_map::unregister_object(id);

		if (registry.valid(this_object))
			registry.destroy(this_object);
	}

public:

	static inline int max_region_upload_index = std::numeric_limits<int>::max();// up to what point in the region we should try to upload transforms

	/// <summary>
	///     Attempts to assign the object to an available slot in its active region.
	/// </summary>
	/// <remarks>
	///     Uses special_region if available, otherwise falls back to the default region.
	///     Searches for an empty slot, or expands the region if allowed.
	/// </remarks>
	/// <returns>
	///     Assigned region slot index, or -1 if assignment fails.
	/// </returns>
	int try_assign_region_slot()
	{

		std::shared_ptr<class_region>& active = special_region ? special_region : region;

		if (active != nullptr)
		{
			if (region_slot_index >= 0 && region_slot_index <= (int)active->size_in_number)
			{
				return region_slot_index;
			}

			for (int i = 0; i < active->object_ptrs.size(); ++i)
			{
				if (active->object_ptrs[i] == nullptr)
				{
					active->object_ptrs[i] = this;
					region_slot_index = i;
					return region_slot_index;
				}
			}

			if (region_slot_index < 0)
			{
				if (active->object_ptrs.size() < active->size_in_number)
				{
					active->object_ptrs.push_back(this);
					region_slot_index = (int)active->object_ptrs.size() - 1;
				}
				else
				{
					//This is now intended use, you can have objects that dont be drawn
					//LOG_ERROR("Game_object_basic, There is not enough space in region, object Cant be Created.");
				}
			}

			return region_slot_index;
		}
		else
			return -1;
	}

	/// <summary>
	///     Returns the object's slot index inside its class region.
	/// </summary>
	/// <returns>Region slot index.</returns>
	int get_region_slot_index() const
	{
		return region_slot_index;
	}

	/// <summary>
	///     Moves the object to a different slot inside its class region.
	/// </summary>
	/// <param name="new_slot">[in] Target region slot index.</param>
	void move_to_slot(int new_slot)
	{
		swap_region_index(new_slot);
		is_pos_changed_flag = true;
		is_any_child_pos_changed_flag = true;
		should_upload_flag = true;
	}

	/// <summary>
	///     Returns the unique ID of the game object.
	/// </summary>
	/// <returns>Object ID.</returns>
	unsigned int get_id() override
	{
		return id;
	}

	//TODO: retire this function to use more general get_component_copy func (derived from base class)
	/// <summary>
	///     Returns a copy of the object's Transform_component.
	/// </summary>
	/// <returns>Copy of the current transform.</returns>
	Transform_component get_transform_copy() const
	{
		return get_transform_ref();
	}

	/// <summary>
	///     Returns a copy of the object's world AABB (Axis-Aligned Bounding Box).
	/// </summary>
	/// <remarks>
	///     If no AABB exists, returns a default-constructed component.
	/// </remarks>
	/// <returns>Copy of World_AABB_component.</returns>
	World_AABB_component get_world_aabb_copy() const
	{
		World_AABB_component* aabb = get_world_aabb_ref();
		if (aabb)
			return *aabb;
		else
			return World_AABB_component();
	}

	/// <summary>
	///     Moves the object into a specified empty slot within its region.
	/// </summary>
	/// <remarks>
	///     Validates region existence, bounds, and slot availability before swapping indices.
	/// </remarks>
	/// <param name="null_region_index">[in] Target empty slot index in the region.</param>
	void use_null_region_pos(const int null_region_index) override
	{
		// pick active region — special takes priority
		std::shared_ptr<class_region>& active = special_region ? special_region : region;

		if (active == nullptr)
		{
			LOG_ERROR("Game_object_basic: Cannot use null region pos, region is not initialized!");
			return;
		}
		else if (null_region_index < 0 || null_region_index >= static_cast<int>(active->object_ptrs.size()))
		{
			LOG_ERROR("Game_object_basic: Null region index %d is out of bounds (valid range: 0 to %d).",
				null_region_index, static_cast<int>(active->object_ptrs.size()) - 1);
			return;
		}
		else if (active->object_ptrs[null_region_index] != nullptr)
		{
			LOG_ERROR("Game_object_basic: Slot %d is not empty, cannot assign object here!", null_region_index);
			return;
		}

		swap_region_index(null_region_index);
	}

	/// <summary>
	///     Swaps this object's region slot position with another object.
	/// </summary>
	/// <remarks>
	///     Exchanges indices between two objects within the same region.
	/// </remarks>
	/// <param name="other_object_id">[in] ID of the object to swap region positions with.</param>
	void swap_region_pos(const unsigned int other_object_id) override
	{
		if (id == other_object_id)
		{
			LOG_DEBUG("Game_object_basic : No need to swap pos with it self at id %d", id);
			return;
		}

		game_object_base* ptr = Global_object_map::get_object(other_object_id);

		if (ptr == nullptr)
		{
			LOG_ERROR("Game_object_basic :There is no Object with id %d to swap with!", id);
			return;
		}

		if (ptr->get_active_region() != get_active_region())
		{
			LOG_ERROR("Game_object_basic : Objects with id %d and %d do not share the same region, cannot swap!", id, other_object_id);
			return;
		}

		int temp_index_holder = region_slot_index;

		swap_region_index(ptr->swap_region_index(temp_index_holder));

	}

	/// <summary>
	///     Returns the class region associated with the object.
	/// </summary>
	/// <returns>Shared pointer to the current class_region.</returns>
	static std::shared_ptr<class_region> get_class_region()
	{
		return region;
	}

	/// <summary>
	///     Returns the currently active region of the object.
	/// </summary>
	/// <remarks>
	///     Prefers the special region if it exists, otherwise returns the class region.
	/// </remarks>
	/// <returns>Shared pointer to the active region.</returns>
	std::shared_ptr<class_region> get_active_region() const override
	{
		return special_region ? special_region : region;
	}

	//---set transforms-----------------------------------------------------------------------

	/// <summary>
	///     Sets the object's world position and marks it for transform update.
	/// </summary>
	/// <param name="new_pos">[in] New position in world space.</param>
	void set_position(const glm::vec3& new_pos)
	{
		get_transform_ref().position = new_pos;
		trigger_pos_changed_flags();
	}

	/// <summary>
	///     Sets the object's rotation and marks it for transform update.
	/// </summary>
	/// <param name="new_rot">[in] New rotation (Euler angles in degrees).</param>
	void set_rotation_euler(const glm::vec3& new_rot)
	{
		get_transform_ref().rotation = new_rot;
		trigger_pos_changed_flags();
	}

	/// <summary>
	///     Sets the object's scale and marks it for transform update.
	/// </summary>
	/// <param name="new_scale">[in] New scale vector.</param>
	void set_scale(const glm::vec3& new_scale)
	{
		get_transform_ref().scale = new_scale;
		trigger_pos_changed_flags();
	}

	/// <summary>
	///     Sets the object's rotation using a quaternion.
	/// </summary>
	/// <param name="new_rot">[in] New rotation quaternion.</param>
	void set_rotation_quat(const glm::quat& new_rot)
	{
		get_transform_ref().rotation_quat = new_rot;
		trigger_pos_changed_flags();
	}

	/// <summary>
	///     Enables or disables quaternion-based rotation for the transform.
	/// </summary>
	/// <param name="is_using_quat">[in] True to enable quaternion rotation, false for Euler.</param>
	void set_is_using_quat(const bool is_using_quat)
	{
		get_transform_ref().use_quat = is_using_quat;
		trigger_pos_changed_flags();
	}

	//---update transforms------------------------------------------------------------------------

	/// <summary>
	///     Moves the object by a delta offset and marks it for transform update.
	/// </summary>
	/// <param name="delta_pos">[in] Position offset to add to current position.</param>
	void move(const glm::vec3& delta_pos)
	{
		get_transform_ref().position += delta_pos;
		trigger_pos_changed_flags();
	}

	/// <summary>
	///     Rotates the object by a delta Euler angle and marks it for transform update.
	/// </summary>
	/// <param name="delta_rot">[in] Rotation offset (Euler angles in degrees).</param>
	void rotate(const glm::vec3& delta_rot)
	{
		get_transform_ref().rotation += delta_rot;
		trigger_pos_changed_flags();
	}

	/// <summary>
	///     Scales the object by multiplying its current scale and marks it for transform update.
	/// </summary>
	/// <param name="delta_scale">[in] Scale multiplier applied to current scale.</param>
	void scale(const glm::vec3& delta_scale)
	{
		get_transform_ref().scale *= delta_scale;
		trigger_pos_changed_flags();
	}

	//TODO: add rotare in quats

	/// <summary>
	///     Applies a transformation matrix to the object's current transform.
	/// </summary>
	/// <remarks>
	///     Multiplies the current transform by the given matrix and marks the object as changed.
	/// </remarks>
	/// <param name="transform_matrix">[in] Transformation matrix to apply.</param>
	void apply_transform(const glm::mat4& transform_matrix)
	{
		get_transform_ref() *= transform_matrix;
		trigger_pos_changed_flags();
	}

	//---get transforms------------------------------------------------------------------------

	/// <summary>
	///     Returns the object's current position.
	/// </summary>
	/// <returns>World position vector.</returns>
	glm::vec3 get_position() const
	{
		return get_transform_ref().position;
	}

	/// <summary>
	///     Returns the object's current rotation.
	/// </summary>
	/// <returns>Euler rotation (in degrees).</returns>
	glm::vec3 get_rotation_euler() const
	{
		return get_transform_ref().rotation;
	}

	/// <summary>
	///     Returns the object's current scale.
	/// </summary>
	/// <returns>Scale vector.</returns>
	glm::vec3 get_scale() const
	{
		return get_transform_ref().scale;
	}

	/// <summary>
	///     Returns the object's rotation as a quaternion.
	/// </summary>
	/// <returns>Current rotation quaternion.</returns>
	glm::quat get_rotation_quat() const
	{
		return get_transform_ref().rotation_quat;
	}

	/// <summary>
	///     Returns whether the object uses quaternion-based rotation.
	/// </summary>
	/// <returns>True if quaternion rotation mode is enabled.</returns>
	bool get_is_using_quat() const
	{
		return get_transform_ref().use_quat;
	}

	//--- upload/change model--------------------------------------------------------------------

	/// <summary>
	///     Sets the active model and initializes its instance region.
	/// </summary>
	/// <remarks>
	///     Reserves a class region for instance data and configures transform attribute indices.
	///     If model is null, disables all transform-related attributes.
	/// </remarks>
	/// <param name="new_model">[in] Model to set as active.</param>
	/// <param name="region_size">[in] Number of instance slots to reserve.</param>
	/// <param name="transform_attrib_index_in">[in] Attribute index for world transform matrix.</param>
	/// <param name="tranpose_inverse_transform_attrib_index_in">
	/// [in] If -1 or smaller, disables uploading of transpose-inverse transform.
	/// </param>
	static void set_model(game_object_basic_model* new_model, const unsigned int region_size,
		int transform_attrib_index_in = MODEL_ATRIB_LAST_INDEX + 1, int tranpose_inverse_transform_attrib_index_in = MODEL_ATRIB_LAST_INDEX + 5)
	{
		(void)registerer;

		model = new_model;

		if (new_model != nullptr)
		{
			region = model->reserve_class_region(region_size);
			region->object_ptrs.assign(region_size, nullptr);
			transform_attrib_index = transform_attrib_index_in;
			transpose_inverse_transform_attrib_index = tranpose_inverse_transform_attrib_index_in;

		}
		else
		{
			region = nullptr;
			transform_attrib_index = -1;
			transpose_inverse_transform_attrib_index = -1;
		}
	}

	/// <summary>
	///     Expands or shrinks the object's instance region size.
	/// </summary>
	/// <remarks>
	///     Can be called with a negative value to reduce region size, but never below zero.
	///     Updates the model's instance buffer layout accordingly.
	/// </remarks>
	/// <param name="additional_size">[in] Amount to add (or subtract if negative) from current region size.</param>
	/// <returns>New region size, or -1 on failure.</returns>
	static int expand_region(const int additional_size)
	{
		if (model == nullptr || region == nullptr)
		{
			LOG_WARNING("Game_object_basic: cant expand region, there is no defined model or region!");
			return -1;
		}
		if (additional_size + region->size_in_number < 0)
		{
			LOG_WARNING("Game_object_basic: cant shrink region below 0!");
			return -1;
		}

		model->reserve_additional_region(additional_size + region->size_in_number, region);
		return additional_size + region->size_in_number;
	}

	//--- draw -----------------------------------------------------------------------------------

	/// <summary>
	///     Draws the object using its assigned model and instance region.
	/// </summary>
	/// <remarks>
	///     Uses the provided drawing region if available, otherwise uses the object's default region.
	///     If amount is 0, draws the full region. If amount exceeds region size,
	///     it is clamped to the region size.
	/// </remarks>
	/// <param name="shader">[in] Shader used for rendering.</param>
	/// <param name="amount">[in] Number of instances to draw (0 = full region).</param>
	/// <param name="drawing_region">[in] Optional region to draw from.</param>
	/// <returns>0 on success, -1 if model is not set.</returns>
	static int draw(Shader& shader, unsigned int amount = 0, std::shared_ptr<class_region> drawing_region = nullptr)
	{
		if (model == nullptr)
		{
			LOG_WARNING("Game_object_basic: cant draw, there is no defined model!");
			return -1;
		}

		if (!drawing_region)
			drawing_region = region;

		if (amount <= 0)
			amount = drawing_region->size_in_number;

		else if (amount > drawing_region->size_in_number)
		{
			LOG_WARNING("Game_object_basic: draw amount is bigger than region size, drawing only region size!");
			amount = drawing_region->size_in_number;
		}

		model->draw(shader, drawing_region, amount);
		return 0;
	}

	//special region functions, this is for objects that want to have their own region rather than sharing with other objects of the same class
	//dont forget this stops them from drawn with basic call, you need to call it with special region as param to draw them

	/// <summary>
	///     Creates and assigns a private special region for the object.
	/// </summary>
	/// <remarks>
	///     Removes the object from its current class region, allocates a new region,
	///     assigns the first slot to the object, and registers transform upload callback.
	/// </remarks>
	/// <param name="new_region_size">[in] Size of the special region to create.</param>
	/// <returns>Created special region, or nullptr if creation fails.</returns>
	std::shared_ptr<class_region> use_special_region(const unsigned int new_region_size = 1)
	{
		if (special_region)
		{
			LOG_WARNING("Game_object_basic: object already has a special region.");
			return special_region;
		}
		if (model == nullptr)
		{
			LOG_ERROR("Game_object_basic: cannot create special region — no model set.");
			return nullptr;
		}
		if (new_region_size <= 0)
		{
			LOG_ERROR("Game_object_basic: cannot create special region - region size cant be 0");
			return nullptr;
		}

		// Remove from class region first
		if (region && region_slot_index >= 0 && region_slot_index < (int)region->object_ptrs.size())
		{
			region->object_ptrs[region_slot_index] = nullptr;
			region_slot_index = -1;
		}

		// Allocate a private region
		special_region = model->reserve_class_region(new_region_size);
		special_region->object_ptrs.assign(new_region_size, nullptr);
		special_region->object_ptrs[0] = this;
		region_slot_index = 0;

		game_object_base::register_upload_transform([]() { Derived::uplad_transform_from_region(special_region, false); });

		set_should_upload_flag(true);
		return special_region;
	}

	/// <summary>
	///     Assigns a special region to the object and updates its region slot.
	/// </summary>
	/// <remarks>
	///     Removes the object from its current class region, assigns the new special region,
	///     attempts to reserve a slot, and marks the object for transform upload.
	/// </remarks>
	/// <param name="new_region">[in] New special region to assign.</param>
	void use_special_region(std::shared_ptr<class_region> new_region)
	{
		if (!new_region)
		{
			LOG_ERROR("Game_object_basic: use_special_region called with null region.");
			return;
		}

		// Leave class region
		if (region && region_slot_index >= 0 && region_slot_index < (int)region->object_ptrs.size())
		{
			region->object_ptrs[region_slot_index] = nullptr;
			region_slot_index = -1;
		}

		special_region = new_region;

		try_assign_region_slot();

		set_should_upload_flag(true);
	}
};

namespace Global_object_map
{

	/// <summary>
	///     Registers a game object in the global object map.
	/// </summary>
	/// <remarks>
	///     Stores the object pointer using its unique ID as the key.
	///     ID 0 is considered invalid and will be rejected.
	/// </remarks>
	/// <param name="obj">[in] Pointer to the object to register.</param>
	inline void register_object(game_object_base* obj)
	{
		unsigned int id = obj->get_id();

		if (id == 0)
		{
			LOG_ERROR("Global_object_map: Invalid id 0, no object can have this id!");
			return;
		}
		else
		{
			//LOG_DEBUG("Global_object_map: Registering object with id %u", id);
		}

		object_list[id] = obj;
	}

	/// <summary>
	///     Removes a game object from the global object map.
	/// </summary>
	/// <param name="id">[in] ID of the object to unregister.</param>
	inline void unregister_object(unsigned int id)
	{
		object_list.erase(id);
	}

	/// <summary>
	///     Retrieves a game object from the global object map by ID.
	/// </summary>
	/// <remarks>
	///     Returns nullptr if the ID is invalid or the object is not found.
	/// </remarks>
	/// <param name="id">[in] Unique object ID.</param>
	/// <returns>Pointer to the game object, or nullptr if not found.</returns>
	inline game_object_base* get_object(unsigned int id)
	{
		if (id == 0)
		{
			LOG_ERROR("Global_object_map: Invalid id 0, no object can have this id!");
			return nullptr;
		}

		auto it = object_list.find(id);
		if (it == object_list.end())
		{
			LOG_ERROR("Global_object_map: No object with id %u found!", id);
		}
		return it != object_list.end() ? it->second : nullptr;
	}
}