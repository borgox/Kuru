cmake_minimum_required(VERSION 3.24)
project(MyMod LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to a Kuru checkout that has been built at least once. Override on the command line:
#   cmake -B build -DKURU_DIR=C:/path/to/Kuru
set(KURU_DIR "" CACHE PATH "Path to the Kuru source tree")
if(NOT KURU_DIR)
    message(FATAL_ERROR "Set KURU_DIR to your Kuru checkout, e.g. -DKURU_DIR=C:/path/to/Kuru")
endif()

# The generated SDK (typed Rocket League classes). Produced when Kuru is built auto-detected under its build tree, or set explicitly with -DKURU_GENERATED_SDK=...
set(KURU_GENERATED_SDK "" CACHE PATH "Path to generated-sdk")
if(NOT KURU_GENERATED_SDK)
    file(GLOB _candidates "${KURU_DIR}/build/*/generated-sdk" "${KURU_DIR}/cmake-build-*/generated-sdk")
    if(_candidates)
        list(GET _candidates 0 KURU_GENERATED_SDK)
    endif()
endif()
if(NOT KURU_GENERATED_SDK OR NOT EXISTS "${KURU_GENERATED_SDK}")
    message(FATAL_ERROR "generated-sdk not found. Build Kuru once, or pass -DKURU_GENERATED_SDK=...")
endif()

add_subdirectory(${KURU_DIR}/sdk-base    ${CMAKE_BINARY_DIR}/kuru-sdk)
add_subdirectory(${KURU_DIR}/third_party ${CMAKE_BINARY_DIR}/kuru-deps)

add_library(MyMod SHARED src/MyMod.cpp)
target_include_directories(MyMod PRIVATE ${KURU_GENERATED_SDK})
target_link_libraries(MyMod PRIVATE KuruSDKBase imgui)
target_compile_definitions(MyMod PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
