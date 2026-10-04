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

target_compile_definitions(KuruLoader PRIVATE KURU_LOADER_BUILD NOMINMAX WIN32_LEAN_AND_MEAN)
