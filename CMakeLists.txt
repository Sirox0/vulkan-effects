cmake_minimum_required(VERSION 3.25)
project(vkeffects LANGUAGES C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

add_compile_definitions(VK_NO_PROTOTYPES)
add_compile_definitions(CGLM_FORCE_DEPTH_ZERO_TO_ONE)

if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_DEPRECATE)
endif()

set(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/config.c
    ${CMAKE_CURRENT_LIST_DIR}/src/main.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mathext.c
    ${CMAKE_CURRENT_LIST_DIR}/src/vk.c
    ${CMAKE_CURRENT_LIST_DIR}/src/pipeline.c
    ${CMAKE_CURRENT_LIST_DIR}/src/temp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/vkFunctions.c
    ${CMAKE_CURRENT_LIST_DIR}/src/util.c
    ${CMAKE_CURRENT_LIST_DIR}/src/vkModel.c
    ${CMAKE_CURRENT_LIST_DIR}/src/modelViewScene.c
    ${CMAKE_CURRENT_LIST_DIR}/src/stb_image.c
)

set(SHADERS
    ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/composition.frag
    ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/fullscreen.vert
    ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/fullscreenNoUV.vert
    ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/gaussianBlur.frag
    ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/model.frag
    ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/model.vert
    ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/shadowmap.vert
    ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/skybox.frag
    ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/skybox.vert
    ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/ssao.frag
    ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/postprocess/uber.frag
)

find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)

set(BUILD_SHARED_LIBS OFF)
set(BUILD_EXAMPLES OFF)
set(BUILD_DOCS OFF)
set(BUILD_TESTING OFF)
add_subdirectory(external/iniparser)

set(BUILD_SHARED_LIBS OFF)
set(ASSIMP_DOUBLE_PRECISION OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_INSTALL_PDB OFF)
set(USE_STATIC_CRT ON)
set(ASSIMP_BUILD_ASSIMP_VIEW OFF)
add_subdirectory(external/assimp)

set(BUILD_SHARED_LIBS OFF)
set(KTX_FEATURE_TESTS OFF)
set(KTX_FEATURE_VK_UPLOAD OFF)
set(KTX_FEATURE_GL_UPLOAD OFF)
set(KTX_FEATURE_TOOLS OFF)
add_subdirectory(external/ktx)

foreach(SHADER IN LISTS SHADERS)
    add_custom_command(OUTPUT ${SHADER}.spv COMMAND glslang -V --lto ${SHADER} -o ${SHADER}.spv DEPENDS ${SHADER} COMMENT "compiling ${SHADER}")
    list(APPEND SPV_SHADERS ${SHADER}.spv)
endForeach()

file(GLOB_RECURSE ASSETS_LIST ${CMAKE_CURRENT_LIST_DIR}/assets/**)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/assets COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets DEPENDS ${ASSETS_LIST} COMMENT "copying ${CMAKE_CURRENT_LIST_DIR}/assets")
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/config.ini COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/config.ini ${CMAKE_CURRENT_BINARY_DIR}/config.ini DEPENDS ${CMAKE_CURRENT_LIST_DIR}/config.ini COMMENT "copying ${CMAKE_CURRENT_LIST_DIR}/config.ini")

add_custom_target(shaders DEPENDS ${SPV_SHADERS})
add_custom_target(assetscopy DEPENDS shaders ${CMAKE_CURRENT_BINARY_DIR}/assets)
add_custom_target(configcopy DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/config.ini)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})
add_dependencies(${CMAKE_PROJECT_NAME} shaders assetscopy configcopy)
target_link_libraries(${CMAKE_PROJECT_NAME} SDL3::SDL3-static Vulkan::Headers iniparser-static assimp::assimp ktx)
if(UNIX)
    target_link_libraries(${CMAKE_PROJECT_NAME} m)
endif()
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include ${CMAKE_CURRENT_LIST_DIR}/external/cglm/include ${CMAKE_CURRENT_LIST_DIR}/external/stb)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE CMAKE_EXPORT_COMPILE_COMMANDS=ON)