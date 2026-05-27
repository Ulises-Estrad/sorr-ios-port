cmake_minimum_required(VERSION 3.20)

project(sorr_portable C)

get_filename_component(SORR_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

option(PORTABLE_RUNTIME_DIAG "Enable portable runtime diagnostic hooks" ON)
option(PORTABLE_RUNTIME_PUMP_EVENTS_DEFAULT "Pump SDL events in the portable runtime" ON)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    "${SORR_ROOT}/cmake")

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)

find_path(SDL2_INCLUDE_DIR SDL.h PATH_SUFFIXES SDL2 REQUIRED)
find_library(SDL2_LIBRARY NAMES SDL2 REQUIRED)
find_path(SDL2_MIXER_INCLUDE_DIR SDL_mixer.h PATH_SUFFIXES SDL2 REQUIRED)
find_library(SDL2_MIXER_LIBRARY NAMES SDL2_mixer REQUIRED)

add_subdirectory("${SORR_ROOT}/3rdparty/tre" "${CMAKE_BINARY_DIR}/tre")

include("${SORR_ROOT}/cmake/common_defs.cmake")

set(BGDRTM_SOURCE_FILES
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
  ${SORR_ROOT}/modules/mod_sound/mod_sound.c
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
  ${SORR_ROOT}/modules/mod_map/file_png.c
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

add_library(bgdrtm STATIC ${BGDRTM_SOURCE_FILES})

target_compile_definitions(bgdrtm
  PUBLIC
    __BGDRTM__
    SDL_MAIN_HANDLED)

target_include_directories(bgdrtm PRIVATE
  ${SORR_ROOT}/core/bgdrtm/include
  ${SORR_ROOT}/core/include
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
  ${SORR_ROOT}/modules/libdraw
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
  ${SORR_ROOT}/modules/mod_chipmunk
  ${SORR_ROOT}/modules/mod_iap
  ${SORR_ROOT}/3rdparty/tre/lib
  ${CMAKE_CURRENT_LIST_DIR}
  ${SDL2_INCLUDE_DIR}
  ${SDL2_MIXER_INCLUDE_DIR}
  ${PNG_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIRS})

if(CMAKE_C_COMPILER_ID MATCHES "GNU")
  target_compile_options(bgdrtm PRIVATE
    -include "${SORR_ROOT}/core/include/xstrings.h"
    -Wno-error=implicit-function-declaration
    -Wno-error=incompatible-pointer-types
    -Wno-error=int-conversion)
endif()

if(PORTABLE_RUNTIME_DIAG)
  target_compile_definitions(bgdrtm PRIVATE PORTABLE_RUNTIME_DIAG)
endif()

if(PORTABLE_RUNTIME_PUMP_EVENTS_DEFAULT)
  target_compile_definitions(bgdrtm PRIVATE PORTABLE_RUNTIME_PUMP_EVENTS_DEFAULT)
endif()

target_link_libraries(bgdrtm
  PRIVATE
    ${SDL2_LIBRARY}
    ${SDL2_MIXER_LIBRARY}
    PNG::PNG
    ZLIB::ZLIB
    libtre)

set(BGDI_SOURCE_FILES
  ${SORR_ROOT}/core/bgdi/src/main.c)

add_executable(${PROJECT_NAME} ${BGDI_SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  OUTPUT_NAME bgdi)

target_compile_definitions(${PROJECT_NAME}
  PUBLIC
    __BGDI__
    SDL_MAIN_HANDLED)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${SORR_ROOT}/core/bgdi/include
  ${SORR_ROOT}/core/include
  ${SORR_ROOT}/core/bgdrtm/include
  ${CMAKE_CURRENT_LIST_DIR})

if(PORTABLE_RUNTIME_DIAG)
  target_compile_definitions(${PROJECT_NAME} PRIVATE PORTABLE_RUNTIME_DIAG)
endif()

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    bgdrtm
    ${SDL2_LIBRARY}
    ${SDL2_MIXER_LIBRARY}
    PNG::PNG
    ZLIB::ZLIB
    libtre
    m)

if(MINGW)
  target_link_options(${PROJECT_NAME} PRIVATE -Wl,--enable-auto-import)
endif()

include("${SORR_ROOT}/cmake/compiler_flags.cmake")
