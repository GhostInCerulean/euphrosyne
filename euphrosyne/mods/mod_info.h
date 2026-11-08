#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        uint8_t major, minor, patch;
    } euphrosyne_semver_t;

    typedef struct
    {
        const char *name;
        euphrosyne_semver_t semver;
    } euphrosyne_name_and_version_t;

    typedef struct
    {
        euphrosyne_name_and_version_t name_and_version;
        euphrosyne_name_and_version_t *dependencies;
        size_t dependencies_count;
    } euphrosyne_mod_info_t;

#ifdef __cplusplus
}
#endif