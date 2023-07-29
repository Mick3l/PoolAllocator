add_executable(allocator_main main.cpp)

target_include_directories(allocator_main PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(allocator_main PRIVATE allocator)
