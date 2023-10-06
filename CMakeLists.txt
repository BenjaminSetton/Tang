cmake_minimum_required(VERSION 3.12)

project(TANG)

# Set the compiler flags: cpp17 and all warnings for MSVC
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    add_compile_options(/W4)
endif()

# Set the source directory
set(CMAKE_SOURCE_DIR ${PROJECT_SOURCE_DIR})

# Force debug and release configurations only, no ugly MinSizeRel or anything like that...
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configurations" FORCE)

# Header files
set(HEADERS
	src/tang.h
	src/asset_loader.h
	src/asset_types.h
	src/renderer.h
	src/vertex_type.h
	src/main_window.h
	src/input_manager.h
	
	src/camera/base_camera.h
	src/camera/freefly_camera.h
	
	src/data_buffer/buffer.h
	src/data_buffer/vertex_buffer.h
	src/data_buffer/index_buffer.h
	src/data_buffer/uniform_buffer.h
	
	src/cmd_buffer/command_buffer.h
	src/cmd_buffer/primary_command_buffer.h
	src/cmd_buffer/secondary_command_buffer.h
	
	src/descriptors/write_descriptor_set.h
	src/descriptors/descriptor_pool.h
	src/descriptors/descriptor_set.h
	src/descriptors/descriptor_allocator.h
	src/descriptors/set_layout/set_layout.h
	src/descriptors/set_layout/set_layout_cache.h
	src/descriptors/set_layout/set_layout_summary.h
	
	src/utils/logger.h
	src/utils/sanity_check.h
	src/utils/uuid.h
	src/utils/key_types.h
)

# Source files
set(SOURCES
	src/tang.cpp
	src/main.cpp
	src/asset_loader.cpp
	src/renderer.cpp
	src/main_window.cpp
	src/input_manager.cpp
	
	src/camera/base_camera.cpp
	src/camera/freefly_camera.cpp
	
	src/data_buffer/buffer.cpp
	src/data_buffer/vertex_buffer.cpp
	src/data_buffer/index_buffer.cpp
	src/data_buffer/uniform_buffer.cpp
	
	src/cmd_buffer/command_buffer.cpp
	src/cmd_buffer/primary_command_buffer.cpp
	src/cmd_buffer/secondary_command_buffer.cpp
	
	src/descriptors/write_descriptor_set.cpp
	src/descriptors/descriptor_set.cpp
	src/descriptors/descriptor_pool.cpp
	src/descriptors/descriptor_allocator.cpp
	src/descriptors/set_layout/set_layout.cpp
	src/descriptors/set_layout/set_layout_cache.cpp
	src/descriptors/set_layout/set_layout_summary.cpp
	
	# Shaders
	src/shaders/shader.vert
	src/shaders/shader.frag
	
	src/utils/uuid.cpp
	src/utils/logger.cpp
)

# Set the output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out)

# Create the executable target for your application.
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# DEPENDENCIES

# VULKAN
set(VULKAN_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src/dependencies/Vulkan/include")
set(VULKAN_LIB "${CMAKE_SOURCE_DIR}/src/dependencies/Vulkan/lib/vulkan-1.lib")
target_link_libraries(${PROJECT_NAME} PRIVATE ${VULKAN_LIB})

# GLFW
set(GLFW_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src/dependencies/GLFW/include")
set(GLFW_LIB "${CMAKE_SOURCE_DIR}/src/dependencies/GLFW/lib/glfw3.lib")
target_link_libraries(${PROJECT_NAME} PRIVATE ${GLFW_LIB})

# GLM
set(GLM_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src/dependencies/GLM/include")

# stb
set(STB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src/dependencies/stb/include")

# assimp
set(ASSIMP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src/dependencies/assimp/include")
set(ASSIMP_LIB "${CMAKE_SOURCE_DIR}/src/dependencies/assimp/lib/Debug/assimp-vc143-mtd.lib")
target_link_libraries(${PROJECT_NAME} PRIVATE ${ASSIMP_LIB})

# Include dependency header directories.
target_include_directories(${PROJECT_NAME} PRIVATE ${VULKAN_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${GLFW_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${GLM_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${STB_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${ASSIMP_INCLUDE_DIR})

# Copy the assimp DLL into the exe's directory
set(ASSIMP_DLL "${CMAKE_SOURCE_DIR}/src/dependencies/assimp/dll/Debug/assimp-vc143-mtd.dll")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ASSIMP_DLL}"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)