add_library(allocator allocator.h)

set_target_properties(allocator PROPERTIES LINKER_LANGUAGE CXX)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

target_link_libraries(allocator PUBLIC nlohmann_json::nlohmann_json)
