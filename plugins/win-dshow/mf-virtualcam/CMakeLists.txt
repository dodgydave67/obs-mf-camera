cmake_minimum_required(VERSION 3.28)

# Can also build on its own without Qt, FFmpeg, or OBS dependencies.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(obs-mf-camera LANGUAGES C CXX)
  include(CTest)
endif()

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
  message(FATAL_ERROR "The experimental MF camera currently requires Windows x64 and MSVC.")
endif()
if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION VERSION_LESS "10.0.22000.0")
  message(FATAL_ERROR "The MF camera requires Windows SDK 10.0.22000.0 or newer.")
endif()

set(OBS_MF_NUGET_ROOT "" CACHE PATH "Directory holding restored Microsoft.Windows.* NuGet packages")
find_path(OBS_MF_WIL_INCLUDE wil/cppwinrt.h
  HINTS "${OBS_MF_NUGET_ROOT}/Microsoft.Windows.ImplementationLibrary.1.0.260126.7/include" REQUIRED)
find_path(OBS_MF_WINRT_INCLUDE winrt/base.h
  HINTS "${OBS_MF_NUGET_ROOT}/Microsoft.Windows.CppWinRT.3.0.260520.1/include" REQUIRED)

add_library(obs-mf-camera-transport STATIC transport.cpp)
target_compile_features(obs-mf-camera-transport PUBLIC cxx_std_17)
target_compile_definitions(obs-mf-camera-transport PUBLIC NOMINMAX UNICODE _UNICODE)
target_link_libraries(obs-mf-camera-transport PUBLIC advapi32 ole32)

add_library(obs-mf-camera-bridge STATIC bridge.cpp)
target_compile_features(obs-mf-camera-bridge PUBLIC cxx_std_17)
target_link_libraries(obs-mf-camera-bridge PRIVATE obs-mf-camera-transport mfplat mfuuid)

add_library(obs-mf-camera-source MODULE
  source/SimpleMediaSource.cpp source/SimpleMediaStream.cpp source/SimpleFrameGenerator.cpp
  source/VirtualCameraMediaSourceActivate.cpp source/winrtCommon.cpp source/dllmain.cpp
  source/VirtualCameraMediaSource.def)
target_compile_features(obs-mf-camera-source PRIVATE cxx_std_17)
target_compile_definitions(obs-mf-camera-source PRIVATE NOMINMAX UNICODE _UNICODE)
target_compile_options(obs-mf-camera-source PRIVATE /EHsc /bigobj /Zc:__cplusplus)
target_include_directories(obs-mf-camera-source PRIVATE "${OBS_MF_WIL_INCLUDE}" "${OBS_MF_WINRT_INCLUDE}")
target_link_libraries(obs-mf-camera-source PRIVATE mfplat mfuuid mf mfsensorgroup windowsapp ole32 propsys)
set_target_properties(obs-mf-camera-source PROPERTIES PREFIX "")
install(TARGETS obs-mf-camera-source RUNTIME DESTINATION mf-camera LIBRARY DESTINATION mf-camera)
install(FILES Install-Source.ps1 Uninstall-Source.ps1 README.md source/LICENSE.Microsoft DESTINATION mf-camera)

if(BUILD_TESTING)
  add_executable(obs-mf-transport-tests tests/transport-tests.cpp)
  target_link_libraries(obs-mf-transport-tests PRIVATE obs-mf-camera-transport)
  add_test(NAME obs-mf-transport COMMAND obs-mf-transport-tests)
  set_tests_properties(obs-mf-transport PROPERTIES TIMEOUT 30)
  add_executable(obs-mf-scale-tests tests/scale-tests.cpp)
  target_compile_features(obs-mf-scale-tests PRIVATE cxx_std_17)
  add_test(NAME obs-mf-scale COMMAND obs-mf-scale-tests)
endif()
