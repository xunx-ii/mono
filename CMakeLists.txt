cmake_minimum_required (VERSION 3.8)

set(CMAKE_TOOLCHAIN_FILE "/root/vcpkg/scripts/buildsystems/vcpkg.cmake")

project ("mono")

add_definitions(-D_WEBSOCKETPP_CPP11_RANDOM_DEVICE_)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(CURL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(websocketpp CONFIG REQUIRED)

add_executable (mono 
    "src/mono.cpp" 
    "src/application.cpp" 
    "src/processes.cpp" 
    "src/taskermgr.cpp"
)

set_property(TARGET mono PROPERTY CXX_STANDARD 17)

target_link_libraries(mono PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(mono PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(mono PRIVATE websocketpp::websocketpp)
target_link_libraries(mono PRIVATE CURL::libcurl)