add_subdirectory(main)
add_subdirectory(emulator)
add_subdirectory(cpu)
add_subdirectory(memory)
add_subdirectory(bus)
add_subdirectory(bios)
add_subdirectory(util)

# Main executable
add_executable(pcstation main/main.cpp)
target_link_libraries(pcstation PRIVATE main)
