add_executable(euphrosyne_cooker
        main.cpp
        shaders/shader_archive_writer.cpp
        shaders/shader_archive_writer.hpp
        shaderc/run.cpp
        shaderc/run.hpp
)

target_link_libraries(euphrosyne_cooker PRIVATE
        euphrosyne
        CLI11::CLI11
)