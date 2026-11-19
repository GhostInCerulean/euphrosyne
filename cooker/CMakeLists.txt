add_executable(euphrosyne_cooker
        main.cpp
        archive/archive_writer.cpp
        archive/archive_writer.hpp
        shaderc/compile.cpp
        shaderc/compile.hpp
)

find_package(Vulkan REQUIRED COMPONENTS dxc)

target_link_libraries(euphrosyne_cooker PRIVATE
        euphrosyne
        CLI11::CLI11
        Vulkan::dxc_lib
        fastgltf::fastgltf
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