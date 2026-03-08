cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE GAMEPLAY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(gameplay STATIC ${GAMEPLAY_SOURCES})

target_include_directories(gameplay PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(gameplay PUBLIC cxx_std_20)
target_link_libraries(gameplay PUBLIC engine)

if(MSVC)
    target_compile_options(gameplay PRIVATE /W4)
else()
    target_compile_options(gameplay PRIVATE -Wall -Wextra -Wpedantic)
endif()
