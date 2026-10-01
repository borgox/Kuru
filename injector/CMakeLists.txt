# The Kuru Injector: a small Dear ImGui desktop app that loads the mod loader / SDK generator

find_program(KURU_PYTHON NAMES python python3 REQUIRED)

set(_embed_dir ${CMAKE_CURRENT_BINARY_DIR}/embedded)
file(MAKE_DIRECTORY ${_embed_dir})
set(_embed_script ${CMAKE_CURRENT_SOURCE_DIR}/embed_file.py)

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
)

target_include_directories(KuruInjector PRIVATE src)
target_link_libraries(KuruInjector PRIVATE imgui iconfonts comdlg32 shell32)
target_compile_definitions(KuruInjector PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)

# The single shippable file is Kuru.exe (with the loader + SDK generator embedded inside it).
set_target_properties(KuruInjector PROPERTIES OUTPUT_NAME "Kuru")
