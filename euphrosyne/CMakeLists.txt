add_library(euphrosyne SHARED
        dummy.cpp
)

target_link_libraries(euphrosyne PUBLIC
        nvrhi
        nvrhi_vk
        flecs::flecs
        SDL3::SDL3
        glaze::glaze
        vk-bootstrap::vk-bootstrap
)

target_include_directories(euphrosyne PUBLIC
        ${CMAKE_SOURCE_DIR}
)