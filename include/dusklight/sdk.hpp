#pragma once

#include <flecs.h>

#include <dusklight/modding/components.hpp>
#include <dusklight/modding/events.hpp>
#include <dusklight/modding/version.hpp>

#if defined(_WIN32)
#define DUSK_MOD_EXPORT __declspec(dllexport)
#else
#define DUSK_MOD_EXPORT __attribute__((visibility("default")))
#endif

using DuskModInitFn = void (*)(flecs::world*);

#define DUSK_MOD_INIT(name) \
    extern "C" DUSK_MOD_EXPORT void dusk_mod_init(flecs::world* name)

