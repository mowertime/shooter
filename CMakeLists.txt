cmake_minimum_required(VERSION 3.20)
project(Shooter VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optional Vulkan support
find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    message(STATUS "Vulkan SDK found: ${Vulkan_INCLUDE_DIRS}")
    add_compile_definitions(VULKAN_AVAILABLE)
else()
    message(STATUS "Vulkan SDK not found - renderer will use stub implementation")
endif()

# Threads required for the job system and world streaming
find_package(Threads REQUIRED)

add_subdirectory(engine)
add_subdirectory(gameplay)

# Main executable
add_executable(Shooter main.cpp)
target_link_libraries(Shooter PRIVATE gameplay engine Threads::Threads)
target_include_directories(Shooter PRIVATE ${CMAKE_SOURCE_DIR})

# Compiler warnings
if(MSVC)
    target_compile_options(Shooter PRIVATE /W4)
else()
    target_compile_options(Shooter PRIVATE -Wall -Wextra -Wpedantic)
endif()
