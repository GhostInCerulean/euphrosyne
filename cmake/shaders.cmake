find_program(DXC_EXECUTABLE NAMES dxc
        HINTS $ENV{VULKAN_SDK}/bin $ENV{VULKAN_SDK}/Bin
        REQUIRED)

# compile_hlsl_shaders(
#     TARGET        <target>              # target that depends on the compiled shaders
#     DIRECTORY     <folder>              # searched recursively for *.hlsl
#     [OUTPUT_DIR   <dir>]                # default: ${CMAKE_BINARY_DIR}/shaders
#     [SHADER_MODEL <6_0|6_5|...>]        # default: 6_0
#     [ENTRY        <name>]               # default: main
#     [INCLUDE_DIRS <dirs...>]
#     [DEFINES      <NAME[=VALUE]...>]
#     [EXTRA_ARGS   <dxc args...>]
#     [NO_NVRHI_SHIFTS]                   # skip the register->binding shifts
# )
#
# Stage comes from the filename:  foo.vs.hlsl  foo.ps.hlsl  foo.cs.hlsl ...
# Supported: vs ps cs gs hs ds ms as. Files without a stage suffix (shared
# headers written as .hlsl) are skipped. .hlsli/.h changes trigger rebuilds.
#
# Output mirrors the source tree:  <dir>/fx/foo.vs.hlsl -> <out>/fx/foo.vs.spv
# The target also gets:  SHADER_DIR="<out>"  as a compile definition.
function(compile_hlsl_shaders)
    cmake_parse_arguments(ARG "NO_NVRHI_SHIFTS"
            "TARGET;DIRECTORY;OUTPUT_DIR;SHADER_MODEL;ENTRY"
            "INCLUDE_DIRS;DEFINES;EXTRA_ARGS" ${ARGN})

    if(NOT ARG_TARGET OR NOT ARG_DIRECTORY)
        message(FATAL_ERROR "compile_hlsl_shaders: TARGET and DIRECTORY are required")
    endif()
    if(NOT ARG_OUTPUT_DIR)
        set(ARG_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)
    endif()
    if(NOT ARG_SHADER_MODEL)
        set(ARG_SHADER_MODEL 6_0)
    endif()
    if(NOT ARG_ENTRY)
        set(ARG_ENTRY main)
    endif()
    get_filename_component(ARG_DIRECTORY "${ARG_DIRECTORY}" ABSOLUTE)

    file(GLOB_RECURSE hlsl_sources CONFIGURE_DEPENDS "${ARG_DIRECTORY}/*.hlsl")
    file(GLOB_RECURSE hlsl_headers CONFIGURE_DEPENDS
            "${ARG_DIRECTORY}/*.hlsli" "${ARG_DIRECTORY}/*.h")

    # Shared flags
    set(common_args -spirv -fspv-target-env=vulkan1.2)
    if(NOT ARG_NO_NVRHI_SHIFTS)
        # NVRHI's default Vulkan binding offsets: t=0, s=128, b=256, u=384.
        list(APPEND common_args
                -fvk-t-shift 0   all
                -fvk-s-shift 128 all
                -fvk-b-shift 256 all
                -fvk-u-shift 384 all)
    endif()
    list(APPEND ARG_INCLUDE_DIRS ${ARG_DIRECTORY})
    foreach(inc IN LISTS ARG_INCLUDE_DIRS)
        list(APPEND common_args -I "${inc}")
    endforeach()
    foreach(def IN LISTS ARG_DEFINES)
        list(APPEND common_args -D "${def}")
    endforeach()
    list(APPEND common_args ${ARG_EXTRA_ARGS})

    set(outputs "")
    foreach(src IN LISTS hlsl_sources)
        get_filename_component(fname "${src}" NAME)
        if(NOT fname MATCHES "^(.+)\\.(vs|ps|cs|gs|hs|ds|ms|as)\\.hlsl$")
            message(STATUS "compile_hlsl_shaders: skipping ${fname} (no stage suffix)")
            continue()
        endif()
        set(stage ${CMAKE_MATCH_2})

        file(RELATIVE_PATH rel "${ARG_DIRECTORY}" "${src}")
        string(REGEX REPLACE "\\.hlsl$" ".spv" rel_out "${rel}")
        set(out "${ARG_OUTPUT_DIR}/${rel_out}")
        get_filename_component(out_dir "${out}" DIRECTORY)

        add_custom_command(
                OUTPUT  "${out}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${out_dir}"
                COMMAND ${DXC_EXECUTABLE} ${common_args}
                -T ${stage}_${ARG_SHADER_MODEL} -E ${ARG_ENTRY}
                -Fo "${out}" "${src}"
                DEPENDS "${src}" ${hlsl_headers}
                COMMENT "DXC ${rel} -> ${rel_out}"
                VERBATIM)
        list(APPEND outputs "${out}")
    endforeach()

    add_custom_target(${ARG_TARGET}_shaders DEPENDS ${outputs})
    add_dependencies(${ARG_TARGET} ${ARG_TARGET}_shaders)
    target_compile_definitions(${ARG_TARGET} PRIVATE SHADER_DIR="${ARG_OUTPUT_DIR}")
endfunction()