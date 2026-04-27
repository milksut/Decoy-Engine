#pragma once

#include "Shader.h"
#include "Globals.h"
#include "Some_functions.h"

#include <array>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

class game_object_basic_model
{
private:
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
			if (attrib_index >= VAO_MAX_ATTRIB_AMOUNT || attrib_index <= 2)
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

		//material properties for this mesh:
		unsigned int mesh_material_id = 0;

		unsigned int VAO, VBO_Mesh, EBO;

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
			instance_attributes[0] = { VBO_Mesh,0,0,3,0 };

			// vertex texture coords
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_data), (void*)offsetof(vertex_data, tex_coords));
			instance_attributes[1] = { VBO_Mesh,1,1,2,0 };

			// vertex  normals
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_data), (void*)offsetof(vertex_data, normal));
			instance_attributes[2] = { VBO_Mesh,2,2,3,0 };
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

			for (attribute id : instance_attributes)
				if (id.VBO) glDeleteBuffers(1, &id.VBO);
			
			glDeleteVertexArrays(1, &VAO);

			main_textures.clear();

			for(std::shared_ptr<class_region> ptr : shared_regions)
			{
				ptr.reset();
			}
		}

		///this function is dangerous! don't use if you don't know what you are doing
		///this function needs to be refactored, dont depend on it. //TODO:
	
		/// <summary>
		///     Updates mesh vertex, index, and texture data on the GPU.
		/// </summary>
		/// <param name="vertices">[in] New vertex data for the mesh.</param>
		/// <param name="indices">[in] New index data for the mesh.</param>
		/// <param name="textures">[in] New textures associated with the mesh.</param>
		/// <param name="use_dynamic_draw">[in] If true, uses dynamic draw for buffer updates.</param>
		void update_mesh(const std::vector<vertex_data> &vertices,const std::vector<unsigned int> &indices,
			const std::vector<Texture>& textures, bool use_dynamic_draw = false)
		{
			this->main_vertices = vertices;
			this->main_indices = indices;
			this->main_textures = textures;

			glBindVertexArray(VAO);
			

			if (vertices.size() > 0)
			{	
				glBindBuffer(GL_ARRAY_BUFFER, VBO_Mesh);

				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex_data), nullptr,
					use_dynamic_draw ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);//orphan

				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex_data), &vertices[0],
					use_dynamic_draw ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);//new
			}
			else
			{
				LOG_WARNING("Mesh - a mesh updated with empty vertices, is this intentional?");
			}


			if (indices.size() > 0)
			{
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
			if (attrib_index >= VAO_MAX_ATTRIB_AMOUNT || attrib_index <= 2)
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
		///     Most VAOs support ~16 attribute slots (0–2 reserved for mesh data). Each slot can store up to 4 floats.
		///     If more is needed, multiple attribute indices are used.
		///     Ensure sufficient vector capacity for all instance data; otherwise undefined behavior may occur.
		/// </summary>
		/// <param name="attrib_size">[in] Size of the attribute in floats.</param>
		/// <param name="attrib_index">[in] Starting attribute index in the VAO.</param>
		/// <param name="loop_instance">[in] Instance divisor (default = 1).</param>
		/// <returns>0 on success, -1 on failure.</returns>
		int add_instance_buffer(int attrib_size, int attrib_index, int loop_instance = 1)
		{
			if(shared_regions.empty())
				return -1; //no regions to create buffer for

			if (!can_override_vbo && instance_attributes[attrib_index].VBO != 0)
				return -1; //attribute already filled

			int index_amount = (attrib_size / 4) + (attrib_size%4 ==0? 0:1);

			if (attrib_index + index_amount -1  >= VAO_MAX_ATTRIB_AMOUNT || attrib_index <= 2)
				return -1; // Invalid or mesh's attribute index
			
			int wanted_amount = shared_regions.back()->offset_in_numbers + shared_regions.back()->size_in_number;

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
			std::shared_ptr<class_region> region, float data_offset_by_attrib_size = 0)
		{
			if (attrib_index >= VAO_MAX_ATTRIB_AMOUNT || attrib_index <= 2)
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

					if(attrib.attrib_start_index <=2)
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
				glDrawArraysInstanced(GL_POINTS, 0, main_vertices.size(), amount);
			}
			draw_call_count++;
		}
	};

	struct Mesh_Childs
	{
		std::vector<std::shared_ptr<Mesh>> Meshes;
		std::vector<Mesh_Childs*> Childs;
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
			Mesh_Childs child_mesh;
			parent_mesh.Childs.push_back(&child_mesh);
			process_node(node->mChildren[i], scene, child_mesh, path);
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

				for (int j = 0; j < material->GetTextureCount(assimp_type); j++)
				{
					Texture* texture;
					aiString str;

					//assimp_type - Texture type, j - Index of the texture to get, &str - output path
					material->GetTexture(assimp_type, j, &str);

					std::string directory = path.substr(0, path.find_last_of('\\'));
					str.Set((directory + '\\' + str.C_Str()).c_str());

					texture = Texture_slots::get_loaded_texture(str.C_Str());//check if texture was loaded before
					if (texture != nullptr)
					{
						textures.push_back(*texture);
					}
					else
					{
						// data returned by load_image but unused here
						int unused_data1 = 0, unused_data2 = 0, unused_data3 = 0;
						texture = new Texture();

						texture->id = load_image(str.C_Str(), unused_data1, unused_data2, unused_data3);
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
					mat_props.illumination_model = illum;
			}
			if (!Material_slots::init_flag)
			{
				Material_slots::init_material_slots();
			}
			uses_material_flag = true;
		}

		std::shared_ptr<Mesh> mesh_ptr = std::make_shared<Mesh>(vertices,indices,textures);
		mesh_ptr->mesh_material_id = uses_material_flag ? Material_slots::register_material(mat_props) : 0;
		return mesh_ptr;
	}

public:

	std::vector<std::shared_ptr<Mesh>> Meshes;//you cant use copy constructor or assignment operator because of Mesh class
	//so you need to manage meshes throut pointers becouse vectors copy elements when resized

	Mesh_Childs root;

	/// <summary>
	///     Creates a new mesh and adds it to the mesh collection.
	/// </summary>
	/// <param name="vertices">[in] Vertex data of the mesh.</param>
	/// <param name="indices">[in] Index data of the mesh.</param>
	/// <param name="textures">[in] Textures associated with the mesh.</param>
	void add_mesh(const std::vector<vertex_data>& vertices,const std::vector<unsigned int>& indices,const std::vector<Texture>& textures)
	{
		Meshes.push_back(std::make_shared<Mesh>(vertices, indices, textures));
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
	///     VAO typically supports ~16 attribute slots (0–2 reserved for mesh data).
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
		std::shared_ptr<class_region> region, float data_offset_by_attrib_size = 0)
	{
		for (std::shared_ptr<Mesh> pointer : Meshes)
		{
			pointer->load_instance_buffer(data, amount_in_attrib_size, attrib_index, region, data_offset_by_attrib_size);
		}
	}

	/// <summary>
	///     Imports a 3D model from file using Assimp and builds the mesh hierarchy.
	/// </summary>
	/// <remarks>
	///     Loads scene data, then processes the root node recursively into engine meshes.
	/// </remarks>
	/// <param name="path">[in] File path of the model to import.</param>
	void import_model_from_file(std::string path)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOG_ERROR("Assimp error: %s", importer.GetErrorString());
		}
		else
		{
			process_node(scene->mRootNode, scene, root,path);
		}

	}
};

