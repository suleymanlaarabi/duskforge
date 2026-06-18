#include "dusk/modding/sdk_world.hpp"

#include <memory>

#include "dusklight/modding/components.hpp"
#include "dusklight/modding/events.hpp"
#include "flecs.h"

namespace dusk::modding {
namespace {

std::unique_ptr<flecs::world> gWorld;
flecs::entity gFrameEntity;
uint64_t gFrameIndex = 0;

void register_public_types(flecs::world& world) {
    world.component<dusklight::Transform>("dusklight::Transform");
    world.component<dusklight::Velocity>("dusklight::Velocity");
    world.component<dusklight::Actor>("dusklight::Actor");
    world.component<dusklight::NativeActor>("dusklight::NativeActor");

    world.component<dusklight::Player>("dusklight::Player");
    world.component<dusklight::Enemy>("dusklight::Enemy");
    world.component<dusklight::Npc>("dusklight::Npc");
    world.component<dusklight::Item>("dusklight::Item");
    world.component<dusklight::Scene>("dusklight::Scene");
    world.component<dusklight::Room>("dusklight::Room");

    world.component<dusklight::events::Frame>("dusklight::events::Frame");
    world.component<dusklight::events::ActorCreated>("dusklight::events::ActorCreated");
    world.component<dusklight::events::ActorDestroyed>("dusklight::events::ActorDestroyed");
}

}  // namespace

flecs::world& world() {
    if (!gWorld) {
        initialize();
    }
    return *gWorld;
}

void initialize() {
    if (gWorld) {
        return;
    }

    gWorld = std::make_unique<flecs::world>();
    gWorld->set(flecs::Rest{});
    gFrameIndex = 0;
    register_public_types(*gWorld);
    gFrameEntity = gWorld->entity("dusk_frame");
}

void shutdown() {
    gFrameEntity = {};
    gWorld.reset();
    gFrameIndex = 0;
}

void progress(float deltaSeconds) {
    if (!gWorld) {
        return;
    }

    const dusklight::events::Frame frame{
        .delta_seconds = deltaSeconds,
        .frame_index = gFrameIndex,
    };
    gFrameEntity.set(frame);
    gWorld->progress(deltaSeconds);
    ++gFrameIndex;
}

uint64_t frame_index() {
    return gFrameIndex;
}

bool is_initialized() {
    return static_cast<bool>(gWorld);
}

}  // namespace dusk::modding
