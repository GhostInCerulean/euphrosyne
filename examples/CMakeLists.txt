embed_shader(
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/triangle.vert
        g_triangle_vert
        ${SHADER_GEN_DIR}/triangle.vert.h
)
embed_shader(
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/triangle.frag
        g_triangle_frag
        ${SHADER_GEN_DIR}/triangle.frag.h
)


add_executable(euphrosyne_example
        main.cpp
        ${SHADER_GEN_DIR}/triangle.vert.h
        ${SHADER_GEN_DIR}/triangle.frag.h
)

target_link_libraries(euphrosyne_example
        euphrosyne
)

target_include_directories(euphrosyne_example PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${SHADER_GEN_DIR}
)