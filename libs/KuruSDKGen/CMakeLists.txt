add_executable(KuruSDKGen
    src/Main.cpp
)

target_link_libraries(KuruSDKGen PRIVATE KuruCommon)
target_include_directories(KuruSDKGen PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk-base
)
target_compile_definitions(KuruSDKGen PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
