# euphrosyne_cook_shaders(<target> <archive_name> ROOTS <dir>...)
function(euphrosyne_cook_shaders target archive_name)
    cmake_parse_arguments(ARG "" "" "ROOTS" ${ARGN})

    set(sources "")
    foreach (root IN LISTS ARG_ROOTS)
        file(GLOB_RECURSE found CONFIGURE_DEPENDS
                ${root}/*.hlsl ${root}/*.hlsli)
        list(APPEND sources ${found})
    endforeach ()

    # Plain path: OUTPUT can't contain target-dependent genexes
    set(archive ${CMAKE_CURRENT_BINARY_DIR}/${archive_name})

    add_custom_command(
            OUTPUT ${archive}
            COMMAND euphrosyne_cooker shaderc ${ARG_ROOTS} -o ${archive}
            DEPENDS euphrosyne_cooker ${sources}
            COMMENT "Cooking shaders -> ${archive_name}"
            VERBATIM
    )
    add_custom_target(${target}_shaders DEPENDS ${archive})
    add_dependencies(${target} ${target}_shaders)

    # Copy next to the exe (genexes are fine in COMMAND)
    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${archive} $<TARGET_FILE_DIR:${target}>/${archive_name}
            VERBATIM
    )
endfunction()