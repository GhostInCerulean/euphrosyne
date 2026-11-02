add_executable(euphrosyne_example
        main.cpp
)

target_link_libraries(euphrosyne_example
        euphrosyne
)

compile_hlsl_shaders(
        TARGET euphrosyne_example
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders
        OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders
        DEFINES USE_FOG=1
)