#pragma once

struct fopAc_ac_c;

namespace dusk::modding::game_bridge {

void initialize();
void shutdown();
void on_actor_created(fopAc_ac_c* actor);
void on_actor_destroyed(fopAc_ac_c* actor);
void sync_before_mods(float deltaSeconds);

}  // namespace dusk::modding::game_bridge

