### Git submodules

### vcpkg

# spdlog
find_package(spdlog CONFIG REQUIRED)
set_target_properties(spdlog::spdlog PROPERTIES IMPORTED_GLOBAL TRUE)
target_compile_definitions(spdlog::spdlog INTERFACE SPDLOG_WCHAR_FILENAMES=1 SPDLOG_NO_THREAD_ID=1 SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
                                                    _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

# GSL
find_path(GSL_INCLUDE_DIR gsl-lite.hpp)
if(GSL_INCLUDE_DIR STREQUAL "GSL_INCLUDE_DIR-NOTFOUND")
    message(FATAL_ERROR "Could not find Guidelines Support Library.")
endif()
include_directories(${GSL_INCLUDE_DIR})
