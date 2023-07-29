include(FetchContent)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(PerformanceTest PerformanceTest.cpp)

target_link_libraries(PerformanceTest PRIVATE GTest::gtest_main allocator)

target_include_directories(PerformanceTest PUBLIC ${PROJECT_SOURCE_DIR})

include(GoogleTest)

gtest_discover_tests(PerformanceTest)
