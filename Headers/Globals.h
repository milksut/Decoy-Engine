#pragma once

//defines----------------------------------------------------------------------------------------
#define TEXTURE_SLOTS 48 //most modern gpu can supprot more than 48 but 
//openGl 3.3 garantees minumum 48 texture units and its usualy enough for most cases

#define VAO_MAX_ATTRIB_AMOUNT 16 //max attrib indexs per vaos

#define OPENGL_VERSION_MAJOR 3
#define OPENGL_VERSION_MINOR 3
//end of defines---------------------------------------------------------------------------------

//Enums -----------------------------------------------------------------------------------------
enum Key_state
{
	Idle,    //While the key is free
	Pressed,// When the key is first pressed
	Hold,   // While the key is hold
	Released// When the key is first released
};
//end of enums-----------------------------------------------------------------------------------



//Includes----------------------------------------------------------------------------------------
#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>
#include <stack>
#include <functional>
#include <unordered_map>

#include <iostream>
#include <ctime>
#include <fstream>
#include <cstdarg>

#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>


#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#define STAT_STRUCT _stat
#define STAT_FUNC   _stat

#else
#include <unistd.h>
#define MKDIR(path) mkdir(path, 0755)
#define STAT_STRUCT stat
#define STAT_FUNC   stat
#endif
//end of includes---------------------------------------------------------------------------------



//// X-Macros----------------------------------------------------------------------------------------
	//TEX_TYPE MACRO------------------------------------------------------------------------------
	#define TEX_TYPES \
		X(DIFFUSE, aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR) \
		X(NORMAL , aiTextureType_NORMALS, aiTextureType_NONE) \
		X(SPECULAR , aiTextureType_SPECULAR, aiTextureType_NONE) \

	enum TextureType
	{
		#define X(name, assimp_name, second_assimp_name) name,
		TEX_TYPES
		#undef X

		Tex_type_amount//always leave in bottom. Used to measure how many texture types are there
	};

	static const std::string Tex_Types_Names[] = 
	{
		#define X(name, assimp_name, second_assimp_name) #name,
		TEX_TYPES
		#undef X
	};

	//end of TEX_TYPE MACRO-----------------------------------------------------------------------

	//Basic_input_channels MACRO ------------------------------------------------------------------
	#define Basic_input_channels\
		X(Keyboard_input) \
		X(Mouse_input) \
		X(Combo_input)

	enum Input_channel
	{
		#define X(name) name,
		Basic_input_channels
		#undef X

		Input_channel_amount//always leave in bottom. Used to measure how many input channels are there
	};

	static const std::string Input_channel_names[] =
	{
		#define X(name) #name,
		Basic_input_channels
		#undef X
	};

	//end of Basic_input_channels MACRO -----------------------------------------------------------
//end of X-Macros---------------------------------------------------------------------------------



//Global variables-------------------------------------------------------------------------------
	unsigned int draw_call_count = 0; //to track how many draw calls are made per frame
	unsigned int tick_count = 0; //to track how many ticks are made per second
//end of global variables------------------------------------------------------------------------



//Structs----------------------------------------------------------------------------------------
struct Texture {
	unsigned int id;
	TextureType type;
	std::string path;
};

struct vertex_data
{
	float position[3];
	float tex_coords[2];
	float normal[3];
};

struct Light {
	bool has_a_source;
	glm::vec3 light_pos;
	glm::vec3 light_target;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;

	float cos_soft_cut_off_angle;
	float cos_hard_cut_off_angle;

	float constant;
	float linear;
	float quadratic;
};

struct class_region //VBO regions given to classes and data inside them
{
	int offset_in_numbers;//how many meshes can be drawn before this region
	int size_in_number;//how many meshes can be drawn using this region

	std::vector<std::shared_ptr<float>> data_ptrs;//datas for this region, vector index -> attribute index, and pointer for data
	std::vector<unsigned int> data_amount;//amount of floats in data_ptr
};

struct material_properties
{
	bool uses_material;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float shininess;
	glm::vec3 emission; //object emits light
	float opacity;
	float index_of_refraction; //how much light bends when entering the material
	int illumination_model; //illumination model used by the material
};

struct Input_key
{
	Input_channel type;
	int code;

	bool operator==(const Input_key& other) const
	{
		return type == other.type && code == other.code;
	}
};

//For events to fire when more than 1 key is pressed, we need to check each combo
struct Key_combo
{
	std::vector<Input_key> keys;
};
//end of structs---------------------------------------------------------------------------------



//Functions--------------------------------------------------------------------------------------
bool folderExists(const std::string& path) {
	struct STAT_STRUCT info;
	if (STAT_FUNC(path.c_str(), &info) != 0) return false;
	return (info.st_mode & S_IFDIR) != 0;
}

bool createFolder(const std::string& path) {
	return MKDIR(path.c_str()) == 0;
}
//end of functions-------------------------------------------------------------------------------



//Namespaces--------------------------------------------------------------------------------------
namespace Texture_slots {
	std::vector<Texture> loaded_textures; //to avoid loading duplicated textures, includes all textures bound or unbound.
	std::stack<int> deleted_textures;

	unsigned int bound_slots[TEXTURE_SLOTS] = { 0 };//active texture slots like GL_TEXTURE0,for index 0 means GL_TEXTURE0 and var is texture id

	unsigned int slot_age[TEXTURE_SLOTS] = { 0 };//to track usage age of slots for replacement if needed

	void new_texture_loaded(const Texture& texture)
	{
		if(loaded_textures.capacity()<100)
			loaded_textures.reserve(100);

		if(deleted_textures.empty())
			loaded_textures.push_back(texture);
		else
		{
			loaded_textures[deleted_textures.top()] = texture;
			deleted_textures.pop();
		}

	}

	Texture* get_loaded_texture(const std::string& path)
	{
		for (Texture& tex : loaded_textures)
		{
			if (tex.path == path)
			{
				return &tex;
			}
		}
		return nullptr;
		
	}

	void age_slots()
	{
		slot_age[0] += bound_slots[0] <= 0 ? 0 : 2; //becouse slot 0 is mostly used during other texture bindings we try to free it as much as possible
		for (int i = 1; i < TEXTURE_SLOTS; ++i) {
			if (bound_slots[i] != 0) {
				slot_age[i]++;
			}
		}
	}

	int get_oldest_slot()
	{
		int oldest_index = 0;
		unsigned int max_age = 0;
		for (int i = 0; i < TEXTURE_SLOTS; ++i) {
			if (slot_age[i] > max_age) {
				max_age = slot_age[i];
				oldest_index = i;
			}
		}
		return oldest_index;
	}

	int get_index_of_bound_slot(const unsigned int texture_id)
	{
		for (int i = 0; i < TEXTURE_SLOTS; ++i) {
			if (bound_slots[i] == texture_id) {
				return i;
			}
		}
		return -1;
	}

	int get_last_empty_space()//because slot 0 is mostly used during other texture bindings we start searching from last slot
	{
		for (int i = TEXTURE_SLOTS - 1; i >= 0; --i) {
			if (bound_slots[i] <= 0) {
				return i;
			}
		}
		return -1;
	}

	void unbound_texture(const int slot_index)
	{
		if (slot_index >= 0 && slot_index < TEXTURE_SLOTS)
		{
			glActiveTexture(GL_TEXTURE0 + slot_index);
			glBindTexture(GL_TEXTURE_2D, 0);
			bound_slots[slot_index] = 0;
			slot_age[slot_index] = 0;
		}
	}

	int bound_texture(const unsigned int texture_id)
	{
		int slot_index = get_index_of_bound_slot(texture_id);
		if (slot_index != -1) {
			// Texture is already bound, reset its age
			slot_age[slot_index] = 0;
			return slot_index;
		}
		slot_index = get_last_empty_space();

		if (slot_index == -1)
		{
			// No available texture slots, replace the oldest one
			slot_index = get_oldest_slot();
			unbound_texture(slot_index);
		}

		glActiveTexture(GL_TEXTURE0 + slot_index);
		glBindTexture(GL_TEXTURE_2D, texture_id);
		bound_slots[slot_index] = texture_id;
		slot_age[slot_index] = 0; // Reset age since it's just been used
		return slot_index;
	}

	void delete_texture(const unsigned int texture_id)
	{
		const int slot_index = get_index_of_bound_slot(texture_id);
		if ( slot_index != -1)
		{
			unbound_texture(slot_index);
		}

		for (int i = 0; i < loaded_textures.size(); ++i)
		{
			if (loaded_textures[i].id == texture_id)
			{
				deleted_textures.push(i);
			}
		}


	}

}

namespace Shader_variables
{
	unsigned int current_shader_id = 0;
}

namespace Event_management
{
	unsigned int event_id_counter = 0;

	//Fill this part yourself
	enum Event_type
	{
		Null,
		Mouse_moved, Mouse_scrolled,
		Mouse_button_pressed, Mouse_button_hold, Mouse_button_released,
		Keyboard_button_pressed, Keyboard_button_hold, Keyboard_button_released,
		Combo_button_pressed, Combo_button_hold, Combo_button_released,

		LAST_EVENT_TYPE//always leave in bottom. Used to measure how many event types there are
	};

	enum class Event_timing { Immediate, Queued };
	enum class Event_scope { Targeted, Announcement };

	//forward declaration
	class Event;

	//must be implamented by anything that wants to use events,
	//used when subscribeing a channel on event manager
	//can be lambada, like if you don't want to bind it to a class
	using Event_receiver_shared = std::shared_ptr<std::function<void(const Event&)>>;

	//doesn't keep the receiver alive
	using Event_receiver_weak = std::weak_ptr<std::function<void(const Event&)>>;

	template<typename T>
	Event_receiver_shared make_receiver(T&& lambda)
	{
		return std::make_shared<std::function<void(const Event&)>>(std::forward<T>(lambda));
	}

	//base event class, not designed to use at it is,
	//create subclases to use it
	class Event
	{

	public:
		const unsigned int id = event_id_counter++;

		Event_timing timing = Event_timing::Immediate;
		Event_scope scope = Event_scope::Announcement;
		Event_type type = Event_type::Null;
		Event_receiver_weak target_receiver;

		bool is_alive = true;

		Event(const Event_timing timing, const Event_type type)
		{
			this->timing = timing;
			this->type = type;
		}
		Event(const Event_timing timing, const Event_receiver_shared& target_receiver, const Event_type type)
		{
			this->timing = timing;
			this->target_receiver = target_receiver;
			this->scope = Event_scope::Targeted;
			this->type = type;
		}

		virtual void execute() = 0;
		virtual ~Event() {}

	};





}

namespace Logger
{
	enum class LogLevel
	{
		INFO,
		WARNING,
		ERROR,
		DEBUG,
		FATAL
	};

	class Logger_class
	{
	public:
		LogLevel console_log_level = LogLevel::INFO;
		void log(LogLevel level, const char* file, int line, const char* format, ...) {
			char buffer[1024];
			va_list args;
			va_start(args, format);
			vsnprintf(buffer, sizeof(buffer), format, args);
			va_end(args);
			time_t now = time(0);
			char* dt = ctime(&now);
			std::string dtStr(dt);
			dtStr.pop_back();
			if (level >= console_log_level)
			{
				std::cout << getLogLevelColor(level) << dtStr << " [" << getLogLevelString(level) << "] " << "(" << file << ":" << line << ") " << buffer << "\033[0m" << std::endl;
			}
			logFile << dtStr << " [" << getLogLevelString(level) << "] " << "(" << file << ":" << line << ") " << buffer << std::endl;
		}

		static Logger_class& getInstance() {
			static Logger_class instance;
			return instance;
		}

	private:

		std::string getLogLevelString(LogLevel level) {
			switch (level) {
			case LogLevel::INFO: return "INFO";
			case LogLevel::WARNING: return "WARNING";
			case LogLevel::ERROR: return "ERROR";
			case LogLevel::DEBUG: return "DEBUG";
			case LogLevel::FATAL: return "FATAL";
			default: return "UNKNOWN";
			}
		}

		std::ofstream logFile;
		Logger_class() {

			if(!folderExists("Logs"))
			{
				if(!createFolder("Logs"))
				{
					std::cerr << "Failed to create Logs folder!" << std::endl;
					return;
				}
				else
				{
					std::cout << "Logs folder created successfully." << std::endl;
				}
			}

			time_t now = time(0);
			tm* ltm = localtime(&now);

			std::string filename = "Logs/log_" +
				std::to_string(1900 + ltm->tm_year) + "-" +
				std::to_string(1 + ltm->tm_mon) + "-" +
				std::to_string(ltm->tm_mday) + "_" +
				std::to_string(ltm->tm_hour) + "-" +
				std::to_string(ltm->tm_min) + "-" +
				std::to_string(ltm->tm_sec) + ".txt";

			logFile.open(filename);
			if (!logFile.is_open()) {
				std::cerr << "Failed to open log file!" << std::endl;
			}
		}

		std::string getLogLevelColor(LogLevel level) {
			switch (level) {
			case LogLevel::INFO: return "\033[32m"; // Green
			case LogLevel::WARNING: return "\033[33m"; // Yellow
			case LogLevel::ERROR: return "\033[31m"; // Red
			case LogLevel::DEBUG: return "\033[34m"; // Blue
			case LogLevel::FATAL: return "\033[35m"; // Magenta
			default: return "\033[0m"; // Reset
			}
		}

	};

	#define LOG_ERROR(format, ...) Logger::Logger_class::getInstance().log(Logger::LogLevel::ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
	#define LOG_WARNING(format, ...) Logger::Logger_class::getInstance().log(Logger::LogLevel::WARNING, __FILE__, __LINE__, format, ##__VA_ARGS__)
	#define LOG_INFO(format, ...) Logger::Logger_class::getInstance().log(Logger::LogLevel::INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
	#define LOG_DEBUG(format, ...) Logger::Logger_class::getInstance().log(Logger::LogLevel::DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
	#define LOG_FATAL(format, ...) Logger::Logger_class::getInstance().log(Logger::LogLevel::FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__)

	#define GL_ERROR_CASE(err) case err: return #err
	const char* getGLErrorString(GLenum error)
	{
		switch (error)
		{
			GL_ERROR_CASE(GL_NO_ERROR);
			GL_ERROR_CASE(GL_INVALID_ENUM);
			GL_ERROR_CASE(GL_INVALID_VALUE);
			GL_ERROR_CASE(GL_INVALID_OPERATION);
			GL_ERROR_CASE(GL_OUT_OF_MEMORY);
			GL_ERROR_CASE(GL_INVALID_FRAMEBUFFER_OPERATION);
		default: return "UNKNOWN_ERROR";
		}
	}

	bool printed_debug_mode_warning = false;//to avoid filling the log with this warning
	void checkGLError(const char* location)
	{
	#ifdef _DEBUG
		GLenum error;
		while ((error = glGetError()) != GL_NO_ERROR)
		{
			LOG_ERROR("OpenGL Error at %s : %s", location, getGLErrorString(error));
		}
	#else
		if(!printed_debug_mode_warning)
		{
			LOG_WARNING("checkGLError is only active in debug mode, skipping check at %s", location);
			printed_debug_mode_warning = true;
		}
		
	#endif
		
	}
}
//end of namespaces-------------------------------------------------------------------------------
