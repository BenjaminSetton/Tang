cmake_minimum_required(VERSION 3.12)

project(Tang)

# Set the compiler flags: cpp17 and all warnings for MSVC
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    add_compile_options(/W4)
endif()

# Set the source directory
set(CMAKE_SOURCE_DIR ${PROJECT_SOURCE_DIR})

# Source files
set(SOURCES
	src/main.cpp
	src/shaders/shader.vert
	src/shaders/shader.frag
)

# Header files
set(HEADERS
	# TODO
)

# Set the output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out)

# Create the executable target for your application.
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# VULKAN
set(VULKAN_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/dependencies/Vulkan/include")
set(VULKAN_LIB "${CMAKE_SOURCE_DIR}/dependencies/Vulkan/lib/vulkan-1.lib")
target_link_libraries(${PROJECT_NAME} PRIVATE ${VULKAN_LIB})

# GLFW
set(GLFW_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/dependencies/GLFW/include")
set(GLFW_LIB "${CMAKE_SOURCE_DIR}/dependencies/GLFW/lib/glfw3.lib")
target_link_libraries(${PROJECT_NAME} PRIVATE ${GLFW_LIB})

# GLM
set(GLM_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/dependencies/GLM/include")

# Include Vulkan and GLFW header directories.
target_include_directories(${PROJECT_NAME} PRIVATE ${VULKAN_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${GLFW_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${GLM_INCLUDE_DIR})
