cmake_minimum_required(VERSION 3.28)
project(d2r_windows_openxr_loader LANGUAGES C CXX RC)
set(OPENXR_SDK "$ENV{OPENXR_SDK}" CACHE PATH "OpenXR SDK checkout")
if(NOT EXISTS "${OPENXR_SDK}/src/loader/CMakeLists.txt")
  message(FATAL_ERROR "Set OPENXR_SDK to the Khronos OpenXR-SDK checkout")
endif()
set(DYNAMIC_LOADER ON CACHE BOOL "Build Windows loader DLL" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Build SDK tests" FORCE)
set(BUILD_WITH_SYSTEM_JSONCPP OFF CACHE BOOL "Use vendored jsoncpp" FORCE)
add_subdirectory("${OPENXR_SDK}" sdk)
# Khronos' non-MSVC branch applies an ELF symbol map even to MinGW. PE exports
# already use XRAPI_DLL_EXPORT; remove the ELF flag and the MinGW lib prefix.
set_target_properties(openxr_loader PROPERTIES LINK_FLAGS "" PREFIX "")
# Ship a self-contained Windows DLL, with no MinGW runtime DLL dependencies.
target_link_options(openxr_loader PRIVATE -static)
