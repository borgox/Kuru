# The SDK Generator
add_library(KuruSDKGen SHARED
    src/DllMain.cpp
    src/ReflectionWalk.cpp
    src/Generator.cpp
)

target_link_libraries(KuruSDKGen PRIVATE KuruCommon KuruSDKBase version)
target_include_directories(KuruSDKGen PRIVATE ${CMAKE_SOURCE_DIR}/sdk-base)
target_compile_definitions(KuruSDKGen PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
