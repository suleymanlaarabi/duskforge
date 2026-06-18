#include <dusklight/sdk.hpp>

DUSK_MOD_INIT(world) {
    world->system<const dusklight::events::Frame>()
        .kind(flecs::OnUpdate)
        .each([](flecs::entity, const dusklight::events::Frame&) {});

    world->system<const dusklight::Transform>()
        .kind(flecs::OnUpdate)
        .each([](flecs::entity, const dusklight::Transform&) {});
}
