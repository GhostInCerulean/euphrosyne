add_library(euphrosyne STATIC
        formats/archive.cpp
        formats/archive.hpp
        pch.hpp
        utils/bytes/tag.hpp
        utils/hash/fnv1a.hpp
        utils/bytes/align.hpp
        dev/logging.cpp
        dev/logging.hpp
        utils/platform/paths.cpp
        utils/platform/paths.hpp
        rendering/renderer.cpp
        rendering/renderer.hpp
        rendering/gpu_context.cpp
        rendering/gpu_context.hpp
        rendering/resource_manager.cpp
        rendering/resource_manager.hpp
        rendering/pipeline_cache.cpp
        rendering/pipeline_cache.hpp
)

find_package(Vulkan REQUIRED)
target_link_libraries(nvrhi_vk PUBLIC nvrhi)
target_link_libraries(euphrosyne PUBLIC
        nvrhi
        nvrhi_vk
        flecs::flecs_static
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
        soloud
)

target_precompile_headers(euphrosyne PUBLIC
        pch.hpp
)

target_include_directories(euphrosyne PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_compile_definitions(euphrosyne PUBLIC
        BOOST_ASIO_HAS_IO_URING
)