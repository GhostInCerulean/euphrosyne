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
)