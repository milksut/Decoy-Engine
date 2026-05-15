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
	
	virtual int swap_region_index(int new_index) = 0;

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
	
	game_object_base() = default;

	

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
			game_object_base::register_upload_transform([]() { Derived::tick_upload_transforms();});
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
	static void tick_upload_transforms()
	{
		if (!model || !region || transform_attrib_index < 0)
			return;

		const int region_size = static_cast<int>(region->object_ptrs.size());

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
				region,
				static_cast<unsigned int>(batch_start)           // offset within region
			);

			staging.clear();

			if(transpose_inverse_transform_attrib_index >= 0)
			{
				//same one for transpose inverse
				model->load_instance_buffer(
					reinterpret_cast<float*>(staging_inverse.data()),
					static_cast<unsigned int>(staging_inverse.size()),
					transpose_inverse_transform_attrib_index,
					region,
					static_cast<unsigned int>(batch_start)
				);
				staging_inverse.clear();
			}
			
			batch_start = -1;
		};

		for (int i = 0; i < region_size; i++)
		{
			void* ptr = region->object_ptrs[i];

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

			// Pull world matrix from EnTT
			Transform_component& tc = obj->registry.get<Transform_component>(obj->this_object);

			if (batch_start == -1)
				batch_start = i; // start a new batch here

			staging.push_back(tc.world);

			if(transpose_inverse_transform_attrib_index >= 0)
				staging_inverse.push_back(glm::transpose(glm::inverse(glm::mat3(tc.world))));

			obj->should_upload_flag = false; // reset flag after queuing for upload
		}

		flush_batch(); // flush any trailing batch
	}

	//-----------------------------------------------------------------------------------------

	entt::entity this_object;
	entt::registry& registry;

	unsigned int id = 0;

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
		if(region == nullptr)
		{
			LOG_ERROR("Game_object_basic : This object dont have a region! cant swap index");
			return -1;
		}

		int temp = region_slot_index;
		region_slot_index = new_index;
		
		if (temp >= 0 && temp < region->object_ptrs.size())
		{
			region->object_ptrs[temp] = nullptr;
		}

		if (region_slot_index >= 0 && region_slot_index < region->object_ptrs.size())
		{
			
			region->object_ptrs[region_slot_index] = this;
		}
		
		set_should_upload_flag(true);
		return temp;
	}

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

		if(comp != nullptr)
		{
			if(Global_object_map::get_object(comp->parent_id)->get_is_any_child_pos_changed_flag())
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

		if(is_pos_changed_flag)
		{
		
			// Local transform
			glm::mat4 local = glm::mat4(1.0f);
			local = glm::translate(local, this_transform.position);
			local = glm::rotate(local, glm::radians(this_transform.rotation.x), glm::vec3(1, 0, 0));
			local = glm::rotate(local, glm::radians(this_transform.rotation.y), glm::vec3(0, 1, 0));
			local = glm::rotate(local, glm::radians(this_transform.rotation.z), glm::vec3(0, 0, 1));
			local = glm::scale(local, this_transform.scale);

			// World Transform
			this_transform.world = parent_transform * local;

			is_pos_changed_flag = false;

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
	///     and places the object into a class region slot if available.
	///     Finally marks the object as needing a transform update.
	/// </remarks>
	/// <param name="registry_in">[in] ECS registry reference.</param>
	/// <param name="tag">[in] Debug/tag name of the object.</param>
	/// <param name="parent_object">[in] Optional parent object for hierarchy.</param>
	/// <param name="transform">[in] Initial transform component.</param>
	game_object_basic(entt::registry& registry_in, const std::string& tag = "Undefined tag",
		game_object_basic* parent_object = nullptr, Transform_component transform = Transform_component())
		: registry(registry_in)
	{
		this_object = registry.create();

		id = generate_id();
		
		Global_object_map::register_object(this);

		registry.emplace<Id_component>(this_object, id);
		
		registry.emplace<Tag_component>(this_object, tag);

		registry.emplace<Transform_component>(this_object, transform);
		
		if(parent_object != nullptr)
		{
			registry.emplace<Parent_component>(this_object, parent_object->get_id());
			registry.get_or_emplace<Child_component>(parent_object->this_object).children_ids.push_back(id);
		}

		if(region != nullptr)
		{
			for(int i=0; i<region->object_ptrs.size(); ++i)
			{
				if(region->object_ptrs[i] == nullptr)
				{
					region->object_ptrs[i] = this;
					region_slot_index = i;
					break;
				}
			}
			if(region_slot_index < 0)
			{
				if(region->object_ptrs.size() < region->size_in_number)
				{
					region->object_ptrs.push_back(this);
					region_slot_index = (int)region->object_ptrs.size() - 1;
				}
				else
				{
					//This is now intended use, you can have objects that dont be drawn
					//LOG_ERROR("Game_object_basic, There is not enough space in region, object Cant be Created.");
				}
			}
		}

		trigger_pos_changed_flags();
	};

	~game_object_basic()
	{
		if (region && region_slot_index >= 0)
			region->object_ptrs[region_slot_index] = nullptr;

		Global_object_map::unregister_object(id);
	}

public:

	/// <summary>
	///     Returns the unique ID of the game object.
	/// </summary>
	/// <returns>Object ID.</returns>
	unsigned int get_id() override
	{
		return id;
	}

	/// <summary>
	///     Returns a copy of the object's Transform_component.
	/// </summary>
	/// <returns>Copy of the current transform.</returns>
	Transform_component get_transform_copy() const
	{
		return get_transform_ref();
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
		if (region == nullptr)
		{
			LOG_ERROR("Game_object_basic: Cannot use null region pos, region is not initialized!");
			return;
		}
		else if (null_region_index < 0 || null_region_index >= static_cast<int>(region->object_ptrs.size()))
		{
			LOG_ERROR("Game_object_basic: Null region index %d is out of bounds (valid range: 0 to %d).",
				null_region_index, static_cast<int>(region->object_ptrs.size()) - 1);
			return;
		}
		else if (region->object_ptrs[null_region_index] != nullptr)
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
		if(id == other_object_id)
		{
			LOG_DEBUG("Game_object_basic : No need to swap pos with it self at id %d", id);
			return;
		}

		game_object_base* ptr = Global_object_map::get_object(other_object_id);

		if(ptr == nullptr)
		{
			LOG_ERROR("Game_object_basic :There is no Object with id %d to swap with!", id);
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
	void set_rotation(const glm::vec3& new_rot)
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
	glm::vec3 get_rotation() const
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
		int transform_attrib_index_in = 3, int tranpose_inverse_transform_attrib_index_in = 7)
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
		if(additional_size + region->size_in_number < 0)
		{
			LOG_WARNING("Game_object_basic: cant shrink region below 0!");
			return -1;
		}
			
		model->reserve_additional_region(additional_size + region->size_in_number, region);
		return additional_size + region->size_in_number;
	}

	//TODO: turn this into a entt comp and update when transform changes
	AABB get_world_aabb()
	{
		if (model == nullptr)
		{
			LOG_WARNING("Game_object_basic: cant get aabb, there is no defined model!");
			return AABB();
		}

		return game_object_basic_model::Mesh::get_world_aabb(model->Meshes[0]->bounding_box, get_transform_ref().world);
	}

	//--- draw -----------------------------------------------------------------------------------

	/// <summary>
	///     Draws the object using its bound model and instance region.
	/// </summary>
	/// <remarks>
	///     If amount is 0, draws the full region. If amount exceeds region size,
	///     it is clamped to region size.
	/// </remarks>
	/// <param name="shader">[in] Shader used for rendering.</param>
	/// <param name="amount">[in] Number of instances to draw (0 = full region).</param>
	/// <returns>0 on success, -1 if model is not set.</returns>
	static int draw(Shader& shader,unsigned int amount = 0)
	{
		if (model == nullptr)
		{
			LOG_WARNING("Game_object_basic: cant draw, there is no defined model!");
			return -1;
		}

		if(amount <= 0)
			amount = region->size_in_number;

		else if (amount > region->size_in_number)
		{
			LOG_WARNING("Game_object_basic: draw amount is bigger than region size, drawing only region size!");
			amount = region->size_in_number;
		}

		model->draw(shader, region, amount);
		return 0;
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
		if(id == 0)
		{
			LOG_ERROR("Global_object_map: Invalid id 0, no object can have this id!");
			return nullptr;
		}

		auto it = object_list.find(id);
		if(it == object_list.end())
		{
			LOG_ERROR("Global_object_map: No object with id %u found!", id);
		}
		return it != object_list.end() ? it->second : nullptr;
	}
}