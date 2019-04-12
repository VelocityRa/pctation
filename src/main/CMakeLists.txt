add_library(main INTERFACE)

target_link_libraries(main INTERFACE imgui::imgui SDL2::SDL2 SDL2::SDL2main emulator gui util)
