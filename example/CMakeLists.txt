add_library(euphrosyne_example_core STATIC
        src/example_game.cpp
        src/example_game.hpp
)

target_link_libraries(euphrosyne_example_core PUBLIC
        euphrosyne
)

add_executable(euphrosyne_example
        src/main.cpp
)

target_link_libraries(euphrosyne_example
        euphrosyne_example_core
)

euphrosyne_cook_shaders(euphrosyne_example shaders.pak
        ROOTS
        ${CMAKE_SOURCE_DIR}/example/assets/shaders
        ${CMAKE_SOURCE_DIR}/euphrosyne/assets/shaders
)