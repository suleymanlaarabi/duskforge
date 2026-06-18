#pragma once

#include <cstdint>

#include <flecs.h>

namespace dusk::modding {

flecs::world& world();
void initialize();
void shutdown();
void progress(float deltaSeconds);
uint64_t frame_index();
bool is_initialized();

}  // namespace dusk::modding

