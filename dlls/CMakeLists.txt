cmake_minimum_required(VERSION 3.5)

# Collect sources
file(GLOB DLL_SOURCES
    *.cpp
    )
file(GLOB PM_SHARED_SOURCES
    ../pm_shared/*.c
    ../pm_shared/*.cpp
    )
file(GLOB GAME_SHARED_SOURCES
    ../game_shared/*.cpp
    )
file(GLOB COMMON_SOURCES
    ../common/*.cpp
    )

add_library(hlr SHARED
    ${DLL_SOURCES}
    ${PM_SHARED_SOURCES}
    ${GAME_SHARED_SOURCES}
    ${COMMON_SOURCES}
)

# Include directories
target_include_directories(hlr PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../engine
    ../common
    ../pm_shared
    ../game_shared
)

# Definitions similar to Makefile
target_compile_definitions(hlr PRIVATE
    stricmp=strcasecmp
    _strnicmp=strncasecmp
    strnicmp=strncasecmp
    CLIENT_WEAPONS
)

# Basic compile options
if(NOT EMSCRIPTEN)
    target_compile_options(hlr PRIVATE -w -g -pthread -fPIC)
    target_link_libraries(hlr PRIVATE pthread m z)
else()
    # When building with Emscripten, link with its pthreads and math libs
    set_target_properties(hlr PROPERTIES SUFFIX ".wasm")
endif()

set_target_properties(hlr PROPERTIES PREFIX "" OUTPUT_NAME "hlr")
