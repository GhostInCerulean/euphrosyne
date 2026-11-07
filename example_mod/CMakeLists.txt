add_library(example_mod SHARED
        src/example_mod.cpp
        src/example_mod.hpp
)

target_link_libraries(example_mod
        euphrosyne_example_core
)