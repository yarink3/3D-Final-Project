get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 14)

######################################################################################################
if(TARGET igl_opengl)
    if(NOT TARGET stb_image)
        igl_download_stb()
        add_subdirectory(${LIBIGL_EXTERNAL}/stb stb_image)
    endif()
    compile_igl_module("png" "")
    target_link_libraries(igl_png ${IGL_SCOPE} igl_stb_image igl_opengl)
endif()

#
#file(GLOB_RECURSE Foo_SOURCES "igl/glfw/imgui/*.cpp")
#file(GLOB_RECURSE Foo_HEADERS "igl/glfw/imgui/*.h")
#
#set (Foo_INCLUDE_DIRS "")
#foreach (_headerFile ${Foo_HEADERS})
#    get_filename_component(_dir ${_headerFile} PATH)
#    list (APPEND Foo_INCLUDE_DIRS ${_dir})
#endforeach()
#list(REMOVE_DUPLICATES Foo_INCLUDE_DIRS)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
#
#find_package(IMGUI REQUIRED)
#include_directories($IMGUI_INCLUDE_DIRS})
######################################################################################################
find_package( OpenCV REQUIRED )

include_directories(${PROJECT_SOURCE_DIR})


add_executable(${PROJECT_NAME}_bin main.cpp inputManager.h OurMesh.h OurMesh.cpp  AudioPlayer.h  )
target_link_libraries(${PROJECT_NAME}_bin  igl::core igl::opengl igl::opengl_glfw igl::opengl igl::png ${OpenCV_LIBS} ${SDL2_LIBRARIES} )

