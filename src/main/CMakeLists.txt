add_library(main INTERFACE)

target_link_libraries(main INTERFACE emulator spdlog::spdlog util)
