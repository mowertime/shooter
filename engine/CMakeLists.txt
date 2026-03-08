cmake_minimum_required(VERSION 3.20)

# Collect all engine source files
file(GLOB_RECURSE ENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(engine STATIC ${ENGINE_SOURCES})

target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..   # exposes engine/ to consumers
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(engine PUBLIC cxx_std_20)

find_package(Threads REQUIRED)
target_link_libraries(engine PUBLIC Threads::Threads)

if(Vulkan_FOUND)
    target_link_libraries(engine PUBLIC Vulkan::Vulkan)
    target_include_directories(engine PUBLIC ${Vulkan_INCLUDE_DIRS})
endif()

if(MSVC)
    target_compile_options(engine PRIVATE /W4)
else()
    target_compile_options(engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
