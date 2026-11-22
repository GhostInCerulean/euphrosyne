file(GLOB_RECURSE EUPHROSYNE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(euphrosyne STATIC
        ${EUPHROSYNE_SOURCES}
        formats/texture.hpp
)

find_package(Vulkan REQUIRED)
target_link_libraries(nvrhi_vk PUBLIC nvrhi)
target_link_libraries(euphrosyne PUBLIC
        nvrhi
        nvrhi_vk
        flecs::flecs
        SDL3::SDL3
        flux::flux
        glaze::glaze
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        Boost::unordered
        Boost::asio
        Boost::system
        Boost::dll
        quill::quill
        quill::fmtquill
        hlslpp::hlslpp
        glm::glm
        dylib
        soloud
        euphrosyne_cpu_flags
)

target_precompile_headers(euphrosyne PUBLIC
        pch.hpp
)

target_include_directories(euphrosyne PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_compile_definitions(euphrosyne PUBLIC
        BOOST_ASIO_HAS_IO_URING=1
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)