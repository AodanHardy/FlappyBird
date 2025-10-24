cmake_minimum_required(VERSION 3.20)
project(FlappyBird CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch SFML 2.6.1 from source (works on macOS/Windows/Linux)
include(FetchContent)
FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.1
)
# speed-ups
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SFML)

add_executable(FlappyBird src/main.cpp)
target_link_libraries(FlappyBird PRIVATE sfml-graphics sfml-window sfml-system sfml-audio)

# Copy assets next to the binary so relative paths work
add_custom_command(TARGET FlappyBird POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:FlappyBird>/assets
)