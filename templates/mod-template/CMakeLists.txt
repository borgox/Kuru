cmake_minimum_required(VERSION 3.24)
project(MyMod LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)

foreach(_required sdk-base/CMakeLists.txt generated-sdk/KuruSDK.hpp imgui/imgui.h)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_required}")
        message(FATAL_ERROR "Use the mod-template folder produced by Kuru's SDK generator. Missing: ${_required}")
    endif()
endforeach()

add_subdirectory(sdk-base)
add_library(imgui STATIC
    imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp)
target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui")

add_library(MyMod SHARED src/MyMod.cpp)
target_include_directories(MyMod PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/generated-sdk")
target_link_libraries(MyMod PRIVATE KuruSDKBase imgui)
