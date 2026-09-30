add_library(KuruCommon STATIC
    src/Globals.cpp
    src/Memory.cpp
    src/Pattern.cpp
    src/Unreal.cpp
)

target_include_directories(KuruCommon
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(KuruCommon PUBLIC cxx_std_20)

target_compile_definitions(KuruCommon PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)

target_link_libraries(KuruCommon PUBLIC psapi)
