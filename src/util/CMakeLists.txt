add_library(util STATIC util.cpp
                        fs.hpp
                        load_file.hpp
                        types.hpp
                        log.hpp
                        log.cpp)

target_link_libraries(util PUBLIC spdlog::spdlog)
