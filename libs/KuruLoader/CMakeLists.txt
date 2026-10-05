add_library(KuruLoader SHARED
    src/DllMain.cpp
    src/HookEngine.cpp
    src/Gui.cpp
    src/Console.cpp
    src/ModRegistry.cpp
    src/Scanner.cpp
    src/PropertyView.cpp
    src/Reflector.cpp
    src/ThreadBridge.cpp
    src/Storage.cpp
    src/Variables.cpp
)


target_link_libraries(KuruLoader PRIVATE KuruCommon KuruSDKBase minhook imgui unordered_dense)

# SDK generation is a separate in-game step. SDKGen itself does not depend on this output.
set(KURU_GENERATED_SDK "${CMAKE_BINARY_DIR}/libs/KuruSDKGen/Release/generated-sdk"
    CACHE PATH "Generated Rocket League SDK used by the loader")
target_include_directories(KuruLoader PRIVATE "${KURU_GENERATED_SDK}")
if(NOT EXISTS "${KURU_GENERATED_SDK}/Kuru/Api.hpp")
    message(STATUS "KuruLoader requires a generated SDK at ${KURU_GENERATED_SDK}; build/inject KuruSDKGen first or set KURU_GENERATED_SDK")
endif()

target_compile_definitions(KuruLoader PRIVATE KURU_LOADER_BUILD NOMINMAX WIN32_LEAN_AND_MEAN)
