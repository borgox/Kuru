# Third-party dependencies. Fetched from source and built with our toolchain

include(FetchContent)

# MinHook, MIT licensed.
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        master
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(MINHOOK_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(minhook)

# Dear ImGui, MIT licensed
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC d3d11 dxgi dxguid)

# IconFontCppHeaders: header-only C++ defines for icon-font codepoints (Lucide, FontAwesome, ...).
FetchContent_Declare(
    iconfontcppheaders
    GIT_REPOSITORY https://github.com/juliettef/IconFontCppHeaders.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(iconfontcppheaders)

add_library(iconfonts INTERFACE)
target_include_directories(iconfonts INTERFACE ${iconfontcppheaders_SOURCE_DIR})

# Lucide icon font (ISC licensed). Fetched once into the build tree; the injector ships/loads it. If the
# download is unavailable the injector still builds and simply renders without glyphs.
set(KURU_LUCIDE_TTF ${CMAKE_BINARY_DIR}/lucide.ttf CACHE INTERNAL "Path to the Lucide font")
if(NOT EXISTS ${KURU_LUCIDE_TTF})
    file(DOWNLOAD
        https://cdn.jsdelivr.net/npm/lucide-static/font/lucide.ttf
        ${KURU_LUCIDE_TTF}
        STATUS _lucide_status)
    list(GET _lucide_status 0 _lucide_code)
    if(NOT _lucide_code EQUAL 0)
        message(WARNING "Could not download Lucide font (icons disabled): ${_lucide_status}")
        file(REMOVE ${KURU_LUCIDE_TTF})
    endif()
endif()
