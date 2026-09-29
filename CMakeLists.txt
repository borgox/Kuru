cmake_minimum_required(VERSION 3.24)

project(Kuru LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /permissive- /EHsc)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(third_party)

add_subdirectory(sdk-base)

add_subdirectory(libs/KuruCommon)
add_subdirectory(libs/KuruDump)
add_subdirectory(libs/KuruSDKGen) #TODO: remove KuruDump and KuruSDKGen and keep only runtime.
add_subdirectory(libs/KuruSDKGenRuntime)
add_subdirectory(libs/KuruLoader)
