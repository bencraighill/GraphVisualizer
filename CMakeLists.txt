cmake_minimum_required(VERSION 3.16)
project(GraphVisualizer)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Collect ImGui source files
set(IMGUI_SOURCES
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
)

# Glad source
set(GLAD_SOURCES
    vendor/glad/src/glad.c
)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Disable GLFW examples")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Disable GLFW tests")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Disable GLFW docs")
add_subdirectory(vendor/glfw)

# Add yaml-cpp
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Disable yaml-cpp tests")
add_subdirectory(vendor/yaml-cpp)
add_subdirectory(vendor/gperftools)

# Add executable with main.cpp + ImGui + Glad
add_executable(GraphVisualizer
    src/main.cpp
    ${IMGUI_SOURCES}
    ${GLAD_SOURCES}
)

# Include directories
target_include_directories(GraphVisualizer PRIVATE
    vendor/glad/include
    vendor/imgui
    vendor/imgui/backends
    vendor/gperftools/src
)

# Link libraries
target_link_libraries(GraphVisualizer PRIVATE
    glfw
    yaml-cpp
    tcmalloc
    profiler
)

# Link OpenGL (platform-specific)
if(WIN32)
    target_link_libraries(GraphVisualizer PRIVATE opengl32)
elseif(APPLE)
    find_library(OpenGL_LIBRARY OpenGL)
    target_link_libraries(GraphVisualizer PRIVATE ${OpenGL_LIBRARY})
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(GraphVisualizer PRIVATE OpenGL::GL)
endif()

# Visual Studio: set working directory to project root
if(MSVC)
    set_target_properties(GraphVisualizer PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()

if(UNIX)
    find_package(X11 REQUIRED)
    target_link_libraries(GraphVisualizer PRIVATE ${X11_LIBRARIES})
endif()
