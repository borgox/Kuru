add_library(KuruSDKGenRuntime SHARED
    src/DllMain.cpp
)

target_link_libraries(KuruSDKGenRuntime PRIVATE KuruCommon KuruSDKBase)
target_compile_definitions(KuruSDKGenRuntime PRIVATE KURU_SDKGEN_RUNTIME_BUILD NOMINMAX WIN32_LEAN_AND_MEAN)
