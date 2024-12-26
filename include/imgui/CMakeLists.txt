target_sources(AutoSprint PRIVATE
    #${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_win32.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_dx11.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_dx12.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui.cpp
)
