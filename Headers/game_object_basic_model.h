#pragma once

#include "Shader.h"
#include "Globals.h"
#include "Some_functions.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

//TODO: please add logs to this header, most of it just return -1 and dont log anything
/// <summary>
///     Maps internal texture types to Assimp texture types for import processing.
/// </summary>
static const aiTextureType Assimp_Tex_Types[] =
{
	#define X(name,assimp_name, second_assimp_name) assimp_name,
	TEX_TYPES
	#undef X
};

/// <summary>
///     Maps internal texture types to secondary Assimp texture type variants.
/// </summary>
static const aiTextureType Assimp_Tex_Types_2[] =
{
	#define X(name,assimp_name, second_assimp_name) second_assimp_name,
	TEX_TYPES
	#undef X
};

//TODO:add param
static glm::mat4 ai_to_glm(const aiMatrix4x4& m) {
	return glm::mat4(m.a1, m.b1, m.c1, m.d1, m.a2, m.b2, m.c2, m.d2,
		m.a3, m.b3, m.c3, m.d3, m.a4, m.b4, m.c4, m.d4);
}

class game_object_basic_model
{
public:
	struct Bone_info
	{
		/*id is index in finalBoneMatrices*/
		int id;

		/*offset matrix transforms vertex from model space to bone space*/
		glm::mat4 offset;

	};

	std::map<std::string, Bone_info> Bone_info_map;
	int Bone_counter = 0;

	glm::mat4 global_inverse_transform = glm::mat4(1.0f);

	auto& Get_bone_info_map() { return Bone_info_map; }
	int& Get_bone_count() { return Bone_counter; }
	void Set_vertex_bone_data_to_default(vertex_data& vertex)
	{
		for (int i = 0; i < MAX_BONES_PER_VERTEX; i++)
		{
			vertex.bone_ids[i] = -1;
			vertex.bone_weights[i] = 0.0f;
		}
	}

	void Set_vertex_bone_data(vertex_data& vertex, int boneID, float weight)
	{
		for (int i = 0; i < MAX_BONES_PER_VERTEX; ++i)
		{
			if (vertex.bone_ids[i] == boneID)
				break;

			if (vertex.bone_ids[i] < 0)
			{
				vertex.bone_weights[i] = weight;
				vertex.bone_ids[i] = boneID;
				break;
			}
		}
	}

	void Extract_bone_weight_for_vertices(std::vector<vertex_data>& vertices, aiMesh* mesh)
	{
		for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
		{
			int boneID = -1;
			std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
			if (boneName == "")
				boneName = "nameless_bone_" + std::to_string(boneIndex);
			if (Bone_info_map.find(boneName) == Bone_info_map.end())
			{
				Bone_info newBoneInfo;
				newBoneInfo.id = Bone_counter;
				newBoneInfo.offset = ai_to_glm(mesh->mBones[boneIndex]->mOffsetMatrix);
				Bone_info_map[boneName] = newBoneInfo;

				boneID = Bone_counter;
				Bone_counter++;
			}
			else
			{
				boneID = Bone_info_map[boneName].id;
			}

			assert(boneID != -1);

			auto weights = mesh->mBones[boneIndex]->mWeights;
			int numWeights = mesh->mBones[boneIndex]->mNumWeights;

			for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
			{
				int vertexId = weights[weightIndex].mVertexId;
				float weight = weights[weightIndex].mWeight;
				assert(vertexId < vertices.size());
				Set_vertex_bone_data(vertices[vertexId], boneID, weight);
			}
		}
		std::vector<int>bone_id_apper_counter(Bone_counter,0);
		int bone_id_minus_one_counter = 0;
		for (vertex_data& data : vertices)
		{
			for (int id : data.bone_ids)
			{
				id >= 0 ? bone_id_apper_counter[id]++ : bone_id_minus_one_counter++;
			}
				
		}

		for(int i =0; i< Bone_counter;i++)
		{
			LOG_DEBUG("Bone with id: %d appeared %d times.\n",i, bone_id_apper_counter[i]);
		}

		LOG_DEBUG("Bone with id: -1 appeared %d times.\n", bone_id_minus_one_counter);

	}

	class Mesh
	{
	private:
		
		struct attribute
		{
			unsigned int VBO;
			int attrib_start_index;
			int attrib_fin_index;
			unsigned int attrib_size_bytes;
			int loop_instance;
		};

		attribute empty_attrib = { 0,-1,-1,0,-1 };

		std::vector<attribute> instance_attributes;

		std::vector<std::shared_ptr<class_region>> shared_regions;

		bool can_override_vbo = false;

		/// <summary>
		///     Binds all mesh textures and material data to the shader as sampler arrays and uniforms.
		/// </summary>
		/// <param name="shader">[in] Shader used for texture and material binding.</param>
		void bind_textures(Shader& shader)
		{
			//these are sent as uniforms to shader as sampler 2d arrays like TEXTURE[], DIFFUSE[] etc. What name is defined in globals.h
			//sent data amount is sent as int array named TEX_COUNTS[], they are in order of enum TextureType
			std::array<std::vector<int>,Tex_type_amount> texture_locations;
			for(std::vector<int> &var : texture_locations)
			{
				var.reserve(TEXTURE_SLOTS);
			}
			int counts[Tex_type_amount] = { 0 };

			for (int i = 0; i < TEXTURE_SLOTS && i < main_textures.size(); i++)
			{
				int slot_index = Texture_slots::bound_texture(main_textures[i].id);
				counts[main_textures[i].type]++;

				texture_locations[main_textures[i].type].push_back(slot_index);
			}
			
			shader.setInt("TEX_COUNTS", counts, Tex_type_amount);
			for(int i = 0; i<Tex_type_amount;i++)
			{
				if(counts[i] > 0)
				{
					shader.setInt(Tex_Types_Names[i], texture_locations[i].data(), counts[i]);
				}
				
			}

			if (!Material_slots::init_flag)
			{
				Material_slots::init_material_slots();
			}
			int material_index = -1;//-1 means no material
			if(mesh_material_id > 0)
			{
				material_index = Material_slots::get_index_of_bound_slot(mesh_material_id);
				if( material_index < 0)
				{
					LOG_INFO("Mesh with material ID %d has no bound material! Binding now.", mesh_material_id);
					material_index = Material_slots::bound_material(mesh_material_id);
				}
			}
			shader.setInt("material_index", material_index);

		}
		
		/// <summary>
		///     Deletes an instance buffer and disables its vertex attributes.
		/// </summary>
		/// <param name="attrib_index">[in] Attribute index of the instance buffer.</param>
		void delete_instance_buffer(int attrib_index)
		{
			if (attrib_index >= VAO_MAX_ATTRIB_AMOUNT || attrib_index <= MODEL_ATRIB_LAST_INDEX)
				return; // Invalid or mesh's attribute index

			attribute attrib = instance_attributes[attrib_index];
			if (attrib.VBO == 0)
				return; // No instance buffer to delete

			glBindVertexArray(VAO);
			
			glDeleteBuffers(1, &attrib.VBO);

			for (int i = attrib.attrib_start_index; i <= attrib.attrib_fin_index; i++)
			{
				instance_attributes[i] = empty_attrib;

				glDisableVertexAttribArray(i);
				glVertexAttribDivisor(i, 0); // Reset divisor
			}

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}

		/// <summary>
		///     Calculates and assigns the offset position for each class region in number space.
		/// </summary>
		void calc_offset_in_number()
		{
			int offset = 0;
			for (std::shared_ptr<class_region> region : shared_regions)
			{
				region->offset_in_numbers = offset;
				offset += region->size_in_number;
			}
		}
	public:

		std::vector<vertex_data>  main_vertices;
		std::vector<unsigned int> main_indices;
		std::vector<Texture>      main_textures;
		std::shared_ptr<class_region> last_bound_region = nullptr;
		AABB bounding_box = {};


		//material properties for this mesh:
		unsigned int mesh_material_id = 0;

		unsigned int VAO, VBO_Mesh, EBO;

		//todo: fix param
		/// <summary>
		///     Constructs a mesh and uploads vertex, index, and texture data to GPU buffers.
		/// </summary>
		/// <param name="vertices">[in] Vertex data of the mesh.</param>
		/// <param name="indices">[in] Index data of the mesh.</param>
		/// <param name="textures">[in] Textures associated with the mesh.</param>
		Mesh(const std::vector<vertex_data> &vertices,const std::vector<unsigned int> &indices,const std::vector<Texture>& textures)
			:instance_attributes(VAO_MAX_ATTRIB_AMOUNT, empty_attrib), main_vertices(vertices), main_indices(indices), main_textures(textures)
		{

			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO_Mesh);
			glGenBuffers(1, &EBO);


			glBindVertexArray(VAO);
			if (vertices.size() > 0)
			{
				glBindBuffer(GL_ARRAY_BUFFER, VBO_Mesh);
				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex_data), &vertices[0], GL_STATIC_DRAW);	

				bounding_box = calculate_aabb(vertices);
			}
			else
			{
				LOG_WARNING("Mesh - a mesh with empty vertices created, is this intentional?");
			}
			

			if (indices.size() > 0)
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
			}
			else
			{
				LOG_WARNING("Mesh - a mesh with empty indices created, is this intentional?");
			}
			

			// vertex positions
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_data), (void*)nullptr);
			instance_attributes[0] = { VBO_Mesh,0,0,3* sizeof(float),0 };

			// vertex texture coords
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_data), (void*)offsetof(vertex_data, tex_coords));
			instance_attributes[1] = { VBO_Mesh,1,1,2* sizeof(float),0 };

			// vertex  normals
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_data), (void*)offsetof(vertex_data, normal));
			instance_attributes[2] = { VBO_Mesh,2,2,3* sizeof(float),0 };
			
			// vertex  Bones
			glEnableVertexAttribArray(3);
			glVertexAttribIPointer(3, 4, GL_INT, sizeof(vertex_data), (void*)offsetof(vertex_data, bone_ids));
			instance_attributes[3] = { VBO_Mesh,3,3,4 * sizeof(int),0 };

			//vertex  Bone weights
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_data), (void*)offsetof(vertex_data, bone_weights));
			instance_attributes[4] = { VBO_Mesh,4,4,4 * sizeof(float),0 };
			
			glBindVertexArray(0);
		}

		/// <summary>
		///     Destructor that releases all GPU resources used by the mesh.
		/// </summary>
		/// <remarks>
		///     Deletes VBO, EBO, VAO, instance buffers, and clears texture and region data.
		/// </remarks>
		~Mesh()
		{
			glDeleteBuffers(1, &VBO_Mesh);
			glDeleteBuffers(1, &EBO);

			for (const attribute& id : instance_attributes)
				if (id.VBO) glDeleteBuffers(1, &id.VBO);
			
			glDeleteVertexArrays(1, &VAO);

			main_textures.clear();

			for(std::shared_ptr<class_region> ptr : shared_regions)
			{
				ptr.reset();
			}
		}

		///this function is dangerous! don't use if you don't know what you are doing
		///this function needs to be refactored, dont depend on it.
		//TODO: Split this function into multiple functions for updating vertices, indices and textures separately
		/// <summary>
		///     Updates mesh vertex, index, and texture data on the GPU.
		/// </summary>
		/// <param name="vertices">[in] New vertex data for the mesh.</param>
		/// <param name="update_aabb">[in] If true, recalculates the axis-aligned bounding box based on new vertex data.</param>
		/// <param name="indices">[in] New index data for the mesh.</param>
		/// <param name="textures">[in] New textures associated with the mesh.</param>
		/// <param name="use_dynamic_draw">[in] If true, uses dynamic draw for buffer updates.</param>
		void update_mesh(const std::vector<vertex_data> &vertices,const bool update_aabb = true,const std::vector<unsigned int> &indices = std::vector<unsigned int>(),
			const std::vector<Texture>& textures = std::vector<Texture>() ,bool use_dynamic_draw = false)
		{

			glBindVertexArray(VAO);

			if (vertices.size() > 0)
			{	
				this->main_vertices = vertices;

				glBindBuffer(GL_ARRAY_BUFFER, VBO_Mesh);

				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex_data), nullptr,
					use_dynamic_draw ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);//orphan

				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex_data), &vertices[0],
					use_dynamic_draw ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);//new

				if (update_aabb)
				{
					bounding_box = calculate_aabb(vertices);
				}
			}
			else
			{
				LOG_WARNING("Mesh - a mesh updated with empty vertices, is this intentional?");
			}


			if (indices.size() > 0)
			{
				this->main_indices = indices;
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), nullptr,
					use_dynamic_draw ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);//orphan

				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0],
					use_dynamic_draw ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);//new
			}
			else
			{
				//printf("Mesh - a mesh updated with empty indices, is this intentional?\n");
				//yes - it is
			}

			if(textures.size() > 0)
			{
				this->main_textures = textures;
			}
			else
			{
				//printf("Mesh - a mesh updated with empty textures, is this intentional?\n");
			}
		}

		Mesh(const Mesh&) = delete;// Delete copy constructor

		Mesh& operator=(const Mesh&) = delete;// Delete copy assignment operator

		/// <summary>
		///     Creates and reserves a new class region in instance data storage.
		/// </summary>
		/// <param name="size_in_number">[in] Size of the region in number of elements.</param>
		/// <returns>Shared pointer to the created class region.</returns>
		std::shared_ptr<class_region> reserve_class_region(int size_in_number)
		{
			std::shared_ptr<class_region> region = std::make_shared<class_region>();
			region->size_in_number = size_in_number;
			region->data_ptrs = std::vector<std::shared_ptr<float>>(VAO_MAX_ATTRIB_AMOUNT, nullptr);
			region->data_amount = std::vector<unsigned int>(VAO_MAX_ATTRIB_AMOUNT, 0);
			shared_regions.push_back(region);

			calc_offset_in_number();

			//resize VBOS and re upload data
			for(attribute &attrib : instance_attributes)
			{
				if (attrib.attrib_start_index <= MODEL_ATRIB_LAST_INDEX)
					continue;

				override_instance_buffer(attrib.attrib_size_bytes / sizeof(float), attrib.attrib_start_index, attrib.loop_instance);

				if(attrib.VBO != 0)
					load_all_regions_for_attribute(attrib.attrib_start_index);
			}

			return region;
		}

		/// <summary>
		///     Expands an existing class region and updates instance buffer layout.
		/// </summary>
		/// <param name="size_in_number">[in] New size of the region in number of elements.</param>
		/// <param name="region">[in] Region to be resized.</param>
		void reserve_additional_region(int size_in_number, std::shared_ptr<class_region> region)
		{
			region->size_in_number = size_in_number;

			calc_offset_in_number();

			//resize VBOS and re upload data
			for (attribute& attrib : instance_attributes)
			{
				override_instance_buffer(attrib.attrib_size_bytes / sizeof(float), attrib.attrib_start_index, attrib.loop_instance);
				if (attrib.VBO != 0)
					load_all_regions_for_attribute(attrib.attrib_start_index);
			}
		}

		/// <summary>
		///     Adds a new class region and updates instance buffer layout.
		/// </summary>
		/// <param name="region">[in] Region to be added.</param>
		void add_class_region(std::shared_ptr<class_region> region)
		{
			shared_regions.push_back(region);
			calc_offset_in_number();

			//resize VBOS and re upload data
			for (attribute& attrib : instance_attributes)
			{
				if (attrib.attrib_start_index <= MODEL_ATRIB_LAST_INDEX)
					continue;

				override_instance_buffer(attrib.attrib_size_bytes / sizeof(float), attrib.attrib_start_index, attrib.loop_instance);

				if (attrib.VBO != 0)
					load_all_regions_for_attribute(attrib.attrib_start_index);
			}
		}

		/// <summary>
		///     Uploads all class region data for a specific vertex attribute to the GPU buffer.
		/// </summary>
		/// <param name="attrib_index">[in] Attribute index whose region data will be uploaded.</param>
		void load_all_regions_for_attribute(int attrib_index)
		{
			if (attrib_index >= VAO_MAX_ATTRIB_AMOUNT || attrib_index <= MODEL_ATRIB_LAST_INDEX)
				return; // Invalid or mesh's attribute index

			attribute attrib = instance_attributes[attrib_index];
			if (attrib.VBO == 0)
				return; // No instance buffer exists for this attribute

			glBindBuffer(GL_ARRAY_BUFFER, attrib.VBO);

			for (std::shared_ptr<class_region> region : shared_regions)
			{
				// Check if this region has data for this attribute
				if (region->data_ptrs[attrib_index] == nullptr)
					continue;
				if (region->data_amount[attrib_index] == 0)
					continue;

				// Calculate offset in bytes
				unsigned int offset_bytes = region->offset_in_numbers * attrib.attrib_size_bytes;
				unsigned int size_bytes = region->data_amount[attrib_index] * sizeof(float);

				// Upload the data
				glBufferSubData(GL_ARRAY_BUFFER, offset_bytes, size_bytes, region->data_ptrs[attrib_index].get());
			}

			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		
		//add instance buffer for instanced rendering

		/// <summary>
		///     Creates an instance buffer for per-instance vertex attributes (e.g. colors, model matrices).
		///     This function only allocates GPU buffers for each known class region; it does not upload data.
		///     Use load_instance_buffer / load_all_regions_for_attribute to fill the buffer after creation.
		///     Most VAOs support ~16 attribute slots (0->4 reserved for mesh data). Each slot can store up to 4 floats.
		///     If more is needed, multiple attribute indices are used.
		///     Ensure sufficient vector capacity for all instance data; otherwise undefined behavior may occur.
		/// </summary>
		/// <param name="attrib_size">[in] Size of the attribute in floats.</param>
		/// <param name="attrib_index">[in] Starting attribute index in the VAO.</param>
		/// <param name="loop_instance">[in] Instance divisor (default = 1).</param>
		/// <returns>0 on success, -1 on failure.</returns>
		int add_instance_buffer(int attrib_size, int attrib_index, int loop_instance = 1)
		{
			if (!can_override_vbo && instance_attributes[attrib_index].VBO != 0)
			{
				LOG_ERROR("Game_object_basic_model: Instance attribute buffer is already filled and override is disabled");
				return -1; //attribute already filled
			}
			int index_amount = (attrib_size / 4) + (attrib_size % 4 == 0 ? 0 : 1);

			if (attrib_index + index_amount - 1 >= VAO_MAX_ATTRIB_AMOUNT || attrib_index <= MODEL_ATRIB_LAST_INDEX)
			{
				LOG_ERROR("Game_object_basic_model: Invalid attribute index or index range exceeds VAO maximum attribute amount");
				return -1; // Invalid or mesh's attribute index
			}
			if (can_override_vbo && instance_attributes[attrib_index].VBO != 0)
			{
				glDeleteBuffers(1, &instance_attributes[attrib_index].VBO);
				// clear all slots that shared this VBO
				for (auto& attrib : instance_attributes)
					if (attrib.VBO == instance_attributes[attrib_index].VBO)
						attrib = empty_attrib;
			}

			int wanted_amount = shared_regions.empty() ? 0 : shared_regions.back()->offset_in_numbers + shared_regions.back()->size_in_number;

			unsigned int attrib_size_bytes = (attrib_size * (unsigned int)sizeof(float));
			
			unsigned int VBO_TEMP;
			glGenBuffers(1, &VBO_TEMP);
			
			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO_TEMP);
			glBufferData(GL_ARRAY_BUFFER, attrib_size_bytes * wanted_amount, nullptr, GL_DYNAMIC_DRAW);
			
			for(int i = 0; i< index_amount; i++)
			{
				instance_attributes[attrib_index + i] = { VBO_TEMP,attrib_index,attrib_index + index_amount -1,attrib_size_bytes , loop_instance };
				glEnableVertexAttribArray(attrib_index + i);

				//glVertexAttribPointer(attrib_index + i, 4, GL_FLOAT, GL_FALSE, attrib_size * sizeof(float), (void*)(i * sizeof(glm::vec4)));
				//i am transitioning to slicing vbo spaces and giving classes their own offsets
				//so they can call it with that ofsset (givving offset to attrip pointer)

				glVertexAttribDivisor(attrib_index + i, loop_instance); // Update this attribute per instance
			}

			glBindVertexArray(0);

			load_all_regions_for_attribute(attrib_index);

			return 0;
		}

		/// <summary>
		///     Recreates an instance buffer by temporarily allowing buffer override.
		/// </summary>
		/// <param name="attrib_size">[in] Size of the attribute in floats.</param>
		/// <param name="attrib_index">[in] Starting attribute index in the VAO.</param>
		/// <param name="loop_instance">[in] Instance divisor (default = 1).</param>
		/// <returns>Result of buffer creation (0 on success, -1 on failure).</returns>
		int override_instance_buffer(int attrib_size, int attrib_index, int loop_instance = 1)
		{
			can_override_vbo = true;
			int result = add_instance_buffer(attrib_size, attrib_index, loop_instance);
			can_override_vbo = false;
			return result;
		}

		/// <summary>
		///     Loads (overwrites) data into an existing instance buffer at a specific region offset.
		/// </summary>
		/// <remarks>
		///     Use after add_instance_buffer / reserve_class_region.
		///     Data is written using glBufferSubData and replaces existing GPU memory.
		/// </remarks>
		/// <param name="data">[in] Pointer to float data (e.g. vector.data()).</param>
		/// <param name="amount_in_attrib_size">[in] Number of attribute units to write.</param>
		/// <param name="attrib_index">[in] Attribute index to write into.</param>
		/// <param name="region">[in] Class region describing memory layout.</param>
		/// <param name="data_offset_by_attrib_size">[in] Offset in attribute units (default = 0).</param>
		void load_instance_buffer(float* data, unsigned int amount_in_attrib_size, int attrib_index,
			std::shared_ptr<class_region> region, unsigned int data_offset_by_attrib_size = 0)
		{
			//TODO: log the errors.
			if (attrib_index >= VAO_MAX_ATTRIB_AMOUNT || attrib_index <= MODEL_ATRIB_LAST_INDEX)
				return; // Invalid or mesh's attribute index

			attribute attrib = instance_attributes[attrib_index];
			if (attrib.VBO == 0)
				return; // No instance buffer to load data

			if(amount_in_attrib_size + data_offset_by_attrib_size > region->size_in_number)
				return; // Trying to load more data than region size

			glBindBuffer(GL_ARRAY_BUFFER, attrib.VBO);
			glBufferSubData(GL_ARRAY_BUFFER, (region->offset_in_numbers + data_offset_by_attrib_size) * attrib.attrib_size_bytes,
				amount_in_attrib_size * attrib.attrib_size_bytes, data);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		/// <summary>
		///     Draws the mesh using instanced rendering with bound textures and instance attributes.
		/// </summary>
		/// <remarks>
		///     Rebinds attribute offsets when the active class region changes and issues instanced draw calls.
		/// </remarks>
		/// <param name="shader">[in] Shader used for rendering.</param>
		/// <param name="region">[in] Active class region for instance data layout.</param>
		/// <param name="amount">[in] Number of instances to draw (default = 1).</param>
		void draw(Shader& shader, std::shared_ptr<class_region>& region, int amount = 1)
		{
			bind_textures(shader);
			
			glBindVertexArray(VAO);
			
			if (last_bound_region != region)
			{
				last_bound_region = region;

				for (const attribute& attrib : instance_attributes)
				{
					if (attrib.VBO == 0)
						continue;

					if(attrib.attrib_start_index <= MODEL_ATRIB_LAST_INDEX)
						continue; //mesh attribute

					glBindBuffer(GL_ARRAY_BUFFER, attrib.VBO);

					unsigned int attribute_size_number = (attrib.attrib_size_bytes / (unsigned int)sizeof(float));
					int component_offset = 0;

					for (int i = attrib.attrib_start_index; i <= attrib.attrib_fin_index; i++)
					{
						int components = (4 < attribute_size_number) ? 4 : attribute_size_number;
						glVertexAttribPointer(i, components, GL_FLOAT, GL_FALSE, attrib.attrib_size_bytes,
							(void*)((region->offset_in_numbers * attrib.attrib_size_bytes) + (component_offset * sizeof(float))));

						component_offset += components;
						attribute_size_number -= components;
					}

				}
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			if(main_indices.size()> 0)
			{
				glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(main_indices.size()),
					GL_UNSIGNED_INT, nullptr, amount);
			}
			else
			{
				glDrawArraysInstanced(GL_POINTS, 0, (unsigned int)main_vertices.size(), amount);
			}
			draw_call_count++;
		}
	};

	struct Mesh_Childs
	{
		std::vector<std::shared_ptr<Mesh>> Meshes;
		std::vector<std::unique_ptr<Mesh_Childs>> Childs;
		int flattened_index_start = -1;
		int flattened_index_end = -1;

	};

	/// <summary>
	///     Recursively processes an Assimp scene node, extracting meshes and child nodes.
	/// </summary>
	/// <remarks>
	///     Converts Assimp meshes into engine meshes and builds a hierarchical node structure.
	/// </remarks>
	/// <param name="node">[in] Current Assimp node being processed.</param>
	/// <param name="scene">[in] Assimp scene containing mesh data.</param>
	/// <param name="parent_mesh">[in/out] Parent mesh container for hierarchy construction.</param>
	/// <param name="path">[in] File path of the loaded model.</param>
	void process_node(aiNode* node, const aiScene* scene, Mesh_Childs& parent_mesh,const std::string& path)
	{
		int range_start = (int)Meshes.size();

		//process meshes of node
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			//process mesh
			std::shared_ptr<Mesh> temp = process_mesh(mesh, scene, path);

			parent_mesh.Meshes.push_back(temp);
			Meshes.push_back(temp);
			
			LOG_INFO("Processed mesh: %s", mesh->mName.C_Str());
		}

		//process childs
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			std::unique_ptr<Mesh_Childs> child_mesh = std::make_unique<Mesh_Childs>();
			process_node(node->mChildren[i], scene, *child_mesh, path);
			parent_mesh.Childs.push_back(std::move(child_mesh));
		}

		if ((int)Meshes.size() > range_start)
		{
			parent_mesh.flattened_index_start = range_start;

			parent_mesh.flattened_index_end = (int)Meshes.size() - 1;
		}
	}

	/// <summary>
	///     Processes an Assimp mesh into engine mesh data (vertices, indices, textures, materials).
	/// </summary>
	/// <remarks>
	///     Extracts geometry, loads textures, reads material properties, and registers materials if present.
	/// </remarks>
	/// <param name="mesh">[in] Assimp mesh to process.</param>
	/// <param name="scene">[in] Assimp scene containing mesh/material data.</param>
	/// <param name="path">[in] File path of the model for texture resolution.</param>
	/// <returns>Shared pointer to the created Mesh.</returns>
	std::shared_ptr<Mesh> process_mesh(aiMesh* mesh, const aiScene* scene,const std::string& path)
	{
		std::vector<vertex_data> vertices;
		std::vector<unsigned int> indices;
		std::vector<Texture> textures;
		material_properties mat_props = {};
		//process vertices
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			vertex_data vertex;

			Set_vertex_bone_data_to_default(vertex);

			//positions
			vertex.position[0] = mesh->mVertices[i].x;
			vertex.position[1] = mesh->mVertices[i].y;
			vertex.position[2] = mesh->mVertices[i].z;
			//normals
			if (mesh->HasNormals())
			{
				vertex.normal[0] = mesh->mNormals[i].x;
				vertex.normal[1] = mesh->mNormals[i].y;
				vertex.normal[2] = mesh->mNormals[i].z;
			}
			else
			{
				vertex.normal[0] = 0.0f;
				vertex.normal[1] = 0.0f;
				vertex.normal[2] = 0.0f;
			}
			//texture coords
			if (mesh->mTextureCoords[0]) //does the mesh contain texture coordinates?
			{
				vertex.tex_coords[0] = mesh->mTextureCoords[0][i].x;
				vertex.tex_coords[1] = mesh->mTextureCoords[0][i].y;
			}
			else
			{
				vertex.tex_coords[0] = 0.0f;
				vertex.tex_coords[1] = 0.0f;
			}
			vertices.push_back(vertex);
		}
		Extract_bone_weight_for_vertices(vertices, mesh);

		//process indices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}
		bool uses_material_flag = false;
		//process material
		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			bool loop = true;
			for (int i = 0; i < Tex_type_amount; loop = !loop)
			{
				aiTextureType assimp_type = loop ? Assimp_Tex_Types[i] : Assimp_Tex_Types_2[i];

				for (unsigned int j = 0; j < material->GetTextureCount(assimp_type); j++)
				{
					Texture* texture;
					aiString str;
					bool is_embedded_texture = false;

					//assimp_type - Texture type, j - Index of the texture to get, &str - output path
					material->GetTexture(assimp_type, j, &str);

					const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(str.C_Str());

					if (embeddedTexture == nullptr)
					{
						std::string directory = path.substr(0, path.find_last_of('\\'));
						str.Set((directory + '\\' + str.C_Str()).c_str());
					}
					

					//TODO: Add Logs
					texture = Texture_slots::get_loaded_texture(str.C_Str());//check if texture was loaded before
					if (texture != nullptr)
					{
						textures.push_back(*texture);
					}
					else
					{
						// data returned by load_texture_from_file but unused here
						int unused_data1 = 0, unused_data2 = 0, unused_data3 = 0;
						texture = new Texture();
						if (embeddedTexture != nullptr)
						{

							if (embeddedTexture->mHeight == 0)
							{
								// compressed format (PNG/JPEG) stored in embeddedTexture->pcData, size in mWidth
								size_t dataSize = static_cast<size_t>(embeddedTexture->mWidth);
								const unsigned char* dataPtr = reinterpret_cast<const unsigned char*>(embeddedTexture->pcData);
								int w = 0, h = 0, channels = 0;
								texture->id = load_image_from_memory(dataPtr, dataSize, w, h, channels, 4, true);
							}
							else
							{
								// uncompressed RGBA8888: mWidth = width, mHeight = height, pcData points to pixels
								int w = static_cast<int>(embeddedTexture->mWidth);
								int h = static_cast<int>(embeddedTexture->mHeight);
								int channels = 4; // Assimp stores uncompressed embedded textures as RGBA
								unsigned char* pixels = reinterpret_cast<unsigned char*>(embeddedTexture->pcData);
								texture->id = process_image(pixels, w, h, channels, 4);
								// Note: process_image calls stbi_image_free on the data pointer,
								// so do NOT free embeddedTexture->pcData here (Assimp owns it). If process_image
								// must not free it, duplicate the buffer first or modify process_image.
							}

						}
						else
						{
							texture->id = load_texture_from_file(str.C_Str(), unused_data1, unused_data2, unused_data3);
						}

						texture->type = static_cast<TextureType>(i);
						texture->path = str.C_Str();

						textures.push_back(*texture);
						Texture_slots::new_texture_loaded(*texture);
					}
				}

				if (!loop) i++;
			}

			aiColor3D color;
			float value;
			int illum;
			aiString name;

			if (AI_SUCCESS == material->Get(AI_MATKEY_NAME, name))
			{

				if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_AMBIENT, color))
					mat_props.ambient = glm::vec3(color.r, color.g, color.b);

				if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color))
					mat_props.diffuse = glm::vec3(color.r, color.g, color.b);

				if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, color))
					mat_props.specular = glm::vec3(color.r, color.g, color.b);

				if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, value))
					mat_props.shininess = value;

				if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_EMISSIVE, color))
					mat_props.emission = glm::vec3(color.r, color.g, color.b);

				if (AI_SUCCESS == material->Get(AI_MATKEY_OPACITY, value))
					mat_props.opacity = value;

				if (AI_SUCCESS == material->Get(AI_MATKEY_REFRACTI, value))
					mat_props.index_of_refraction = value;

				if (AI_SUCCESS == material->Get(AI_MATKEY_SHADING_MODEL, illum))
					mat_props.illumination_model = (float)illum;
			}
			if (!Material_slots::init_flag)
			{
				Material_slots::init_material_slots();
			}
			uses_material_flag = true;
		}

		std::shared_ptr<Mesh> mesh_ptr = std::make_shared<Mesh>(vertices, indices, textures);
		mesh_ptr->mesh_material_id = uses_material_flag ? Material_slots::register_material(mat_props) : 0;
		return mesh_ptr;
	}

	Assimp::Importer scene_importer;
	const aiScene* last_scene_pointer = nullptr;

	std::vector<std::shared_ptr<Mesh>> Meshes;//you cant use copy constructor or assignment operator because of Mesh class

	std::vector<Mesh_Childs> roots;

	AABB model_aabb = {};

	void update_model_aabb()
	{
		if (Meshes.empty())
			return;
		model_aabb = Meshes[0]->bounding_box;
		for (const std::shared_ptr<Mesh>& mesh : Meshes)
		{
			model_aabb.min = glm::min(model_aabb.min, mesh->bounding_box.min);
			model_aabb.max = glm::max(model_aabb.max, mesh->bounding_box.max);
		}
	}

	static AABB calculate_aabb(const std::vector<glm::vec3>& vertices)
	{
		glm::vec3 min(FLT_MAX);
		glm::vec3 max(-FLT_MAX);

		for (const auto& v : vertices)
		{
			min.x = std::min(min.x, v.x);
			min.y = std::min(min.y, v.y);
			min.z = std::min(min.z, v.z);

			max.x = std::max(max.x, v.x);
			max.y = std::max(max.y, v.y);
			max.z = std::max(max.z, v.z);
		}
		return { min, max };
	}

	static AABB calculate_aabb(const std::vector<vertex_data>& vertices)
	{
		glm::vec3 min(FLT_MAX);
		glm::vec3 max(-FLT_MAX);
		for (const auto& v : vertices)
		{
			min.x = std::min(min.x, v.position[0]);
			min.y = std::min(min.y, v.position[1]);
			min.z = std::min(min.z, v.position[2]);
			max.x = std::max(max.x, v.position[0]);
			max.y = std::max(max.y, v.position[1]);
			max.z = std::max(max.z, v.position[2]);
		}
		return { min, max };
	}

	static AABB get_world_aabb(const AABB& local, const glm::mat4& model)
	{
		glm::vec3 corners[8] = {
			{local.min.x, local.min.y, local.min.z},
			{local.max.x, local.min.y, local.min.z},
			{local.min.x, local.max.y, local.min.z},
			{local.max.x, local.max.y, local.min.z},
			{local.min.x, local.min.y, local.max.z},
			{local.max.x, local.min.y, local.max.z},
			{local.min.x, local.max.y, local.max.z},
			{local.max.x, local.max.y, local.max.z},
		};

		AABB world;
		world.min = glm::vec3(FLT_MAX);
		world.max = glm::vec3(-FLT_MAX);

		for (auto& c : corners) {
			glm::vec3 wc = glm::vec3(model * glm::vec4(c, 1.0f));
			world.min = glm::min(world.min, wc);
			world.max = glm::max(world.max, wc);
		}
		return world;
	}

	/// <summary>
	///     Creates a new mesh and adds it to the mesh collection.
	/// </summary>
	/// <param name="vertices">[in] Vertex data of the mesh.</param>
	/// <param name="indices">[in] Index data of the mesh.</param>
	/// <param name="textures">[in] Textures associated with the mesh.</param>
	void add_mesh(const std::vector<vertex_data>& vertices,const std::vector<unsigned int>& indices,const std::vector<Texture>& textures)
	{
		Meshes.push_back(std::make_shared<Mesh>(vertices, indices, textures));
		update_model_aabb();
	}

	/// <summary>
	///     Draws all child meshes using the given shader and instance region.
	/// </summary>
	/// <param name="shader">[in] Shader used for rendering.</param>
	/// <param name="region">[in] Instance data region used for drawing.</param>
	/// <param name="amount">[in] Number of instances to draw per mesh (default = 1).</param>
	void draw(Shader& shader, std::shared_ptr<class_region>& region, int amount = 1)
	{
		for(std::shared_ptr<Mesh> pointer : Meshes)
		{
			pointer->draw(shader, region, amount);
		}
	}

	/// <summary>
	///     Reserves a shared class region across all meshes in the object.
	/// </summary>
	/// <remarks>
	///     Creates the region in the first mesh and propagates it to all others.
	/// </remarks>
	/// <param name="size_in_number">[in] Size of the region in number of elements.</param>
	/// <returns>Shared pointer to the created class region.</returns>
	std::shared_ptr<class_region> reserve_class_region(int size_in_number)
	{
		if (Meshes.empty()) {
			LOG_FATAL("No meshes to reserve class region for.");
			throw std::runtime_error("No meshes to reserve class region for.");
		}
		// Reserve region in the first mesh
		std::shared_ptr<class_region> region = Meshes[0]->reserve_class_region(size_in_number);

		// Add the same region to all other meshes
		for (size_t i = 1; i < Meshes.size(); i++)
		{
			Meshes[i]->add_class_region(region);
		}
		return region;
	}

	/// <summary>
	///     Expands an existing class region across all meshes in the model.
	/// </summary>
	/// <remarks>
	///     Updates region size and propagates the change to each mesh.
	/// </remarks>
	/// <param name="size_in_number">[in] New size of the region in number of elements.</param>
	/// <param name="region">[in] Region to be expanded.</param>
	void reserve_additional_region(int size_in_number, std::shared_ptr<class_region> region)
	{
		for (std::shared_ptr<Mesh> pointer : Meshes)
		{
			pointer->reserve_additional_region(size_in_number, region);
		}
	}

	/// <summary>
	///     Creates per-instance attribute buffers across all meshes (e.g. color, model matrix).
	/// </summary>
	/// <remarks>
	///     Only allocates buffers. Use load_instance_buffer to fill data.
	///     If expanding existing buffers, data must be reloaded after.
	///     VAO typically supports ~16 attribute slots (0->4 reserved for mesh data).
	///     Each slot can hold up to 4 floats; larger data uses multiple slots.
	///     Ensure sufficient instance data capacity; otherwise undefined behavior may occur.
	/// </remarks>
	/// <param name="attrib_size">[in] Size of the attribute in floats.</param>
	/// <param name="attrib_index">[in] Starting VAO attribute index.</param>
	/// <param name="loop_instance">[in] Instance divisor (default = 1).</param>
	void add_instance_buffer(int attrib_size, int attrib_index, int loop_instance = 1)
	{
		for (std::shared_ptr<Mesh> pointer : Meshes)
		{
			pointer->add_instance_buffer(attrib_size, attrib_index, loop_instance);
		}
	}


	/// <summary>
	///     Loads (overwrites) instance buffer data across all meshes in the model.
	/// </summary>
	/// <remarks>
	///     Writes into previously created buffers (add_instance_buffer).
	///     Data is overwritten at the specified region offset.
	///     Use class_region to define shared memory layout across meshes.
	/// </remarks>
	/// <param name="data">[in] Pointer to float data (e.g. vector.data()).</param>
	/// <param name="amount_in_attrib_size">[in] Number of attribute units to write.</param>
	/// <param name="attrib_index">[in] Attribute index to write to.</param>
	/// <param name="region">[in] Class region describing memory layout.</param>
	/// <param name="data_offset_by_attrib_size">[in] Offset in attribute units (default = 0).</param>
	void load_instance_buffer(float* data, unsigned int amount_in_attrib_size, int attrib_index,
		std::shared_ptr<class_region> region, unsigned int data_offset_by_attrib_size = 0)
	{
		for (std::shared_ptr<Mesh> pointer : Meshes)
		{
			pointer->load_instance_buffer(data, amount_in_attrib_size, attrib_index, region, data_offset_by_attrib_size);
		}
	}

	//TODO: fix param
	/// <summary>
	///     Imports a 3D model from file using Assimp and builds the mesh hierarchy.
	/// </summary>
	/// <remarks>
	///     Loads scene data, then processes the root node recursively into engine meshes.
	/// </remarks>
	/// <param name="path">[in] File path of the model to import.</param>
	int import_model_from_file(std::string path, const bool flip_uvs = true)
	{
		const aiScene* scene = scene_importer.ReadFile(path, aiProcess_Triangulate 
			| (flip_uvs ? aiProcess_FlipUVs : 0u) | aiProcess_CalcTangentSpace);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOG_ERROR("Assimp error: %s", scene_importer.GetErrorString());
			return -1;
		}
		else
		{
			last_scene_pointer = const_cast<aiScene*>(scene); // Store pointer for potential future use
			global_inverse_transform = glm::inverse(ai_to_glm(last_scene_pointer->mRootNode->mTransformation));
			Mesh_Childs new_root;
			process_node(scene->mRootNode, scene, new_root,path);
			roots.push_back(std::move(new_root));
			update_model_aabb();
			return (int)roots.size() - 1;
		}

		
	}

	/// <summary>
	///     Applies a transformation matrix directly to mesh vertices and normals.
	/// </summary>
	/// <remarks>
	///     Transforms vertex positions using the given matrix and correctly updates normals
	///     using the inverse-transpose of the rotation part. Then uploads the updated mesh data.
	/// </remarks>
	/// <param name="mesh_index">[in] Index of the mesh to modify.</param>
	/// <param name="transform">[in] Transformation matrix to apply to vertices.</param>
	/// <param name="update_aabb">[in] If true, recalculates the axis-aligned bounding box after transformation (default = true).</param>
	void offset_mesh_vertices(const unsigned int mesh_index, const glm::mat4 transform, bool update_aabb = true)
	{
		if(Meshes.size() <= mesh_index)
		{
			LOG_ERROR("Mesh index out of bounds in offset_mesh_vertices");
			return;
		}

		std::vector<vertex_data>& vertices = Meshes[mesh_index]->main_vertices;
		glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(transform)));

		for (vertex_data& v : vertices)
		{
			//pos
			glm::vec4 new_pos = glm::vec4(v.position[0], v.position[1], v.position[2], 1.0f);
			new_pos = transform * new_pos;
			v.position[0] = new_pos.x;
			v.position[1] = new_pos.y;
			v.position[2] = new_pos.z;

			// normal
			glm::vec3 new_normal = normal_matrix * glm::vec3(v.normal[0], v.normal[1], v.normal[2]);
			new_normal = glm::normalize(new_normal);
			v.normal[0] = new_normal.x;
			v.normal[1] = new_normal.y;
			v.normal[2] = new_normal.z;
		}

		Meshes[mesh_index]->update_mesh(vertices, update_aabb);

		if(update_aabb)
		{
			update_model_aabb();
		}
	}
	//unneded? maybe just use a for loop?

	/// <summary>
	///     Applies a transformation matrix to a range of meshes.
	/// </summary>
	/// <remarks>
	///     Iterates from start to end mesh index and applies vertex transformation
	///     (position and normal update) to each mesh.
	/// </remarks>
	/// <param name="mesh_index_start">[in] First mesh index in range.</param>
	/// <param name="mesh_index_end">[in] Last mesh index in range (inclusive).</param>
	/// <param name="transform">[in] Transformation matrix to apply.</param>
	/// <param name="update_aabb">[in] If true, recalculates the axis-aligned bounding box after transformation (default = true).</param>
	void offset_mesh_vertices(const unsigned int mesh_index_start, const unsigned int mesh_index_end, const glm::mat4 transform, bool update_aabb = true)
	{
		if (Meshes.size() <= mesh_index_start)
		{
			LOG_ERROR("Mesh start index out of bounds in offset_mesh_vertices");
			return;
		}
		else if (Meshes.size() <= mesh_index_end)
		{
			LOG_ERROR("Mesh end index out of bounds in offset_mesh_vertices");
			return;
		}

		for(unsigned int i = mesh_index_start; i <= mesh_index_end; i++)
		{
			offset_mesh_vertices(i, transform, update_aabb);
		}

		if (update_aabb)
		{
			update_model_aabb();
		}
	}

	/// <summary>
	///     Applies a transformation matrix to all meshes in the container.
	/// </summary>
	/// <remarks>
	///     Calls the ranged offset_mesh_vertices overload using full mesh range (0 to size-1).
	/// </remarks>
	/// <param name="transform">[in] Transformation matrix to apply to all meshes.</param>
	/// <param name="update_aabb">[in] If true, recalculates the axis-aligned bounding box after transformation (default = true).</param>
	void offset_mesh_vertices(const glm::mat4 transform, bool update_aabb = true)
	{
		offset_mesh_vertices((unsigned int)0, (unsigned int)Meshes.size() - 1, transform, update_aabb);

		if (update_aabb)
		{
			update_model_aabb();
		}
	}

};

