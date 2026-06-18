#pragma once

struct fopAc_ac_c;

namespace dusk::modding::game_bridge {

void initialize();
void shutdown();
void on_actor_created(fopAc_ac_c* actor);
void on_actor_destroyed(fopAc_ac_c* actor);

// Copies the native actor manager state into the Flecs world before mod systems run.
// Hooks cover the normal create/delete path, while this pass catches actors created
// before the bridge starts or removed through paths that do not hit our hooks.
void sync_before_mods(float deltaSeconds);

}  // namespace dusk::modding::game_bridge
