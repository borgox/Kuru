# The SDK Generator
add_library(KuruSDKGenRuntime SHARED
    src/DllMain.cpp
    src/ReflectionWalk.cpp
    src/Generator.cpp
)

target_link_libraries(KuruSDKGenRuntime PRIVATE KuruCommon KuruSDKBase version)
target_include_directories(KuruSDKGenRuntime PRIVATE ${CMAKE_SOURCE_DIR}/sdk-base)
target_compile_definitions(KuruSDKGenRuntime PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
