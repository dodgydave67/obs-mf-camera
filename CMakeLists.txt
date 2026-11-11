cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(obs-studio VERSION ${OBS_VERSION_CANONICAL})

option(OBS_MF_FORK_BUILD "Build the isolated experimental OBS MF application" OFF)
if(OBS_MF_FORK_BUILD)
  if(NOT WIN32)
    message(FATAL_ERROR "The OBS MF application is currently Windows only.")
  endif()
  set(VIRTUALCAM_GUID "703018E1-1834-48E9-8896-5EC54AA23476" CACHE STRING "Fork DirectShow camera GUID" FORCE)
  set(ENABLE_MF_VIRTUALCAM ON CACHE BOOL "Build the Media Foundation camera" FORCE)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows/architecture.cmake")
  if(NOT OBS_PARENT_ARCHITECTURE STREQUAL CMAKE_VS_PLATFORM_NAME)
    return()
  endif()
endif()

include(compilerconfig)
include(defaults)
include(helpers)

option(ENABLE_FRONTEND "Enable building with UI (requires Qt)" ON)
option(ENABLE_SCRIPTING "Enable scripting support" ON)
option(ENABLE_HEVC "Enable HEVC encoders" ON)

add_subdirectory(libobs)
if(OS_WINDOWS)
  add_subdirectory(libobs-d3d11)
  add_subdirectory(libobs-winrt)
endif()
add_subdirectory(libobs-opengl)
if(OS_MACOS)
  add_subdirectory(libobs-metal)
endif()
add_subdirectory(plugins)

add_subdirectory(test/test-input)

add_subdirectory(frontend)

message_configuration()
