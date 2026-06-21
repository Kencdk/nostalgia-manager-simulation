# Component CMakeLists.txt for Database Handler
add_library(Database STATIC 
    src/data/Database.cpp
)

target_include_directories(Database PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../.. # Include parent directory includes if necessary
)

# Add unit tests target linking against the library
add_executable(DatabaseTests src/data/test_database.cpp) 
target_link_libraries(DatabaseTests PRIVATE Database gtest gmock)
add_test(NAME DatabaseUnitTests COMMAND DatabaseTests)