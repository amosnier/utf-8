cmake_minimum_required(VERSION 3.25)

project(utf-8
        VERSION 1.0
        DESCRIPTION "Encode/decode a std::range to/from UTF-8"
        LANGUAGES CXX)

# Only enable testing if this library is not included from a separate project.
string(COMPARE EQUAL "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}" UTF_8_ENABLE_TESTING)

add_subdirectory(src)

if (UTF_8_ENABLE_TESTING)
        add_subdirectory(test)
endif()
