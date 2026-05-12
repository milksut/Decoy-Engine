#pragma once

#include "Globals.h"

#include "game_object_basic_model.h"

#include "Components/Hierarchy_components.h"
#include "Components/Tag_components.h"
#include "Components/Transform_components.h"


//this class is for calling transform upload with each derived classes own static class region
class game_object_base
{
public:

	virtual void set_is_pos_changed_flag(bool val) = 0;
	virtual bool get_is_pos_changed_flag() = 0;
	virtual void set_is_any_child_pos_changed_flag(bool val) = 0;
	virtual bool get_is_any_child_pos_changed_flag() = 0;
	virtual void tick_transforms(const glm::mat4) = 0;
	virtual void trigger_child_pos_changed_flag() = 0;
	virtual unsigned int get_id() = 0;

	void static Tick(entt::registry& registry)
	{
		auto group = registry.group<Transform_component>(entt::get<Id_component>, entt::exclude<Parent_component>);

		group.each([](auto /*entity*/, Transform_component& /*transform*/, Id_component& id_comp)
		{
			Global_object_map::get_object(id_comp.id)->tick_transforms(glm::mat4(1.0f));
		});

		upload_all_transforms();

	}

	static unsigned int get_id_counter()
	{
		return id_counter;
	}

protected:
	static std::vector<std::function<void()>>& get_func_registry()
	{
		static std::vector<std::function<void()>> func_registry;
		return func_registry;
	}

	static void register_upload_transform(std::function<void()> fn)
	{
		get_func_registry().push_back(std::move(fn));
	}

	static void upload_all_transforms()
	{
		for (auto& fn : get_func_registry())
			fn();
	}

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

			if (!obj->is_pos_changed_flag)
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

			obj->is_pos_changed_flag = false; // reset flag after queuing for upload
		}

		flush_batch(); // flush any trailing batch
	}

	//-----------------------------------------------------------------------------------------

	entt::entity this_object;
	entt::registry& registry;

	unsigned int id = 0;

	int region_slot_index = -1; // this objects slot in region->object_ptrs

	Transform_component& get_transform_ref() const
	{
		return registry.get<Transform_component>(this_object);
	}

	static game_object_basic* from_region_ptr(void* ptr)
	{
		return static_cast<game_object_basic<Derived>*>(ptr);
	}

protected:

	bool is_pos_changed_flag = false;
	bool is_any_child_pos_changed_flag = false;

	//TODO: renamke this to prevent miss use
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

	void trigger_pos_changed_flags()
	{
		is_pos_changed_flag = true;
		trigger_child_pos_changed_flag();
	}

	void set_is_pos_changed_flag(bool val) override
	{
		is_pos_changed_flag = val;
	}

	bool get_is_pos_changed_flag() override
	{
		return is_pos_changed_flag;
	}

	void set_is_any_child_pos_changed_flag(bool val) override
	{
		is_any_child_pos_changed_flag = val;
	}

	bool get_is_any_child_pos_changed_flag() override
	{
		return is_any_child_pos_changed_flag;
	}


	void tick_transforms(const glm::mat4 parent_transform) override
	{
		Transform_component& this_transform = get_transform_ref();

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

		}

		Child_component* childs = registry.try_get<Child_component>(this_object);

		if (childs != nullptr)
		{
			for (unsigned int child_id : childs->children_ids)
			{
				game_object_base* child = Global_object_map::get_object(child_id);

				if (is_pos_changed_flag)
					child->set_is_pos_changed_flag(true);

				else if (!child->get_is_any_child_pos_changed_flag())
					continue;

				child->tick_transforms(this_transform.world);
			}
		}
		
		set_is_any_child_pos_changed_flag(false);

	}
	
	game_object_basic(entt::registry& registry, const std::string& tag = "Undefined tag",
		game_object_basic* parent_object = nullptr, Transform_component transform = Transform_component())
		: registry(registry)
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
					LOG_ERROR("Game_object_basic, There is not enough space in region, object Cant be Created.");
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

	unsigned int get_id() override
	{
		return id;
	}

	Transform_component get_transform_copy() const
	{
		return get_transform_ref();
	}

	//---set transforms-----------------------------------------------------------------------
	void set_position(const glm::vec3& new_pos)
	{
		get_transform_ref().position = new_pos;
		trigger_pos_changed_flags();
	}

	void set_rotation(const glm::vec3& new_rot)
	{
		get_transform_ref().rotation = new_rot;
		trigger_pos_changed_flags();
	}

	void set_scale(const glm::vec3& new_scale)
	{
		get_transform_ref().scale = new_scale;
		trigger_pos_changed_flags();
	}

	//---update transforms------------------------------------------------------------------------
	void move(const glm::vec3& delta_pos)
	{
		get_transform_ref().position += delta_pos;
		trigger_pos_changed_flags();
	}

	void rotate(const glm::vec3& delta_rot)
	{
		get_transform_ref().rotation += delta_rot;
		trigger_pos_changed_flags();
	}

	void scale(const glm::vec3& delta_scale)
	{
		get_transform_ref().scale *= delta_scale;
		trigger_pos_changed_flags();
	}

	//---get transforms------------------------------------------------------------------------
	glm::vec3 get_position() const
	{
		return get_transform_ref().position;
	}

	glm::vec3 get_rotation() const
	{
		return get_transform_ref().rotation;
	}

	glm::vec3 get_scale() const
	{
		return get_transform_ref().scale;
	}

	//--- upload/change model--------------------------------------------------------------------

	/// <param name="tranpose_inverse_transform_attrib_index_in">if -1 or smaller, dont upload transpose inverse transform</param>
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

	//you can call it with a negative to make it smaller
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

	//--- draw -----------------------------------------------------------------------------------
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
	inline std::unordered_map<unsigned int, game_object_base*> object_list;

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

	inline void unregister_object(unsigned int id)
	{
		object_list.erase(id);
	}

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