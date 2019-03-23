# Git submodules
# vcpkg
find_package(spdlog CONFIG REQUIRED)
set_target_properties(spdlog::spdlog PROPERTIES IMPORTED_GLOBAL TRUE)
# if(MSVC)
target_compile_definitions(spdlog::spdlog INTERFACE SPDLOG_WCHAR_FILENAMES=1 SPDLOG_NO_THREAD_ID=1 SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
                                                    _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
