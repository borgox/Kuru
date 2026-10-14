# The SDK runtime is a normal STATIC lib.
add_library(KuruSDKBaseRuntime STATIC
    src/Archive.cpp
    src/Globals.cpp
    src/HostApi.cpp
    src/KuruLog.cpp
    src/Names.cpp
    src/ObjectModel.cpp
    src/Package.cpp
    src/PropertyTag.cpp
    src/Render.cpp
    src/Script.cpp
    src/Strings.cpp
    src/World.cpp
)
# TODO: Check all implementations to ensure they're valid
target_include_directories(KuruSDKBaseRuntime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(KuruSDKBaseRuntime PUBLIC cxx_std_20)
set_target_properties(KuruSDKBaseRuntime PROPERTIES POSITION_INDEPENDENT_CODE ON)

# What the host and mods link: SDK code + headers, compiled into each consumer.
add_library(KuruSDKBase INTERFACE)
target_include_directories(KuruSDKBase INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KuruSDKBase INTERFACE KuruSDKBaseRuntime)
target_compile_features(KuruSDKBase INTERFACE cxx_std_20)
