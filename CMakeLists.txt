cmake_minimum_required(VERSION 3.16)
project(ghosttap LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ImGui sources
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(ghosttap
    src/main.cpp
    src/gui.cpp
    ${IMGUI_SOURCES}
)

target_include_directories(ghosttap PRIVATE
    include
    ${IMGUI_DIR}
)

target_link_libraries(ghosttap PRIVATE
    pthread
    glfw
    OpenGL::GL
)

install(TARGETS ghosttap DESTINATION bin)
