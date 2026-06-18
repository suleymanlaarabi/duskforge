#include "dusk/modding/game_bridge.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "dusklight/modding/components.hpp"
#include "dusklight/modding/events.hpp"
#include "dusk/modding/sdk_world.hpp"
#include "f_op/f_op_actor.h"
#include "f_op/f_op_actor_iter.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_name.h"

namespace dusk::modding::game_bridge {
namespace {

std::unordered_map<fpc_ProcID, flecs::entity> gActors;
std::unordered_set<fpc_ProcID> gSeenThisFrame;
bool gInitialized = false;

dusklight::Transform transform_from_actor(const fopAc_ac_c* actor) {
    const auto* pos = fopAcM_GetPosition_p(actor);
    return {
        .x = pos->x,
        .y = pos->y,
        .z = pos->z,
        .pitch = actor->current.angle.x,
        .yaw = actor->current.angle.y,
        .roll = actor->current.angle.z,
    };
}

dusklight::Velocity velocity_from_actor(const fopAc_ac_c* actor) {
    const auto* speed = fopAcM_GetSpeed_p(actor);
    return {
        .x = speed->x,
        .y = speed->y,
        .z = speed->z,
    };
}

dusklight::Actor actor_component_from_actor(const fopAc_ac_c* actor) {
    return {
        .id = static_cast<uint32_t>(fopAcM_GetID(actor)),
        .profile = fopAcM_GetProfName(actor),
        .room = fopAcM_GetRoomNo(actor),
        .group = fopAcM_GetGroup(actor),
        .params = fopAcM_GetParam(actor),
    };
}

bool is_item_actor(const fopAc_ac_c* actor) {
    const s16 profile = fopAcM_GetProfName(actor);
    return profile == fpcNm_ITEM_e || profile == fpcNm_Demo_Item_e ||
           profile == fpcNm_OBJ_SSITEM_e;
}

void apply_tags(flecs::entity entity, const fopAc_ac_c* actor) {
    switch (fopAcM_GetGroup(actor)) {
    case fopAc_PLAYER_e:
        entity.add<dusklight::Player>();
        break;
    case fopAc_ENEMY_e:
        entity.add<dusklight::Enemy>();
        break;
    case fopAc_NPC_e:
        entity.add<dusklight::Npc>();
        break;
    default:
        break;
    }

    if (is_item_actor(actor)) {
        entity.add<dusklight::Item>();
    }
}

void update_entity(flecs::entity entity, fopAc_ac_c* actor) {
    entity.set(actor_component_from_actor(actor));
    entity.set(dusklight::NativeActor{
        .id = static_cast<uint32_t>(fopAcM_GetID(actor)),
        .ptr = actor,
    });
    entity.set(transform_from_actor(actor));
    entity.set(velocity_from_actor(actor));
}

void destroy_entity(fpc_ProcID id, flecs::entity entity) {
    const dusklight::events::ActorDestroyed event{.id = static_cast<uint32_t>(id)};
    entity.emit(event);
    entity.destruct();
}

// Runs once per frame over the native actor list. The callback only does hash
// lookups and POD component writes; mod queries then read a stable ECS snapshot.
int sync_actor_callback(void* rawActor, void*) {
    auto* actor = static_cast<fopAc_ac_c*>(rawActor);
    const fpc_ProcID id = fopAcM_GetID(actor);
    gSeenThisFrame.insert(id);

    auto it = gActors.find(id);
    if (it == gActors.end()) {
        on_actor_created(actor);
        it = gActors.find(id);
    }
    if (it != gActors.end()) {
        update_entity(it->second, actor);
    }
    return 0;
}

}  // namespace

void initialize() {
    if (gInitialized) {
        return;
    }
    gInitialized = true;
}

void shutdown() {
    for (auto& [id, entity] : gActors) {
        if (entity.is_alive()) {
            destroy_entity(id, entity);
        }
    }
    gActors.clear();
    gSeenThisFrame.clear();
    gInitialized = false;
}

void on_actor_created(fopAc_ac_c* actor) {
    if (!gInitialized || actor == nullptr || !is_initialized()) {
        return;
    }

    const fpc_ProcID id = fopAcM_GetID(actor);
    if (gActors.find(id) != gActors.end()) {
        return;
    }

    auto entity = world().entity(("actor_" + std::to_string(id)).c_str());
    apply_tags(entity, actor);
    update_entity(entity, actor);

    const dusklight::events::ActorCreated event{.id = static_cast<uint32_t>(id)};
    entity.emit(event);
    gActors.emplace(id, entity);
}

void on_actor_destroyed(fopAc_ac_c* actor) {
    if (!gInitialized || actor == nullptr || !is_initialized()) {
        return;
    }

    const fpc_ProcID id = fopAcM_GetID(actor);
    auto it = gActors.find(id);
    if (it == gActors.end()) {
        return;
    }

    destroy_entity(id, it->second);
    gActors.erase(it);
}

void sync_before_mods(float) {
    if (!gInitialized || !is_initialized()) {
        return;
    }

    // The full scan is intentional. It makes the SDK robust against engine code
    // paths that bypass our create/delete hooks, while keeping the work O(actor_count).
    gSeenThisFrame.clear();
    fopAcIt_Executor(&sync_actor_callback, nullptr);

    for (auto it = gActors.begin(); it != gActors.end();) {
        if (gSeenThisFrame.find(it->first) == gSeenThisFrame.end()) {
            destroy_entity(it->first, it->second);
            it = gActors.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace dusk::modding::game_bridge
