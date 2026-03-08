cmake_minimum_required(VERSION 3.20)

# Minimal test framework – we use a single-header catch2-like approach
# but without external dependencies for simplicity.

add_executable(ShooterTests
    test_main.cpp
    test_ecs.cpp
    test_physics.cpp
    test_ai.cpp
    test_world.cpp
    test_audio.cpp
    test_ballistics.cpp
    test_threading.cpp
)

target_link_libraries(ShooterTests PRIVATE ShooterEngineLib)
target_include_directories(ShooterTests PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME EngineTests COMMAND ShooterTests)
