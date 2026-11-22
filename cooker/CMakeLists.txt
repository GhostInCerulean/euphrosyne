file(GLOB_RECURSE EUPHROSYNE_COOKER_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_executable(euphrosyne_cooker
        ${EUPHROSYNE_COOKER_SOURCES}
        src/blender/blender_to_gltf.cpp
        src/blender/blender_to_gltf.hpp
        src/texture_cooker.cpp
        src/texture_cooker.hpp
)

find_package(Vulkan REQUIRED COMPONENTS dxc)

target_link_libraries(euphrosyne_cooker PRIVATE
        euphrosyne
        CLI11::CLI11
        Vulkan::dxc_lib
        fastgltf::fastgltf
        Boost::process
        ktx
        meshoptimizer
        euphrosyne_cpu_flags
)

if (WIN32)
    get_filename_component(DXC_BIN_DIR "${Vulkan_dxc_EXECUTABLE}" DIRECTORY)
    add_custom_command(TARGET euphrosyne_cooker POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DXC_BIN_DIR}/dxcompiler.dll"
            $<TARGET_FILE_DIR:euphrosyne_cooker>
            VERBATIM
    )
endif ()