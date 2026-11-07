add_executable(euphrosyne_example
        main.cpp
)

target_link_libraries(euphrosyne_example
        euphrosyne
)

euphrosyne_cook_shaders(euphrosyne_example shaders.pak
        ROOTS
        ${CMAKE_SOURCE_DIR}/example/assets/shaders
        ${CMAKE_SOURCE_DIR}/euphrosyne/assets/shaders
)