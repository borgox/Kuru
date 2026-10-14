# The Kuru Injector: a small Dear ImGui desktop app that loads the mod loader / SDK generator

find_program(KURU_PYTHON NAMES python python3 REQUIRED)

set(_embed_dir ${CMAKE_CURRENT_BINARY_DIR}/embedded)
file(MAKE_DIRECTORY ${_embed_dir})
set(_embed_script ${CMAKE_CURRENT_SOURCE_DIR}/embed_file.py)

# Keep notices available even when only Kuru.exe is distributed.
set(_notices "${_embed_dir}/NOTICES.txt")
file(READ "${PROJECT_SOURCE_DIR}/LICENSE" _kuru_license)
file(READ "${PROJECT_SOURCE_DIR}/THIRD_PARTY_LICENSES" _third_party_licenses)
file(CONFIGURE OUTPUT "${_notices}"
    CONTENT "@_kuru_license@\n@_third_party_licenses@" @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/LICENSE" "${PROJECT_SOURCE_DIR}/THIRD_PARTY_LICENSES")
add_custom_command(OUTPUT ${_embed_dir}/notices.cpp
    COMMAND ${KURU_PYTHON} ${_embed_script} ${_notices} ${_embed_dir}/notices.cpp KURU_NOTICES_DATA
    DEPENDS ${_notices} ${_embed_script}
    VERBATIM)

# Embed a built target's DLL as a C++ byte array.
add_custom_command(OUTPUT ${_embed_dir}/loader.cpp
    COMMAND ${KURU_PYTHON} ${_embed_script} $<TARGET_FILE:KuruLoader> ${_embed_dir}/loader.cpp KURU_LOADER_DATA
    DEPENDS KuruLoader ${_embed_script}
    VERBATIM)
add_custom_command(OUTPUT ${_embed_dir}/sdkgen.cpp
    COMMAND ${KURU_PYTHON} ${_embed_script} $<TARGET_FILE:KuruSDKGen> ${_embed_dir}/sdkgen.cpp KURU_SDKGEN_DATA
    DEPENDS KuruSDKGen ${_embed_script}
    VERBATIM)

# Embed the icon font if it was fetched
if(EXISTS ${KURU_LUCIDE_TTF})
    add_custom_command(OUTPUT ${_embed_dir}/font.cpp
        COMMAND ${KURU_PYTHON} ${_embed_script} ${KURU_LUCIDE_TTF} ${_embed_dir}/font.cpp KURU_ICONFONT_DATA
        DEPENDS ${KURU_LUCIDE_TTF} ${_embed_script}
        VERBATIM)
else()
    file(WRITE ${_embed_dir}/font.cpp
        "extern const unsigned char KURU_ICONFONT_DATA[];\nextern const unsigned int KURU_ICONFONT_DATA_size;\nconst unsigned char KURU_ICONFONT_DATA[] = {0};\nconst unsigned int KURU_ICONFONT_DATA_size = 0u;\n")
endif()

add_executable(KuruInjector WIN32
    src/main.cpp
    src/Injection.cpp
    src/Mods.cpp
    src/Config.cpp
    ${_embed_dir}/loader.cpp
    ${_embed_dir}/sdkgen.cpp
    ${_embed_dir}/font.cpp
    ${_embed_dir}/notices.cpp
)

target_include_directories(KuruInjector PRIVATE src)
target_link_libraries(KuruInjector PRIVATE imgui iconfonts comdlg32 shell32)
target_compile_definitions(KuruInjector PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)

# The single shippable file is Kuru.exe (with the loader + SDK generator embedded inside it).
set_target_properties(KuruInjector PROPERTIES OUTPUT_NAME "Kuru")

install(TARGETS KuruInjector RUNTIME DESTINATION . COMPONENT KuruRelease)
install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" "${PROJECT_SOURCE_DIR}/THIRD_PARTY_LICENSES"
    DESTINATION . COMPONENT KuruRelease)

# Use the same source manifest as the generator's embedded template.
get_target_property(_release_imgui_includes imgui INTERFACE_INCLUDE_DIRECTORIES)
list(GET _release_imgui_includes 0 _release_imgui_dir)
add_custom_target(KuruReleaseTemplate
    COMMAND ${KURU_PYTHON} "${PROJECT_SOURCE_DIR}/libs/KuruSDKGen/package_template.py"
        "${PROJECT_SOURCE_DIR}" "${_release_imgui_dir}"
        "${PROJECT_SOURCE_DIR}/build/releases/Kuru-mod-template-${PROJECT_VERSION}.zip"
        --sdk "${KURU_GENERATED_SDK}"
    COMMENT "Packaging the standalone mod template with the generated SDK"
    VERBATIM)
