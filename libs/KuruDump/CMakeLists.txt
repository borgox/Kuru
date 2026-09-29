add_library(KuruDump SHARED
    src/DllMain.cpp
)

target_link_libraries(KuruDump PRIVATE KuruCommon version)
target_compile_definitions(KuruDump PRIVATE KURU_DUMP_BUILD NOMINMAX WIN32_LEAN_AND_MEAN)
