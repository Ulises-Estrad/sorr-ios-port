cmake_minimum_required(VERSION 3.22)

project(sorr_ios_shell C)

get_filename_component(SORR_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

set(SORR_IOS_BUNDLE_IDENTIFIER "dev.local.sorr.iosshell" CACHE STRING "Bundle identifier for the iOS shell app")
set(SORR_IOS_SDL2_ROOT "" CACHE PATH "Path to an SDL2 iOS install prefix containing include/SDL2 and lib")
set(SORR_IOS_SDL2_FRAMEWORK "" CACHE PATH "Path to SDL2.framework for iOS")
option(SORR_IOS_SHELL_USE_SDL_MAIN_HANDLED "Define SDL_MAIN_HANDLED for the iOS shell app" OFF)
option(SORR_IOS_D3_FIRST_RENDER "Build the D3 runtime-linked first-render probe" OFF)

find_package(ZLIB REQUIRED)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  message(WARNING "This build area is intended for an iOS toolchain. Configure with an iOS CMake toolchain or Xcode generator on macOS.")
endif()

if(SORR_IOS_D3_FIRST_RENDER)
  add_subdirectory("${SORR_ROOT}/3rdparty/tre" "${CMAKE_BINARY_DIR}/tre-ios")

  set(SORR_IOS_RUNTIME_SOURCE_FILES
    ${SORR_ROOT}/core/bgdrtm/src/copy.c
    ${SORR_ROOT}/core/bgdrtm/src/dcbr.c
    ${SORR_ROOT}/core/bgdrtm/src/dirs.c
    ${SORR_ROOT}/core/bgdrtm/src/instance.c
    ${SORR_ROOT}/core/bgdrtm/src/interpreter.c
    ${SORR_ROOT}/core/bgdrtm/src/misc.c
    ${SORR_ROOT}/core/bgdrtm/src/strings.c
    ${SORR_ROOT}/core/bgdrtm/src/sysprocs.c
    ${SORR_ROOT}/core/bgdrtm/src/varspace_file.c
    ${SORR_ROOT}/core/bgdrtm/src/fmath.c
    ${SORR_ROOT}/core/common/debug.c
    ${SORR_ROOT}/core/common/files.c
    ${SORR_ROOT}/core/common/xctype.c
    ${SORR_ROOT}/modules/libbgload/bgload.c
    ${SORR_ROOT}/modules/mod_say/mod_say.c
    ${SORR_ROOT}/modules/mod_string/mod_string.c
    ${SORR_ROOT}/modules/mod_math/mod_math.c
    ${SORR_ROOT}/modules/mod_mem/mod_mem.c
    ${SORR_ROOT}/modules/mod_time/mod_time.c
    ${SORR_ROOT}/modules/mod_file/mod_file.c
    ${CMAKE_CURRENT_LIST_DIR}/src/sorr_ios_mod_sound_stub.c
    ${SORR_ROOT}/modules/libsdlhandler/libsdlhandler.c
    ${SORR_ROOT}/modules/libjoy/libjoy.c
    ${SORR_ROOT}/modules/mod_joy/mod_joy.c
    ${SORR_ROOT}/modules/libkey/libkey.c
    ${SORR_ROOT}/modules/mod_key/mod_key.c
    ${SORR_ROOT}/modules/mod_proc/mod_proc.c
    ${SORR_ROOT}/modules/mod_sort/mod_sort.c
    ${SORR_ROOT}/modules/mod_timers/mod_timers.c
    ${SORR_ROOT}/modules/libgrbase/g_bitmap.c
    ${SORR_ROOT}/modules/libgrbase/g_clear.c
    ${SORR_ROOT}/modules/libgrbase/g_grlib.c
    ${SORR_ROOT}/modules/libgrbase/g_pal.c
    ${SORR_ROOT}/modules/libgrbase/g_blendop.c
    ${SORR_ROOT}/modules/libgrbase/g_conversion.c
    ${SORR_ROOT}/modules/libgrbase/libgrbase.c
    ${SORR_ROOT}/modules/libblit/g_blit.c
    ${SORR_ROOT}/modules/libblit/g_pixel.c
    ${SORR_ROOT}/modules/libblit/libblit.c
    ${SORR_ROOT}/modules/libvideo/g_regions.c
    ${SORR_ROOT}/modules/libvideo/g_video.c
    ${SORR_ROOT}/modules/librender/g_fade.c
    ${SORR_ROOT}/modules/librender/g_frame.c
    ${SORR_ROOT}/modules/librender/g_instance.c
    ${SORR_ROOT}/modules/librender/g_object.c
    ${SORR_ROOT}/modules/librender/g_rects.c
    ${SORR_ROOT}/modules/librender/g_screen.c
    ${SORR_ROOT}/modules/librender/librender.c
    ${SORR_ROOT}/modules/mod_video/mod_video.c
    ${SORR_ROOT}/modules/libmouse/libmouse.c
    ${SORR_ROOT}/modules/mod_multi/mod_multi.c
    ${SORR_ROOT}/modules/mod_map/file_fnt.c
    ${SORR_ROOT}/modules/mod_map/file_fpg.c
    ${SORR_ROOT}/modules/mod_map/file_map.c
    ${SORR_ROOT}/modules/mod_map/file_pal.c
    ${SORR_ROOT}/modules/mod_map/file_pcx.c
    ${CMAKE_CURRENT_LIST_DIR}/src/sorr_ios_file_png_stub.c
    ${SORR_ROOT}/modules/mod_map/mod_map.c
    ${SORR_ROOT}/modules/libfont/libfont.c
    ${SORR_ROOT}/modules/mod_dir/mod_dir.c
    ${SORR_ROOT}/modules/libtext/libtext.c
    ${SORR_ROOT}/modules/mod_text/mod_text.c
    ${SORR_ROOT}/modules/mod_rand/mod_rand.c
    ${SORR_ROOT}/modules/mod_grproc/mod_grproc.c
    ${SORR_ROOT}/modules/libscroll/libscroll.c
    ${SORR_ROOT}/modules/mod_scroll/mod_scroll.c
    ${SORR_ROOT}/modules/libdraw/libdraw.c
    ${SORR_ROOT}/modules/mod_draw/mod_draw.c
    ${SORR_ROOT}/modules/mod_regex/mod_regex.c
    ${SORR_ROOT}/modules/mod_screen/mod_screen.c
    ${SORR_ROOT}/modules/mod_path/mod_path.c
    ${SORR_ROOT}/modules/mod_effects/mod_effects.c
    ${SORR_ROOT}/modules/mod_blendop/mod_blendop.c
    ${SORR_ROOT}/modules/mod_m7/mod_m7.c
    ${SORR_ROOT}/modules/libwm/libwm.c
    ${SORR_ROOT}/modules/mod_wm/mod_wm.c
    ${SORR_ROOT}/modules/mod_sys/mod_sys.c
    ${SORR_ROOT}/modules/mod_flic/mod_flic.c)
else()
  set(SORR_IOS_RUNTIME_SOURCE_FILES
    ${SORR_ROOT}/core/bgdrtm/src/fmath.c
    ${SORR_ROOT}/core/bgdrtm/src/misc.c
    ${SORR_ROOT}/core/bgdrtm/src/strings.c)
endif()

add_executable(SorrIOSShell MACOSX_BUNDLE
  src/sorr_ios_shell_main.c
  ${SORR_IOS_RUNTIME_SOURCE_FILES})

set_target_properties(SorrIOSShell PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/Info.plist.in"
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${SORR_IOS_BUNDLE_IDENTIFIER}"
  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1"
  XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS "iphoneos iphonesimulator"
  XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME ""
  XCODE_ATTRIBUTE_INFOPLIST_KEY_UIRequiresFullScreen "YES"
  XCODE_ATTRIBUTE_INFOPLIST_KEY_UISupportedInterfaceOrientations "UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight")

target_compile_definitions(SorrIOSShell PRIVATE
  __BGDRTM__
  TARGET_IOS
  SORR_IOS_APP_SHELL)

if(SORR_IOS_D3_FIRST_RENDER)
  target_compile_definitions(SorrIOSShell PRIVATE
    __MONOLITHIC__
    NO_MODCHIPMUNK
    NO_MODICONV
    NO_MODFMODEX
    NO_MODCURL
    NO_MODMATHI
    NO_MODSENSOR
    NO_FSOCK
    NO_MODIAP
    NO_MODIMAGE
    PORTABLE_RUNTIME_DIAG
    PORTABLE_RUNTIME_PUMP_EVENTS_DEFAULT
    SORR_HOST_POINTER_TABLES
    SORR_IOS_D3_FIRST_RENDER
    VERSION="1.0.0")
endif()

target_include_directories(SorrIOSShell PRIVATE
  ${SORR_ROOT}/core/bgdrtm/include
  ${SORR_ROOT}/core/include
  ${ZLIB_INCLUDE_DIRS})

if(SORR_IOS_D3_FIRST_RENDER)
  target_include_directories(SorrIOSShell PRIVATE
    ${SORR_ROOT}/modules/libbgload
    ${SORR_ROOT}/modules/mod_say
    ${SORR_ROOT}/modules/mod_string
    ${SORR_ROOT}/modules/mod_math
    ${SORR_ROOT}/modules/mod_mem
    ${SORR_ROOT}/modules/mod_time
    ${SORR_ROOT}/modules/mod_file
    ${SORR_ROOT}/modules/mod_sound
    ${SORR_ROOT}/modules/libsdlhandler
    ${SORR_ROOT}/modules/mod_joy
    ${SORR_ROOT}/modules/libjoy
    ${SORR_ROOT}/modules/libkey
    ${SORR_ROOT}/modules/mod_key
    ${SORR_ROOT}/modules/mod_proc
    ${SORR_ROOT}/modules/mod_sort
    ${SORR_ROOT}/modules/mod_timers
    ${SORR_ROOT}/modules/libgrbase
    ${SORR_ROOT}/modules/libblit
    ${SORR_ROOT}/modules/libvideo
    ${SORR_ROOT}/modules/librender
    ${SORR_ROOT}/modules/mod_video
    ${SORR_ROOT}/modules/libmouse
    ${SORR_ROOT}/modules/mod_mouse
    ${SORR_ROOT}/modules/mod_map
    ${SORR_ROOT}/modules/libfont
    ${SORR_ROOT}/modules/mod_dir
    ${SORR_ROOT}/modules/libtext
    ${SORR_ROOT}/modules/mod_text
    ${SORR_ROOT}/modules/mod_rand
    ${SORR_ROOT}/modules/mod_grproc
    ${SORR_ROOT}/modules/libscroll
    ${SORR_ROOT}/modules/mod_scroll
    ${SORR_ROOT}/modules/mod_draw
    ${SORR_ROOT}/modules/mod_screen
    ${SORR_ROOT}/modules/mod_path
    ${SORR_ROOT}/modules/mod_effects
    ${SORR_ROOT}/modules/mod_blendop
    ${SORR_ROOT}/modules/mod_m7
    ${SORR_ROOT}/modules/libwm
    ${SORR_ROOT}/modules/mod_wm
    ${SORR_ROOT}/modules/mod_sys
    ${SORR_ROOT}/modules/mod_flic
    ${SORR_ROOT}/modules/mod_regex
    ${SORR_ROOT}/modules/mod_multi
    ${SORR_ROOT}/3rdparty/tre/lib
    ${CMAKE_CURRENT_LIST_DIR}/../portable
    ${CMAKE_CURRENT_LIST_DIR}/src)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(SorrIOSShell PRIVATE -ffunction-sections -fdata-sections)
  if(SORR_IOS_D3_FIRST_RENDER)
    target_compile_options(SorrIOSShell PRIVATE -include "${SORR_ROOT}/core/include/xstrings.h")
  endif()
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

if(SORR_IOS_D3_FIRST_RENDER)
  target_link_libraries(SorrIOSShell PRIVATE libtre)
endif()

# The default target remains the shell/data-layout proof. D3 explicitly opts
# into the portable runtime source list and uses local iOS stubs for audio and
# PNG so first render can be attempted without bundling codecs or game assets.
