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
add_subdirectory(libs/KuruSDKGen)
add_subdirectory(libs/KuruLoader)
add_subdirectory(injector)

# Release archives contain the executable and its license notices.
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "Kuru")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_FILE_NAME "Kuru-${CPACK_PACKAGE_VERSION}-windows-x64")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${PROJECT_NAME};KuruRelease;/")
include(CPack)
