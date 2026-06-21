cmake_minimum_required(VERSION 3.10)

project(NostalgiaManager CXX)

# Set build directories
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug) # Or Release, depending on testing needs

# --- Source Components ---

# Core Engine Library
add_subdirectory(NostalgiaManager/src/engine)

# Data Handling Library
add_subdirectory(NostalgiaManager/src/data)

# Add other necessary subdirectories here...

# Test setup (optional - often handled by CTest, but good to define the overall test group)
enable_testing()