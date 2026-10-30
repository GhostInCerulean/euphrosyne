add_library(euphrosyne SHARED
        dummy.cpp
)

find_package(Vulkan REQUIRED)
target_link_libraries(nvrhi_vk PUBLIC nvrhi)
target_link_libraries(euphrosyne PUBLIC
        nvrhi
        nvrhi_vk
        flecs::flecs
        SDL3::SDL3
        glaze::glaze
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
)

target_include_directories(euphrosyne PUBLIC
        ${CMAKE_SOURCE_DIR}
)