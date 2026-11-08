#include "example_mod.hpp"

namespace mod
{
struct Module
{
    Module(flecs::world &world)
    {
    }
};
} // namespace mod

extern "C"
{
    euphrosyne_mod_info_t ModInfo()
    {
        return {.name_and_version = {.name = "Example mod", .semver = {.major = 1, .minor = 0, .patch = 0}},
                .dependencies = nullptr,
                .dependencies_count = 0};
    }

    void ModInit(quill::Logger *logger)
    {
        LOG_INFO(logger, "Example Mod init");
    }

    void ModInject(flecs::world *ecs)
    {
        ecs->import<mod::Module>();
    }

    void ModDeinit()
    {
    }
}