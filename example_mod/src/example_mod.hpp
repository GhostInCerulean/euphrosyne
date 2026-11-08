#pragma once
#include "euphrosyne/mods/mod_info.h"
#include <flecs.h>

extern "C"
{
    euphrosyne_mod_info_t ModInfo();
    void ModInit(quill::Logger *logger);
    void ModInject(flecs::world *ecs);
    void ModDeinit();
}