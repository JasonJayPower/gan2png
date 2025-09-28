# Require a minimum version of CMake.
cmake_minimum_required(VERSION 3.21)

# Set the name of the project.
project(gan2png
    VERSION 1.0
    DESCRIPTION "gan2png"
    LANGUAGES CXX)

# Automatically find all source files in the src directory.
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "src/*.cpp")

# Add an executable target using the discovered source files.
add_executable(${PROJECT_NAME} ${SRC_FILES})

# Specify the C++ standard required.
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

# Add include directories.
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/src)

# Link libraries (if any — update this as needed).
target_link_libraries(${PROJECT_NAME} PRIVATE)

