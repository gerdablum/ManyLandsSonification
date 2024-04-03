cmake_minimum_required (VERSION 2.6)
project (ManyLands)

set(CMAKE_CXX_STANDARD 17)
if(WIN32)
    set(GUI_TYPE WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set(STK_DIR C:/Users/Alina/Master-Projects/4D-sonification/sonification-libraries/stk-5.0.1)
set(STK_BINARY_DIR C:/Users/Alina/Master-Projects/4D-sonification/sonification-libraries/stk-build)

if (MSVC)
    add_definitions("/MP")
endif()

find_package(Boost REQUIRED)
if(NOT EMSCRIPTEN)
    find_package(OpenGL REQUIRED)
    find_package(SDL2 REQUIRED)
endif()

if(NOT WIN32 OR EMSCRIPTEN)
    add_compile_definitions(USE_GL_ES3)
endif()

if(WIN32)
    add_compile_definitions(NOMINMAX)
    # We add the definition below to suppress warning from boost library
    add_compile_definitions(_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
endif()


include_directories(${Boost_INCLUDE_DIRS})
include_directories("include/glm" "include/imgui" "include/CDT" "include/stk")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
if(WIN32)
    include_directories(${OPENGL_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR})
    include_directories("include/GL3W")
endif()

file(GLOB H_FILES "src/*.h")
file(GLOB CPP_FILES "src/*.cpp")
file(GLOB IMGUI_FILES "include/imgui/*.h" "include/imgui/*.cpp")
file(GLOB SHADERS_FILES "shaders/*.frag" "shaders/*.vert")
if(WIN32)
    file(GLOB GL3W_FILES "include/GL3W/GL/*.h" "include/GL3W/GL/*.c")
endif()

set(destinationName "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${destinationName})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/SDL2.dll DESTINATION ${destinationName})
file(COPY ${STK_DIR}/rawwaves DESTINATION ${destinationName})

include(CMakePrintHelpers)
set(destinationName "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
message("Copy files to where .exe is located at \"${destinationName}\"")

if(WIN32)
    add_executable(ManyLands ${GUI_TYPE} ${CPP_FILES} ${GL3W_FILES} ${IMGUI_FILES} ${H_FILES} ${SHADERS_FILES}
            src/Audio.cpp)
else()
    add_executable(ManyLands ${GUI_TYPE} ${CPP_FILES} ${IMGUI_FILES} ${H_FILES} ${SHADERS_FILES}
            src/Audio.cpp)
endif()

source_group("imgui" FILES ${IMGUI_FILES})
source_group("shaders" FILES ${SHADERS_FILES})

if(WIN32)
    source_group("gl3w" FILES ${GL3W_FILES})
endif()

# stk library
add_subdirectory(${STK_DIR} ${STK_BINARY_DIR})
target_link_libraries(ManyLands stk)

if(EMSCRIPTEN)
    # Emscripten
    target_link_libraries(ManyLands "-s USE_SDL=2 -s FULL_ES3=1 -s USE_WEBGL2=1 -o ManyLands.html --shell-file assets/shell_minimal.html -s \"EXPORTED_FUNCTIONS=['_main', '_js_load_ode']\" -s \"EXTRA_EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap']\"")
    set_target_properties(ManyLands PROPERTIES COMPILE_FLAGS "-s USE_SDL=2 -s FULL_ES3=1 -s USE_WEBGL2=1")
    set_target_properties(ManyLands PROPERTIES LINK_FLAGS "-s ALLOW_MEMORY_GROWTH=1 -s BINARYEN_TRAP_MODE='clamp' --preload-file assets")
else()
    target_link_libraries(ManyLands ${OPENGL_LIBRARIES} SDL2::SDL2 SDL2::SDL2main)
    #${SDL2_LIBRARY}
endif()
