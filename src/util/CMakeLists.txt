add_library(util STATIC util.cpp fs.hpp load_file.hpp types.hpp log.hpp)

target_include_directories(util PUBLIC .)

target_link_libraries(util INTERFACE spdlog::spdlog)
