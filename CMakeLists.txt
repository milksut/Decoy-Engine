cmake_minimum_required (VERSION 3.15)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project ("Decoy_Engine")

# Require C++17 for EnTT and modern headers
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. BASIC SETTINGS
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/external")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) 

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    # Enable PDB generation for all configs
    add_compile_options(/Zi)
    add_link_options(/DEBUG)

    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} /O2 /arch:AVX2 /GL")
else()
    # GCC/Clang
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -march=native -g -flto")
endif()

# Use after downloading dependencies to prevent checking each time
set(FETCHCONTENT_FULLY_DISCONNECTED ON)

include(FetchContent)

# -------------------------------------------------------------------
# 2. LIBRARIES (FETCHCONTENT)
# -------------------------------------------------------------------

# --- GLAD (OpenGL Installer) ---
# We are pulling GLAD from this repository, which is compatible with CMake.
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG v0.1.36 # Most stable version
)
# GLAD Settings: We only specify the GL version.
set(GLAD_INSTALL OFF CACHE BOOL "" FORCE)

set(GLAD_PROFILE "core")
set(GLAD_API "gl=3.3")

FetchContent_MakeAvailable(glad)

# --- GLFW ---
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# --- GLM ---
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# --- EnTT (Entity-Component-System) ---
FetchContent_Declare(
  entt
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG v3.16.0
)
FetchContent_MakeAvailable(entt)

add_library(entt INTERFACE)
target_include_directories(entt INTERFACE ${entt_SOURCE_DIR}/single_include)

# --- ASSIMP ---
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG v5.3.1
)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_VIEW OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)

# --- STB (Image Loading) ---
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
# -------------------------------------------------------------------
# 3. YOUR FILES
# -------------------------------------------------------------------

# Only add files that you have created yourself here.
add_executable(${PROJECT_NAME} 
    "model_loading_test.cpp"
    "stb_setup.cpp"
)

# -------------------------------------------------------------------
# 4. LINKING & INCLUDES
# -------------------------------------------------------------------

# By adding `glad::glad`, we include all OpenGL functions in the project.
target_link_libraries(${PROJECT_NAME} PRIVATE 
    glad
    glfw 
    glm::glm 
    assimp::assimp
    stb
    entt
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${stb_SOURCE_DIR}
    "Headers"     # This allows the system to directly find the files inside the headers folder.
    .
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")