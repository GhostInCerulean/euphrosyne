file(GLOB_RECURSE EUPHROSYNE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(euphrosyne STATIC
        ${EUPHROSYNE_SOURCES}
)

target_link_libraries(euphrosyne PUBLIC
        Diligent-Common
        Diligent-BasicPlatform
        Diligent-Archiver-shared
        Diligent-GraphicsEngineOpenGL-shared
        Diligent-GraphicsEngineVk-shared
        flecs::flecs
        SDL3::SDL3
        flux::flux
        glaze::glaze
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