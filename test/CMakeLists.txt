add_executable(utf-8_test utf-8_test.cpp)
add_executable(utf-8_decoder_test utf-8_decoder_test.cpp)

target_link_libraries(utf-8_test PRIVATE utf-8)
target_link_libraries(utf-8_decoder_test PRIVATE utf-8)
