# Component CMakeLists.txt for MatchEngine
add_library(MatchEngine STATIC 
    src/engine/MatchEngine.cpp
)

target_include_directories(MatchEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../.. # Include parent directory includes if necessary
)

# Add unit tests target linking against the library
add_executable(MatchEngineTests src/engine/test_match_engine.cpp) 
target_link_libraries(MatchEngineTests PRIVATE MatchEngine gtest gmock)
add_test(NAME MatchEngineUnitTests COMMAND MatchEngineTests)