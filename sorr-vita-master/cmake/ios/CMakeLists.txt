cmake_minimum_required(VERSION 3.22)

project(sorr_ios_shell C)

get_filename_component(SORR_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(SORR_IOS_LAUNCH_STORYBOARD "${CMAKE_CURRENT_LIST_DIR}/LaunchScreen.storyboard")

set(SORR_IOS_BUNDLE_IDENTIFIER "dev.local.sorr.iosshell" CACHE STRING "Bundle identifier for the iOS shell app")
set(SORR_IOS_SDL2_ROOT "" CACHE PATH "Path to an SDL2 iOS install prefix containing include/SDL2 and lib")
set(SORR_IOS_SDL2_FRAMEWORK "" CACHE PATH "Path to SDL2.framework for iOS")
option(SORR_IOS_SHELL_USE_SDL_MAIN_HANDLED "Define SDL_MAIN_HANDLED for the iOS shell app" OFF)

find_package(ZLIB REQUIRED)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  message(WARNING "This build area is intended for an iOS toolchain. Configure with an iOS CMake toolchain or Xcode generator on macOS.")
endif()

add_executable(SorrIOSShell MACOSX_BUNDLE
  src/sorr_ios_shell_main.c
  ${SORR_IOS_LAUNCH_STORYBOARD}
  ${SORR_ROOT}/core/bgdrtm/src/fmath.c
  ${SORR_ROOT}/core/bgdrtm/src/misc.c
  ${SORR_ROOT}/core/bgdrtm/src/strings.c)

set_source_files_properties("${SORR_IOS_LAUNCH_STORYBOARD}" PROPERTIES
  MACOSX_PACKAGE_LOCATION Resources)

set_target_properties(SorrIOSShell PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/Info.plist.in"
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${SORR_IOS_BUNDLE_IDENTIFIER}"
  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1"
  XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS "iphoneos iphonesimulator"
  XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME ""
  XCODE_ATTRIBUTE_INFOPLIST_KEY_UILaunchStoryboardName "LaunchScreen"
  XCODE_ATTRIBUTE_INFOPLIST_KEY_UIRequiresFullScreen "YES"
  XCODE_ATTRIBUTE_INFOPLIST_KEY_UISupportedInterfaceOrientations "UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight")

target_compile_definitions(SorrIOSShell PRIVATE
  __BGDRTM__
  TARGET_IOS
  SORR_IOS_APP_SHELL)

target_include_directories(SorrIOSShell PRIVATE
  ${SORR_ROOT}/core/bgdrtm/include
  ${SORR_ROOT}/core/include
  ${ZLIB_INCLUDE_DIRS})

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(SorrIOSShell PRIVATE -ffunction-sections -fdata-sections)
endif()

if(APPLE)
  target_link_options(SorrIOSShell PRIVATE -Wl,-dead_strip)
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU")
  target_link_options(SorrIOSShell PRIVATE -Wl,--gc-sections)
endif()

if(SORR_IOS_SHELL_USE_SDL_MAIN_HANDLED)
  target_compile_definitions(SorrIOSShell PRIVATE SDL_MAIN_HANDLED)
endif()

if(SORR_IOS_SDL2_FRAMEWORK)
  get_filename_component(SDL2_FRAMEWORK_DIR "${SORR_IOS_SDL2_FRAMEWORK}" DIRECTORY)
  target_include_directories(SorrIOSShell PRIVATE "${SORR_IOS_SDL2_FRAMEWORK}/Headers")
  target_link_options(SorrIOSShell PRIVATE "-F${SDL2_FRAMEWORK_DIR}")
  target_link_libraries(SorrIOSShell PRIVATE "-framework SDL2")
  set_target_properties(SorrIOSShell PROPERTIES
    XCODE_EMBED_FRAMEWORKS "${SORR_IOS_SDL2_FRAMEWORK}"
    XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY YES)
elseif(SORR_IOS_SDL2_ROOT)
  find_path(SDL2_INCLUDE_DIR
    NAMES SDL.h
    PATHS
      "${SORR_IOS_SDL2_ROOT}/include/SDL2"
      "${SORR_IOS_SDL2_ROOT}/include"
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
    REQUIRED)
  find_library(SDL2_LIBRARY
    NAMES SDL2 SDL2-static
    PATHS
      "${SORR_IOS_SDL2_ROOT}/lib"
      "${SORR_IOS_SDL2_ROOT}"
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
    REQUIRED)
  find_library(SDL2MAIN_LIBRARY
    NAMES SDL2main
    PATHS
      "${SORR_IOS_SDL2_ROOT}/lib"
      "${SORR_IOS_SDL2_ROOT}"
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH)

  message(STATUS "Using SDL2 include directory: ${SDL2_INCLUDE_DIR}")
  message(STATUS "Using SDL2 library: ${SDL2_LIBRARY}")
  if(SDL2MAIN_LIBRARY)
    message(STATUS "Using SDL2main library: ${SDL2MAIN_LIBRARY}")
  endif()

  target_include_directories(SorrIOSShell PRIVATE "${SDL2_INCLUDE_DIR}")
  if(NOT SORR_IOS_SHELL_USE_SDL_MAIN_HANDLED AND SDL2MAIN_LIBRARY)
    target_link_libraries(SorrIOSShell PRIVATE "${SDL2MAIN_LIBRARY}")
  endif()
  target_link_libraries(SorrIOSShell PRIVATE "${SDL2_LIBRARY}")
else()
  find_package(SDL2 CONFIG QUIET)
  if(NOT SORR_IOS_SHELL_USE_SDL_MAIN_HANDLED AND TARGET SDL2::SDL2main)
    target_link_libraries(SorrIOSShell PRIVATE SDL2::SDL2main)
  endif()

  if(TARGET SDL2::SDL2)
    target_link_libraries(SorrIOSShell PRIVATE SDL2::SDL2)
  elseif(TARGET SDL2::SDL2-static)
    target_link_libraries(SorrIOSShell PRIVATE SDL2::SDL2-static)
  else()
    message(FATAL_ERROR "SDL2 for iOS was not found. Set SORR_IOS_SDL2_FRAMEWORK to SDL2.framework or SORR_IOS_SDL2_ROOT to an SDL2 iOS install prefix.")
  endif()
endif()

if(APPLE)
  target_link_libraries(SorrIOSShell PRIVATE
    "-framework AudioToolbox"
    "-framework AVFoundation"
    "-framework CoreBluetooth"
    "-framework CoreGraphics"
    "-framework CoreHaptics"
    "-framework CoreMotion"
    "-framework Foundation"
    "-framework GameController"
    "-framework Metal"
    "-framework OpenGLES"
    "-framework QuartzCore"
    "-framework UIKit")
endif()

target_link_libraries(SorrIOSShell PRIVATE ZLIB::ZLIB)

# Milestone 1 intentionally links only the tiny bgdrtm entry subset and does
# not load SorR.dat.
# Use cmake/portable/CMakeLists.txt as the source-list reference for the later
# runtime target once the pointer-through-int ABI plan is implemented.
