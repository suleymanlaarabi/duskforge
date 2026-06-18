#include <dusklight/sdk.hpp>
#include <format>
#include "dusklight/modding/components.hpp"
#include "dusklight/modding/log.hpp"
#include "flecs.h"

DUSK_MOD_INIT(_world) {
    flecs::world& world = *_world;

    dusklight::log::info("hello_flecs", "initialized");

    world.system<const dusklight::events::Frame>()
        .kind(flecs::OnUpdate)
        .each([](flecs::entity, const dusklight::events::Frame&) {});

    world.observer<const dusklight::Actor>()
        .event(flecs::OnAdd)
        .each([](const dusklight::Actor& actor) {
            dusklight::log::info("hello_flecs", std::format("actor: {}", actor.profile));
        });

    world.system<const dusklight::Transform>()
        .kind(flecs::OnUpdate)
        .each([](flecs::entity, const dusklight::Transform&) {});
}
