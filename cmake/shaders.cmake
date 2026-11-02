find_program(GLSLANG_VALIDATOR glslangValidator
        HINTS $ENV{VULKAN_SDK}/bin $ENV{VULKAN_SDK}/Bin
        REQUIRED)

set(SHADER_GEN_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${SHADER_GEN_DIR})

function(embed_shader SRC VARNAME OUT)
    add_custom_command(
            OUTPUT  ${OUT}
            COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.2 --vn ${VARNAME} -o ${OUT} ${SRC}
            DEPENDS ${SRC}
            COMMENT "Compiling ${SRC}"
            VERBATIM)
endfunction()

