# The SDK Generator
find_package(Python3 REQUIRED COMPONENTS Interpreter)
get_target_property(_imgui_includes imgui INTERFACE_INCLUDE_DIRECTORIES)
list(GET _imgui_includes 0 _template_imgui_dir)
file(GLOB_RECURSE _template_inputs CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/sdk-base/*" "${PROJECT_SOURCE_DIR}/templates/mod-template/*")
file(GLOB _template_imgui_inputs CONFIGURE_DEPENDS "${_template_imgui_dir}/*.h" "${_template_imgui_dir}/*.cpp")
set(_template_bundle "${CMAKE_CURRENT_BINARY_DIR}/embedded/template.cpp")
add_custom_command(OUTPUT "${_template_bundle}"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/package_template.py"
        "${PROJECT_SOURCE_DIR}" "${_template_imgui_dir}" "${_template_bundle}"
    DEPENDS ${_template_inputs} ${_template_imgui_inputs}
        "${_template_imgui_dir}/LICENSE.txt" "${PROJECT_SOURCE_DIR}/LICENSE"
        "${CMAKE_CURRENT_SOURCE_DIR}/package_template.py"
    VERBATIM)
add_library(KuruModTemplate STATIC src/TemplateBundle.cpp "${_template_bundle}")
target_include_directories(KuruModTemplate PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
add_library(KuruSDKGen SHARED
    src/DllMain.cpp
    src/ReflectionWalk.cpp
    src/Generator.cpp
)

target_link_libraries(KuruSDKGen PRIVATE KuruCommon KuruSDKBase KuruModTemplate version)
target_include_directories(KuruSDKGen PRIVATE ${CMAKE_SOURCE_DIR}/sdk-base)
target_compile_definitions(KuruSDKGen PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
