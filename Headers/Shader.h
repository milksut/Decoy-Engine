#pragma once
#ifndef SHADER_H
#define SHADER_H

#include "Globals.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <iostream>

using namespace Shader_variables;

class Shader
{
private:
    std::unordered_map<std::string, int> uniform_locations;

public:
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath)
    {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        // ensure stream objects can throw exceptions:
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            LOG_ERROR("ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: %s", e.what());
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        
        
        //This part makes every shader to use our Materials UBO that defined at globals,
        GLuint blockIndex = glGetUniformBlockIndex(ID, "Material_block");
        if(blockIndex == GL_INVALID_INDEX)
        {
            LOG_INFO("Shader with id: %d dont have a Material block, skipping material UBO Binding");
		}
        else
        {
            if(!Material_slots::init_flag)
            {
                Material_slots::init_material_slots();
		    }
            glUniformBlockBinding(ID, blockIndex, Material_slots::ubo_slot);
            Logger::checkGLError("after adding Material ubo");
        }


    }
    // adding geometry shader is optional
    // ------------------------------------------------------------------------
    void add_geometry_shader(const char* geometryPath)
    {
        std::string geoCode;
        std::ifstream geoShaderFile;

        geoShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            // open files
            geoShaderFile.open(geometryPath);
            std::stringstream geoShaderStream;
            // read file's buffer contents into streams
            geoShaderStream << geoShaderFile.rdbuf();
            // close file handlers
            geoShaderFile.close();
            // convert stream into string
            geoCode = geoShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            LOG_ERROR("ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: %s", e.what());
        }

        const char* geoShaderCode = geoCode.c_str();

        unsigned int geometry;

        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry, 1, &geoShaderCode, NULL);
        glCompileShader(geometry);
        checkCompileErrors(geometry, "GEOMETRY");

        glAttachShader(ID, geometry);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        glDeleteShader(geometry);
    }
	// this function lets you bind UBO's to shader that don crated at init
    // ------------------------------------------------------------------------
    int bind_UBO(const char* block_name, unsigned int UBO_Slot)
    {
        GLuint blockIndex = glGetUniformBlockIndex(ID, block_name);
        if (blockIndex == GL_INVALID_INDEX)
        {
            LOG_ERROR("Shader with id: %d dont have a block named: %s", ID, block_name);
            return -1;
        }
        else
        {
            glUniformBlockBinding(ID, blockIndex, UBO_Slot);
            Logger::checkGLError("after adding ubo:");
            return 0;
        }
	}
    // activate the shader
    // ------------------------------------------------------------------------
    void use()
    {
        current_shader_id = ID;
        glUseProgram(ID);
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string& name, bool value)
    {
        if (current_shader_id != ID)
            use();

		if(uniform_locations.find(name) == uniform_locations.end())
        {
            uniform_locations[name] = glGetUniformLocation(ID, name.c_str());
        }
        glUniform1i(uniform_locations[name], (int)value);
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string& name, int value) 
    {
        if (current_shader_id != ID)
            use();

        if (uniform_locations.find(name) == uniform_locations.end())
        {
            uniform_locations[name] = glGetUniformLocation(ID, name.c_str());
        }
        glUniform1i(uniform_locations[name], value);
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string& name, int value[], int amount)
    {
        if (current_shader_id != ID)
            use();

        if (uniform_locations.find(name) == uniform_locations.end())
        {
            uniform_locations[name] = glGetUniformLocation(ID, name.c_str());
        }
        glUniform1iv(uniform_locations[name], amount, value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string& name, float value)
    {
        if (current_shader_id != ID)
            use();

        if (uniform_locations.find(name) == uniform_locations.end())
        {
            uniform_locations[name] = glGetUniformLocation(ID, name.c_str());
        }
        glUniform1f(uniform_locations[name], value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string& name, float value[], int amount)
    {
        if (current_shader_id != ID)
            use();

        if (uniform_locations.find(name) == uniform_locations.end())
        {
            uniform_locations[name] = glGetUniformLocation(ID, name.c_str());
        }
        glUniform1fv(uniform_locations[name], amount, value);
    }
    // ------------------------------------------------------------------------
    void setMatrix4fv(const std::string& name, const float* value)
    {
        if (current_shader_id != ID)
            use();

        if (uniform_locations.find(name) == uniform_locations.end())
        {
            uniform_locations[name] = glGetUniformLocation(ID, name.c_str());
        }
        glUniformMatrix4fv(uniform_locations[name], 1, GL_FALSE, value);
    }
    // ------------------------------------------------------------------------
    void setVec4(const std::string& name, const glm::vec4& value)
    {
        if (current_shader_id != ID)
            use();

        if (uniform_locations.find(name) == uniform_locations.end())
        {
            uniform_locations[name] = glGetUniformLocation(ID, name.c_str());
        }
        glUniform4fv(uniform_locations[name], 1, glm::value_ptr(value));
    }
    // ------------------------------------------------------------------------
    void setVec4(const std::string& name, const glm::vec4 value[], int amount)
    {
        if (current_shader_id != ID)
            use();

        if (uniform_locations.find(name) == uniform_locations.end())
        {
            uniform_locations[name] = glGetUniformLocation(ID, name.c_str());
        }
        glUniform4fv(uniform_locations[name], amount, glm::value_ptr(value[0]));
    }
    // ------------------------------------------------------------------------
    void setVec3(const std::string& name, const glm::vec3& value)
    {
        if (current_shader_id != ID)
            use();

        if (uniform_locations.find(name) == uniform_locations.end())
        {
            uniform_locations[name] = glGetUniformLocation(ID, name.c_str());
        }
        glUniform3fv(uniform_locations[name], 1, glm::value_ptr(value));
    }

private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                LOG_ERROR("ERROR::SHADER_COMPILATION_ERROR of type: %s\n%s", type.c_str(), infoLog);
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                LOG_ERROR("ERROR::PROGRAM_LINKING_ERROR of type: %s\n%s", type.c_str(), infoLog);
            }
        }
    }
};
#endif